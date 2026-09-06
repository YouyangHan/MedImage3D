#include "ResliceViewer.h"

#include <vtkImageData.h>
#include <vtkImageReslice.h>
#include <vtkObjectFactory.h>
#include <vtkResliceCursor.h>
#include <vtkResliceCursorRepresentation.h>
#include <vtkResliceCursorWidget.h>

vtkStandardNewMacro(ResliceViewer);

ResliceViewer::ResliceViewer()
{
    // 轴对齐模式(正交切片)；斜切模式留给后续扩展
    this->SetResliceModeToAxisAligned();
    // 滚轮翻层
    this->SetSliceScrollOnMouseWheel(1);
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

    // 切片移到中间
    const int* range = this->GetSliceRange();
    if (range)
        this->SetSlice(static_cast<int>((range[0] + range[1]) * 0.5));
}

void ResliceViewer::setSharedCursor(vtkResliceCursor* cursor)
{
    if (cursor)
        this->SetResliceCursor(cursor);
}
