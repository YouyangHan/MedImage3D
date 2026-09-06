#pragma once

// ============================================================================
// ResliceViewer —— MPR 切片查看器(vtkResliceImageViewer 子类)
//
// 设计模式：适配器(Adapter)
//   把 VTK 的 vtkResliceImageViewer(内部 vtkImageReslice + 十字线 widget)
//   适配成"设置体数据 / 设置方向 / 共享光标"三个语义化方法，上层无需了解
//   VTK 内部 pipeline。
//
// 三视图联动基础：三个 ResliceViewer 共享同一个 vtkResliceCursor(十字线光标)，
//   光标中心/轴向全局唯一，十字线自动同步(见 ViewManager, 步骤 6)。
// ============================================================================

#include <vtkResliceImageViewer.h>

class vtkImageData;
class vtkResliceCursor;

class ResliceViewer : public vtkResliceImageViewer
{
public:
    static ResliceViewer* New();
    vtkTypeMacro(ResliceViewer, vtkResliceImageViewer);

    // 设置体数据并初始化默认窗宽窗位(取体素范围)
    void setVolume(vtkImageData* image);

    // 设置切片方向：vtkImageViewer2::SLICE_ORIENTATION_YZ/XZ/XY
    //  (YZ=矢状, XZ=冠状, XY=横断)
    void setSliceOrientation(int orientation);

    // 共享十字线光标(三视图联动)；须在 setVolume 之后调用
    void setSharedCursor(vtkResliceCursor* cursor);

    // 从共享光标中心同步本视图切片(世界坐标 -> 切片索引)；联动时由 ViewManager 调用
    void synchronizeFromCursor();
    // 本视图切片 -> 共享光标中心(切片索引 -> 世界坐标)，并触发 ResliceAxesChangedEvent
    void synchronizeToCursor();

    // 重建十字线 representation(联动时需按新光标重建参考线)
    void buildRepresentation();

protected:
    ResliceViewer();
    ~ResliceViewer() override;
};
