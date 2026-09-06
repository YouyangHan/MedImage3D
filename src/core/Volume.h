#pragma once

// ============================================================================
// Volume —— 三维体数据(值对象 Value Object)
//
// 封装由 DICOM 序列重建出的体数据，供 MPR 切片视图与三维体绘制共用。
// 像素类型为 signed short(HU 值)，spacing/origin/dimensions 由 ITK GDCM
// 自动解析，后续视图层据此做窗宽窗位、切片定位。
//
// 设计：纯数据载体，无业务逻辑；持有 VTK 智能指针(引用计数原子, 可跨线程传递)。
// ============================================================================

#include <vtkImageData.h>
#include <vtkSmartPointer.h>

#include <QMetaType>

#include <array>

class Volume
{
public:
    vtkSmartPointer<vtkImageData> imageData;              // 体数据(short, 3D)

    std::array<double, 3> spacing { 1.0, 1.0, 1.0 };      // 体素间距(mm)
    std::array<double, 3> origin  { 0.0, 0.0, 0.0 };      // 原点(世界坐标)
    std::array<int, 3>    dimensions { 0, 0, 0 };         // 各轴尺寸(像素)

    // 是否已加载有效体数据
    bool isValid() const { return imageData != nullptr; }

    // 清空，回到未加载状态
    void reset()
    {
        imageData = nullptr;
        dimensions = { 0, 0, 0 };
        spacing = { 1.0, 1.0, 1.0 };
        origin = { 0.0, 0.0, 0.0 };
    }
};

// 注册为 Qt 元类型：使 Volume 能通过队列连接跨线程传递(加载线程 -> 主线程)
Q_DECLARE_METATYPE(Volume)
