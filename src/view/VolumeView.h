#pragma once

// ============================================================================
// VolumeView —— 三维体绘制视图(QWidget 包装)
//
// 封装 QVTKOpenGLNativeWidget + vtkSmartVolumeMapper + vtkVolume，
// 对外提供 setVolume / setPreset 简洁接口。
// 传递函数(骨窗/软组织/肌肉)由 TransferFunctionFactory 按策略创建。
//
// 交互：vtkInteractorStyleTrackballCamera 旋转/缩放/平移。
// ============================================================================

#include <QWidget>
#include <vtkRenderer.h>   // renderer() 内联返回需完整类型
#include <vtkSmartPointer.h>

class QVTKOpenGLNativeWidget;
class vtkGenericOpenGLRenderWindow;
class vtkImageData;
class vtkSmartVolumeMapper;
class vtkVolume;
enum class TransferPreset;

class VolumeView : public QWidget
{
    Q_OBJECT

public:
    explicit VolumeView(QWidget* parent = nullptr);
    ~VolumeView() override;

    // 设置体数据与传递函数预设(默认骨窗)
    void setVolume(vtkImageData* image, TransferPreset preset);

    // 切换传递函数预设
    void setPreset(TransferPreset preset);

    // 底层渲染器，供 ViewManager 联动控制
    vtkRenderer* renderer() { return m_renderer; }

private:
    QVTKOpenGLNativeWidget* m_vtkWidget = nullptr;
    vtkSmartPointer<vtkGenericOpenGLRenderWindow> m_renderWindow;
    vtkSmartPointer<vtkRenderer>            m_renderer;
    vtkSmartPointer<vtkSmartVolumeMapper>   m_mapper;
    vtkSmartPointer<vtkVolume>              m_volume;
};
