#pragma once

#include <QMainWindow>

class QVTKOpenGLNativeWidget;
class vtkRenderer;

/**
 * @brief 主窗口 —— 环境验证骨架
 *
 * 当前功能: 在 QVTKOpenGLNativeWidget 中渲染一个 VTK 圆锥体,
 * 并在状态栏显示 VTK / ITK / DCMTK 的版本号, 用于验证三方库链接正确。
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override = default;

private:
    void setupRenderDemo();

    QVTKOpenGLNativeWidget* m_vtkWidget = nullptr;
    vtkRenderer*            m_renderer  = nullptr;
};
