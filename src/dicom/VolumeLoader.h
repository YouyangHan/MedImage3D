#pragma once

// ============================================================================
// VolumeLoader —— 体数据加载器
//
// 设计模式：门面(Facade)
//   把"ITK GDCM 读像素 -> ImageToVTKImageFilter 桥接 vtkImageData"封装。
//
// 关键线程模型（重要）：
//   - loadItk() 只做 ITK 读取(纯 ITK, 线程安全)，可在后台线程调用；
//   - convertToVolume() 做 ITK->VTK 桥接 + DeepCopy(涉及 VTK)，必须在主线程调用。
//   混用(后台线程执行 VTK 桥接)会导致主线程渲染崩溃(VTK 非线程安全)。
// ============================================================================

#include <itkImage.h>
#include <itkSmartPointer.h>

#include <QStringList>

#include <functional>

class Volume;

class VolumeLoader
{
public:
    using ImageType = itk::Image<signed short, 3>;
    using ImagePointer = ImageType::Pointer;

    // 进度回调：0.0 ~ 1.0(ITK ProgressEvent)
    using ProgressCallback = std::function<void(double progress)>;

    // 后台线程：读取 DICOM 序列为 ITK image(纯 ITK, 不碰 VTK, 线程安全)
    static ImagePointer loadItk(const QStringList& filePaths,
                                ProgressCallback progress = {});

    // 主线程：ITK image -> vtkImageData(DeepCopy) -> Volume(涉及 VTK, 须在主线程)
    static Volume convertToVolume(ImagePointer image);

    // 便捷：同步加载(主线程直接调用, 两步合一；测试/简单场景用)
    static Volume load(const QStringList& filePaths, ProgressCallback progress = {});
};
