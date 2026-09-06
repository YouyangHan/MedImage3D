#include "ResliceViewer.h"

#include <vtkImageData.h>
#include <vtkImageReslice.h>
#include <vtkObjectFactory.h>
#include <vtkResliceCursor.h>
#include <vtkResliceCursorPolyDataAlgorithm.h>
#include <vtkResliceCursorRepresentation.h>
#include <vtkResliceCursorWidget.h>

#include <cmath>

vtkStandardNewMacro(ResliceViewer);

ResliceViewer::ResliceViewer()
{
    // 滚轮翻层
    this->SetSliceScrollOnMouseWheel(1);
    // 注：斜切模式(OBLIQUE, 显示十字线并联动)需在 interactor 设置后切换，
    // 由 SliceView 构造在 SetupInteractor 之后调用 SetResliceModeToOblique()。
}

ResliceViewer::~ResliceViewer() = default;

void ResliceViewer::setVolume(vtkImageData* image)
{
    if (!image)
        return;

    // SetInputData 内部已自动完成：cursor 的 SetImage/SetCenter、
    // 默认窗宽窗位(全范围)、重采样背景色(体素最小值)。
    // 注意：不再调用 Reset()——它在轴对齐模式下会触发 InitializeReslicePlane/
    // ResetCamera 访问 cursor 的 plane，与"先 SetInputData 后共享光标"的时序
    // 交互时访问无效指针而崩溃(参考 3d-xmake 亦未调用 Reset)。
    this->SetInputData(image);
}

void ResliceViewer::setSliceOrientation(int orientation)
{
    if (orientation < vtkImageViewer2::SLICE_ORIENTATION_YZ ||
        orientation > vtkImageViewer2::SLICE_ORIENTATION_XY) {
        vtkErrorMacro("非法切片方向: " << orientation);
        return;
    }

    this->SetSliceOrientation(orientation);

    // 关键：vtkResliceImageViewer::SetSliceOrientation 只改相机，不更新
    // cursor algorithm 的平面法向(构造时设为默认 XY 后不再变)。三个视图共享
    // 光标，若不分别设置平面法向，会显示同一张切片。这里手动同步。
    if (vtkResliceCursorRepresentation* rep =
            vtkResliceCursorRepresentation::SafeDownCast(
                this->GetResliceCursorWidget()->GetRepresentation())) {
        rep->GetCursorAlgorithm()->SetReslicePlaneNormal(orientation);
    }

    // 切片移到中间
    const int* range = this->GetSliceRange();
    if (range)
        this->SetSlice(static_cast<int>((range[0] + range[1]) * 0.5));
}

void ResliceViewer::setSharedCursor(vtkResliceCursor* cursor)
{
    if (!cursor)
        return;
    this->SetResliceCursor(cursor);

    // SetResliceCursor 只替换光标引用，不重建十字线 polydata；
    // 显式重建 representation，让红蓝绿参考线按新光标显示。
    if (vtkResliceCursorRepresentation* rep =
            vtkResliceCursorRepresentation::SafeDownCast(
                this->GetResliceCursorWidget()->GetRepresentation())) {
        rep->BuildRepresentation();
    }
}

void ResliceViewer::synchronizeFromCursor()
{
    vtkResliceCursor* rc = this->GetResliceCursor();
    vtkImageData* img = this->GetInput();
    if (!rc || !img)
        return;

    // 世界坐标 -> 切片索引：world[axis] = origin[axis] + slice * spacing[axis]
    double c[3];
    rc->GetCenter(c);
    const double* origin = img->GetOrigin();
    const double* spacing = img->GetSpacing();
    const int axis = this->GetSliceOrientation();   // 0=X, 1=Y, 2=Z
    const int z = static_cast<int>(std::round((c[axis] - origin[axis]) / spacing[axis]));
    this->SetSlice(z);
}

void ResliceViewer::synchronizeToCursor()
{
    vtkResliceCursor* rc = this->GetResliceCursor();
    vtkImageData* img = this->GetInput();
    if (!rc || !img)
        return;

    // 切片索引 -> 世界坐标，更新光标中心，并广播给其他视图
    double c[3];
    rc->GetCenter(c);
    const double* origin = img->GetOrigin();
    const double* spacing = img->GetSpacing();
    const int axis = this->GetSliceOrientation();
    c[axis] = origin[axis] + this->GetSlice() * spacing[axis];
    rc->SetCenter(c);
    this->GetResliceCursorWidget()->InvokeEvent(
        vtkResliceCursorWidget::ResliceAxesChangedEvent);
}
