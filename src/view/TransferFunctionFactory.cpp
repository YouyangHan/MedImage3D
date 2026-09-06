#include "TransferFunctionFactory.h"

#include <vtkColorTransferFunction.h>
#include <vtkNew.h>
#include <vtkPiecewiseFunction.h>
#include <vtkVolumeProperty.h>

namespace {

// 设置骨窗预设的关键点
void setupBone(vtkColorTransferFunction* color, vtkPiecewiseFunction* opacity,
               vtkVolumeProperty* prop)
{
    color->AddRGBPoint(-3024, 0.0, 0.0, 0.0);
    color->AddRGBPoint(-16,   0.73, 0.25, 0.30);
    color->AddRGBPoint(641,   0.90, 0.82, 0.56);
    color->AddRGBPoint(3071,  1.0,  1.0,  1.0);

    opacity->AddPoint(-3024, 0.0);
    opacity->AddPoint(-16,   0.0);
    opacity->AddPoint(641,   0.72);
    opacity->AddPoint(3071,  0.71);

    prop->ShadeOn();
    prop->SetAmbient(0.1);
    prop->SetDiffuse(0.9);
    prop->SetScalarOpacityUnitDistance(0.8919);
}

// 设置软组织窗预设的关键点
void setupSoftTissue(vtkColorTransferFunction* color, vtkPiecewiseFunction* opacity,
                     vtkVolumeProperty* prop)
{
    color->AddRGBPoint(0,    0.0, 0.0, 0.0);
    color->AddRGBPoint(500,  1.0, 0.5, 0.3);
    color->AddRGBPoint(1000, 1.0, 0.5, 0.3);
    color->AddRGBPoint(1150, 1.0, 1.0, 0.9);

    opacity->AddPoint(0,    0.0);
    opacity->AddPoint(500,  0.15);
    opacity->AddPoint(1000, 0.15);
    opacity->AddPoint(1150, 0.85);

    prop->ShadeOn();
    prop->SetAmbient(0.4);
    prop->SetDiffuse(0.6);
}

// 设置肌肉窗预设的关键点
void setupMuscle(vtkColorTransferFunction* color, vtkPiecewiseFunction* opacity,
                 vtkVolumeProperty* prop)
{
    color->AddRGBPoint(-3024, 0.0,  0.0,  0.0);
    color->AddRGBPoint(-155,  0.55, 0.25, 0.15);
    color->AddRGBPoint(217,   0.88, 0.60, 0.29);
    color->AddRGBPoint(420,   1.0,  0.94, 0.95);
    color->AddRGBPoint(3071,  0.83, 0.66, 1.0);

    opacity->AddPoint(-3024, 0.0);
    opacity->AddPoint(-155,  0.0);
    opacity->AddPoint(217,   0.68);
    opacity->AddPoint(420,   0.83);
    opacity->AddPoint(3071,  0.80);

    prop->ShadeOn();
    prop->SetAmbient(0.1);
    prop->SetDiffuse(0.9);
    prop->SetScalarOpacityUnitDistance(0.8919);
}

} // namespace

vtkSmartPointer<vtkVolumeProperty> TransferFunctionFactory::createVolumeProperty(TransferPreset preset)
{
    // 颜色传递函数(标量 -> RGB) 与 标量不透明度传递函数(标量 -> 不透明度)
    vtkNew<vtkColorTransferFunction> color;
    vtkNew<vtkPiecewiseFunction>     opacity;

    vtkNew<vtkVolumeProperty> prop;
    prop->SetColor(color);
    prop->SetScalarOpacity(opacity);
    prop->SetInterpolationTypeToLinear();

    // 策略分支：按预设填充关键点
    switch (preset) {
    case TransferPreset::Bone:
        setupBone(color, opacity, prop);
        break;
    case TransferPreset::SoftTissue:
        setupSoftTissue(color, opacity, prop);
        break;
    case TransferPreset::Muscle:
        setupMuscle(color, opacity, prop);
        break;
    }

    return prop;
}

QString TransferFunctionFactory::presetName(TransferPreset preset)
{
    switch (preset) {
    case TransferPreset::Bone:       return QStringLiteral("骨窗");
    case TransferPreset::SoftTissue: return QStringLiteral("软组织");
    case TransferPreset::Muscle:     return QStringLiteral("肌肉");
    }
    return {};
}
