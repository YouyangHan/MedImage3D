#include "ResliceViewer.h"

#include <vtkCamera.h>
#include <vtkImageData.h>
#include <vtkImageReslice.h>
#include <vtkObjectFactory.h>
#include <vtkPolyData.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkResliceCursor.h>
#include <vtkResliceCursorActor.h>
#include <vtkResliceCursorLineRepresentation.h>
#include <vtkResliceCursorPolyDataAlgorithm.h>
#include <vtkResliceCursorRepresentation.h>
#include <vtkResliceCursorWidget.h>

#include <algorithm>
#include <cmath>
#include <iostream>

vtkStandardNewMacro(ResliceViewer);

ResliceViewer::ResliceViewer()
{
    // 滚轮翻层
    this->SetSliceScrollOnMouseWheel(1);

    // 限制 reslice 平面在体数据内(参考 3d-xmake)，否则十字线可能超出体数据不显示
    if (vtkResliceCursorRepresentation* rep =
            vtkResliceCursorRepresentation::SafeDownCast(
                this->GetResliceCursorWidget()->GetRepresentation())) {
        rep->SetRestrictPlaneToVolume(1);
    }

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
    this->SetInputData(image);

    // 关键：SetInputData 之后光标才有图像，此时启用十字线 widget 才能正确
    // 显示红蓝绿参考线(参考 3d-xmake 在 SetInputData 后调用 On())。
    this->GetResliceCursorWidget()->On();

    // 调整相机平行缩放(SetInputData 可能重置相机)，确保十字线/切片在视口内
    if (this->GetRenderer()) {
        double sp[3];
        int dim[3];
        image->GetSpacing(sp);
        image->GetDimensions(dim);
        double maxExtent = 1.0;
        for (int i = 0; i < 3; ++i)
            maxExtent = std::max(maxExtent, sp[i] * dim[i]);
        this->GetRenderer()->ResetCamera();
        this->GetRenderer()->GetActiveCamera()->SetParallelScale(maxExtent * 0.55);
    }

    // 诊断 + 显式设置十字线颜色/线宽
    vtkResliceCursorWidget* w = this->GetResliceCursorWidget();
    std::cerr << "[十字线诊断] enabled=" << w->GetEnabled()
              << " renderer=" << (this->GetRenderer() ? "有" : "无")
              << " rep=" << (w->GetRepresentation() ? w->GetRepresentation()->GetClassName() : "null")
              << " repVis=" << (w->GetRepresentation() ? w->GetRepresentation()->GetVisibility() : -1);
    if (vtkResliceCursor* rc = this->GetResliceCursor()) {
        int npts[3] = { -1, -1, -1 };
        for (int i = 0; i < 3; ++i) {
            if (vtkPolyData* pd = rc->GetCenterlineAxisPolyData(i))
                npts[i] = pd->GetNumberOfPoints();
        }
        std::cerr << " centerlinePts=" << npts[0] << ',' << npts[1] << ',' << npts[2];
    } else {
        std::cerr << " cursor=null";
    }
    if (auto* rep = vtkResliceCursorRepresentation::SafeDownCast(w->GetRepresentation())) {
        auto* algo = rep->GetCursorAlgorithm();
        std::cerr << " planeNormal=" << algo->GetReslicePlaneNormal()
                  << " axis1=" << algo->GetPlaneAxis1() << " axis2=" << algo->GetPlaneAxis2();
    }
    if (auto* lineRep = vtkResliceCursorLineRepresentation::SafeDownCast(w->GetRepresentation())) {
        auto* actor = lineRep->GetResliceCursorActor();
        const double colors[3][3] = { {1,0,0}, {0,1,0}, {0,0,1} };
        for (int i = 0; i < 3; ++i) {
            vtkProperty* p = actor->GetCenterlineActor(i)->GetProperty();
            p->SetColor(colors[i][0], colors[i][1], colors[i][2]);
            p->SetEdgeColor(colors[i][0], colors[i][1], colors[i][2]);
            p->SetEdgeVisibility(1);
            p->SetLighting(0);       // 线无需光照
            p->SetLineWidth(3);      // 加粗，便于观察
            std::cerr << " vis" << i << "=" << actor->GetCenterlineActor(i)->GetVisibility();
        }
    }
    if (this->GetRenderer()) {
        std::cerr << " actors=" << this->GetRenderer()->GetActors()->GetNumberOfItems()
                  << " viewProps=" << this->GetRenderer()->GetViewProps()->GetNumberOfItems();
    }
    std::cerr << std::endl;
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

    // 调整相机平行缩放以适配体数据(参考 3d-xmake，否则十字线/切片可能超出视口)
    if (this->GetRenderer() && this->GetInput()) {
        double sp[3];
        int dim[3];
        this->GetInput()->GetSpacing(sp);
        this->GetInput()->GetDimensions(dim);
        const int y = (orientation < 2) ? 2 : 1;
        const double scale = sp[y] * dim[y] / 2 * 1.1;
        this->GetRenderer()->ResetCamera();
        this->GetRenderer()->GetActiveCamera()->SetParallelScale(scale);
    }
}

void ResliceViewer::setSharedCursor(vtkResliceCursor* cursor)
{
    if (!cursor)
        return;
    this->SetResliceCursor(cursor);
    buildRepresentation();
}

void ResliceViewer::buildRepresentation()
{
    // 重建十字线 representation，让红蓝绿参考线按当前光标显示。
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
