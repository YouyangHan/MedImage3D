#pragma once

// ============================================================================
// SliceView —— 单个 MPR 切片视图(QWidget 包装)
//
// 封装 QVTKOpenGLNativeWidget + ResliceViewer，对外提供简洁接口：
//   setVolume / setOrientation / setSharedCursor。
// 三个方向(矢状/冠状/横断)各用一个 SliceView，共享同一光标实现联动。
// ============================================================================

#include "ResliceViewer.h"   // 完整类型: vtkSmartPointer<ResliceViewer> 的转换运算符需要

#include <QWidget>
#include <vtkSmartPointer.h>

class QVTKOpenGLNativeWidget;
class vtkGenericOpenGLRenderWindow;
class vtkImageData;
class vtkResliceCursor;

class SliceView : public QWidget
{
    Q_OBJECT

public:
    explicit SliceView(QWidget* parent = nullptr);
    ~SliceView() override;

    void setVolume(vtkImageData* image);
    void setOrientation(int orientation);
    void setSharedCursor(vtkResliceCursor* cursor);

    // 底层 viewer，供 ViewManager 联动控制
    ResliceViewer* viewer() { return m_viewer; }

private:
    QVTKOpenGLNativeWidget* m_vtkWidget = nullptr;
    vtkSmartPointer<vtkGenericOpenGLRenderWindow> m_renderWindow;
    vtkSmartPointer<ResliceViewer> m_viewer;
};
