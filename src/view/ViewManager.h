#pragma once

// ============================================================================
// ViewManager —— 四视图编排与联动
//
// 设计模式：中介者(Mediator) + 观察者(Observer)
//   居中编排四个视图(三个 MPR 切片 + 一个体绘制)，视图之间不直接通信；
//   通过 VTK 事件(ResliceAxesChangedEvent / SliceChangedEvent)观察联动，
//   共享同一 vtkResliceCursor，保证十字线与切片位置三视图同步。
//
// 联动机制：
//   - 拖动十字线 -> ResliceAxesChangedEvent -> 三视图按光标中心同步切片；
//   - 滚轮翻层 -> SliceChangedEvent -> 更新光标中心 -> 广播给其余视图。
// ============================================================================

#include "dicom/SeriesInfo.h"
#include "core/Volume.h"

#include <QList>
#include <QWidget>
#include <vtkSmartPointer.h>

class SliceView;
class VolumeView;
class vtkResliceCursor;
class vtkCommand;
enum class TransferPreset;

class ViewManager : public QWidget
{
    Q_OBJECT

public:
    explicit ViewManager(QWidget* parent = nullptr);
    ~ViewManager() override;

    // 设置体数据并刷新四视图
    void setVolume(const Volume& volume);

    // 切换体绘制传递函数预设(骨窗/软组织/肌肉)
    void setPreset(TransferPreset preset);

    // 联动回调(VTK 事件触发，由回调对象调用)
    void onCursorChanged();                 // 光标中心变化 -> 三视图同步
    void onSliceChanged(SliceView* source); // 某视图翻层 -> 更新光标中心

private:
    void setupViews();                      // 构建四视图布局与事件连接

    QList<SliceView*> m_sliceViews;         // 三个切片视图(矢状/冠状/横断)
    VolumeView*       m_volumeView = nullptr;
    vtkSmartPointer<vtkResliceCursor> m_cursor;   // 共享十字线光标
    vtkSmartPointer<vtkCommand>       m_cursorCb; // ResliceAxesChangedEvent 回调
    vtkSmartPointer<vtkCommand>       m_sliceCb;  // SliceChangedEvent 回调
};
