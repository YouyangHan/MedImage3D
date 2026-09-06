#include "ViewManager.h"

#include "SliceView.h"
#include "TransferFunctionFactory.h"
#include "VolumeView.h"

#include <vtkCommand.h>
#include <vtkImageData.h>
#include <vtkImageViewer2.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkResliceCursor.h>
#include <vtkResliceCursorActor.h>
#include <vtkResliceCursorLineRepresentation.h>
#include <vtkResliceCursorPolyDataAlgorithm.h>
#include <vtkResliceCursorRepresentation.h>
#include <vtkResliceCursorWidget.h>
#include <vtkResliceImageViewer.h>

#include <QGridLayout>
#include <QTimer>

#include <iostream>

namespace {

// 联动回调：监听 VTK 事件，转发给 ViewManager。
//   - sliceView 非空时监听该视图的 SliceChangedEvent(滚轮翻层)；
//   - sliceView 为空时监听 ResliceAxesChangedEvent(十字线拖动)。
class ViewManagerCallback : public vtkCommand
{
public:
    static ViewManagerCallback* New() { return new ViewManagerCallback; }

    void SetManager(ViewManager* m) { m_manager = m; }
    void SetSliceView(SliceView* v) { m_sliceView = v; }

    void Execute(vtkObject*, unsigned long event, void*) override
    {
        if (!m_manager)
            return;
        if (event == vtkResliceImageViewer::SliceChangedEvent)
            m_manager->onSliceChanged(m_sliceView);
        else if (event == vtkResliceCursorWidget::ResliceAxesChangedEvent)
            m_manager->onCursorChanged();
    }

private:
    ViewManager* m_manager = nullptr;
    SliceView*   m_sliceView = nullptr;
};

} // namespace

ViewManager::ViewManager(QWidget* parent)
    : QWidget(parent)
{
    setupViews();
}

ViewManager::~ViewManager() = default;

void ViewManager::setupViews()
{
    // 共享十字线光标
    m_cursor = vtkSmartPointer<vtkResliceCursor>::New();

    // 2x2 网格：横断(上左)、矢状(上右)、冠状(下左)、体绘制(下右)
    auto* grid = new QGridLayout(this);
    grid->setContentsMargins(0, 0, 0, 0);

    const int orientations[3] = {
        vtkImageViewer2::SLICE_ORIENTATION_XY,   // 横断
        vtkImageViewer2::SLICE_ORIENTATION_YZ,   // 矢状
        vtkImageViewer2::SLICE_ORIENTATION_XZ,   // 冠状
    };
    const int positions[3][2] = { {0, 0}, {0, 1}, {1, 0} };

    // 光标变化回调(三视图同步)
    m_cursorCb = vtkSmartPointer<ViewManagerCallback>::New();
    static_cast<ViewManagerCallback*>(m_cursorCb.GetPointer())->SetManager(this);

    for (int i = 0; i < 3; ++i) {
        auto* sv = new SliceView(this);
        sv->setOrientation(orientations[i]);
        sv->setSharedCursor(m_cursor);
        grid->addWidget(sv, positions[i][0], positions[i][1]);
        m_sliceViews << sv;

        // 观察十字线拖动 -> 同步三视图
        sv->viewer()->GetResliceCursorWidget()->AddObserver(
            vtkResliceCursorWidget::ResliceAxesChangedEvent, m_cursorCb);

        // 观察滚轮翻层 -> 更新光标中心(广播给其他视图)
        auto sliceCb = vtkSmartPointer<ViewManagerCallback>::New();
        sliceCb->SetManager(this);
        sliceCb->SetSliceView(sv);
        sv->viewer()->AddObserver(vtkResliceImageViewer::SliceChangedEvent, sliceCb);
        m_sliceCb = sliceCb;   // 仅持有最后一个以保活(前几个由 viewer 引用计数持有)
    }

    // 体绘制视图
    m_volumeView = new VolumeView(this);
    grid->addWidget(m_volumeView, 1, 1);
}

void ViewManager::setVolume(const Volume& volume)
{
    if (!volume.isValid())
        return;

    // 光标中心重置到体数据中心
    m_cursor->SetImage(volume.imageData);
    m_cursor->SetCenter(volume.imageData->GetCenter());

    for (SliceView* sv : m_sliceViews) {
        sv->setVolume(volume.imageData);
        sv->setSharedCursor(m_cursor);
    }
    m_volumeView->setVolume(volume.imageData, TransferPreset::Bone);

    // 初始渲染：OBLIQUE 模式切片/十字线依赖 Render，且需在视图 show 之后
    // (setCurrentWidget 触发) 渲染，故延迟到事件循环下一轮。
    QTimer::singleShot(0, this, [this]() {
        for (SliceView* sv : m_sliceViews)
            sv->render();
        m_volumeView->render();

        // 诊断：render 后检查十字线 actor mapper 输入连接
        if (!m_sliceViews.isEmpty()) {
            auto* w = m_sliceViews[0]->viewer()->GetResliceCursorWidget();
            if (auto* lineRep = vtkResliceCursorLineRepresentation::SafeDownCast(w->GetRepresentation())) {
                auto* actor = lineRep->GetResliceCursorActor();
                for (int i = 0; i < 3; ++i) {
                    auto* mapper = vtkPolyDataMapper::SafeDownCast(actor->GetCenterlineActor(i)->GetMapper());
                    std::cerr << "[render后] line" << i << " conn="
                              << (mapper ? mapper->GetNumberOfInputConnections(0) : -1)
                              << " vis=" << actor->GetCenterlineActor(i)->GetVisibility() << "\n";
                }
            }
        }
    });
}

void ViewManager::setPreset(TransferPreset preset)
{
    m_volumeView->setPreset(preset);
}

void ViewManager::onCursorChanged()
{
    // 十字线拖动：光标中心/平面变化 -> 三视图重建十字线并刷新渲染
    for (SliceView* sv : m_sliceViews) {
        sv->buildRepresentation();
        sv->render();
    }
}

void ViewManager::onSliceChanged(SliceView* source)
{
    Q_UNUSED(source);
    // 滚轮翻层：OBLIQUE 模式已移动共享光标中心，三视图重建十字线并刷新
    for (SliceView* sv : m_sliceViews) {
        sv->buildRepresentation();
        sv->render();
    }
}
