#include "ViewManager.h"

#include "SliceView.h"
#include "TransferFunctionFactory.h"
#include "VolumeView.h"

#include <vtkCommand.h>
#include <vtkImageData.h>
#include <vtkImageViewer2.h>
#include <vtkResliceCursor.h>
#include <vtkResliceCursorWidget.h>
#include <vtkResliceImageViewer.h>

#include <QGridLayout>

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
        sv->synchronizeFromCursor();   // 初始按中心定位切片
    }
    m_volumeView->setVolume(volume.imageData, TransferPreset::Bone);
}

void ViewManager::setPreset(TransferPreset preset)
{
    m_volumeView->setPreset(preset);
}

void ViewManager::onCursorChanged()
{
    // 光标中心变化(十字线拖动) -> 三个切片视图按中心同步切片位置
    for (SliceView* sv : m_sliceViews) {
        sv->synchronizeFromCursor();
        sv->viewer()->Render();
    }
}

void ViewManager::onSliceChanged(SliceView* source)
{
    // 某视图滚轮翻层 -> 更新光标中心 -> 触发 ResliceAxesChangedEvent 广播
    if (!source)
        return;
    source->synchronizeToCursor();
}
