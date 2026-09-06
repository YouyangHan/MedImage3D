#include "MainWindow.h"

#include <QVTKOpenGLNativeWidget.h>

#include <vtkActor.h>
#include <vtkConeSource.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkNew.h>
#include <vtkPolyDataMapper.h>
#include <vtkRenderer.h>
#include <vtkVersion.h>

#include <itkImage.h>
#include <itkVersion.h>

#include <dcmtk/config/osconfig.h>
#include <dcmtk/dcmdata/dctk.h>

#include <QStatusBar>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    m_vtkWidget = new QVTKOpenGLNativeWidget(this);
    setCentralWidget(m_vtkWidget);

    setupRenderDemo();

    // 状态栏显示三方库版本, 验证链接正确
    statusBar()->showMessage(QStringLiteral("VTK %1 | ITK %2 | DCMTK %3")
        .arg(QString::fromLatin1(vtkVersion::GetVTKVersion()))
        .arg(QString::fromLatin1(ITK_VERSION))
        .arg(QString::fromLatin1(PACKAGE_VERSION)));
}

void MainWindow::setupRenderDemo()
{
    vtkNew<vtkGenericOpenGLRenderWindow> renderWindow;
    m_vtkWidget->setRenderWindow(renderWindow);

    m_renderer = vtkRenderer::New();
    renderWindow->AddRenderer(m_renderer);
    m_renderer->SetBackground(0.1, 0.12, 0.15);

    vtkNew<vtkConeSource> cone;
    cone->SetHeight(3.0);
    cone->SetRadius(1.0);
    cone->SetResolution(64);

    vtkNew<vtkPolyDataMapper> mapper;
    mapper->SetInputConnection(cone->GetOutputPort());

    vtkNew<vtkActor> actor;
    actor->SetMapper(mapper);

    m_renderer->AddActor(actor);
    m_renderer->ResetCamera();
}
