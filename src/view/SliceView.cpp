#include "SliceView.h"

#include "ResliceViewer.h"

#include <QVTKOpenGLNativeWidget.h>
#include <vtkGenericOpenGLRenderWindow.h>

#include <QVBoxLayout>

SliceView::SliceView(QWidget* parent)
    : QWidget(parent)
{
    // VTK 渲染控件
    m_vtkWidget = new QVTKOpenGLNativeWidget(this);
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_vtkWidget);

    // 渲染窗口与查看器绑定
    m_renderWindow = vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New();
    m_viewer = vtkSmartPointer<ResliceViewer>::New();
    m_viewer->SetRenderWindow(m_renderWindow);
    m_vtkWidget->setRenderWindow(m_renderWindow);

    // 十字线 widget 需要 interactor
    m_viewer->SetupInteractor(m_vtkWidget->interactor());
}

SliceView::~SliceView() = default;

void SliceView::setVolume(vtkImageData* image)
{
    m_viewer->setVolume(image);
}

void SliceView::setOrientation(int orientation)
{
    m_viewer->setSliceOrientation(orientation);
}

void SliceView::setSharedCursor(vtkResliceCursor* cursor)
{
    m_viewer->setSharedCursor(cursor);
}
