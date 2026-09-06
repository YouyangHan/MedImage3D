#pragma once

// ============================================================================
// TransferFunctionFactory —— 体绘制传递函数工厂
//
// 设计模式：工厂方法(Factory Method) + 策略(Strategy)
//   把"骨窗/软组织/肌肉"三套传递函数(颜色 + 标量不透明度)封装成可替换策略，
//   由工厂方法按预设创建完整的 vtkVolumeProperty。
//   新增预设只需：加枚举 + 工厂分支，调用侧无需改动。
//
// 关键点数据参考 3d-xmake 的 ImageDataNode::setRenderMode。
// ============================================================================

#include <QString>
#include <vtkSmartPointer.h>

class vtkVolumeProperty;

// 传递函数预设
enum class TransferPreset {
    Bone,        // 骨窗
    SoftTissue,  // 软组织窗
    Muscle,      // 肌肉窗
};

class TransferFunctionFactory
{
public:
    // 工厂方法：创建指定预设的 vtkVolumeProperty(颜色传递函数 + 标量不透明度 + 光照)
    static vtkSmartPointer<vtkVolumeProperty> createVolumeProperty(TransferPreset preset);

    // 预设中文名(供 UI 显示)
    static QString presetName(TransferPreset preset);
};
