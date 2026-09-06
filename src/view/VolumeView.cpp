#include "VolumeView.h"

#include "TransferFunctionFactory.h"

#include <QVTKOpenGLNativeWidget.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkRenderer.h>
#include <vtkSmartVolumeMapper.h>
#include <vtkVolume.h>
#include <vtkVolumeProperty.h>

#include <QVBoxLayout>

VolumeView::VolumeView(QWidget* parent)
    : QWidget(parent)
{
    // VTK 渲染控件
    m_vtkWidget = new QVTKOpenGLNativeWidget(this);
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_vtkWidget);

    // 渲染窗口 + 渲染器
    m_renderWindow = vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New();
    m_vtkWidget->setRenderWindow(m_renderWindow);

    m_renderer = vtkSmartPointer<vtkRenderer>::New();
    m_renderer->SetBackground(0.0, 0.0, 0.0);
    m_renderWindow->AddRenderer(m_renderer);

    // 三维视图交互：轨迹球旋转/缩放/平移
    auto style = vtkSmartPointer<vtkInteractorStyleTrackballCamera>::New();
    m_vtkWidget->interactor()->SetInteractorStyle(style);

    // 体绘制管线：智能体绘制器
    m_mapper = vtkSmartPointer<vtkSmartVolumeMapper>::New();
    m_mapper->SetBlendModeToComposite();
    // 强制 CPU 光线投射：GPU 光线投射在部分集成显卡/大数据上会触发驱动崩溃
    // (表现为 app.exec() 里无栈崩溃)。CPU 渲染稳定，后续需要 GPU 再切换。
    m_mapper->SetRequestedRenderModeToRayCast();

    m_volume = vtkSmartPointer<vtkVolume>::New();
    m_volume->SetMapper(m_mapper);
    m_renderer->AddVolume(m_volume);
}

VolumeView::~VolumeView() = default;

void VolumeView::setVolume(vtkImageData* image, TransferPreset preset)
{
    m_mapper->SetInputData(image);
    m_volume->SetProperty(TransferFunctionFactory::createVolumeProperty(preset));
    m_renderer->ResetCamera();
    // 不在此显式 Render()：setVolume 在视图未 show 时被调用, 此时无 OpenGL
    // 上下文, Render 会崩溃。改由 Qt 在 widget show/paint 时自动渲染。
    m_vtkWidget->update();
}

void VolumeView::setPreset(TransferPreset preset)
{
    m_volume->SetProperty(TransferFunctionFactory::createVolumeProperty(preset));
    m_vtkWidget->update();
}
