#include "VolumeLoader.h"

#include "common/Logging.h"
#include "core/Volume.h"

#include <itkCommand.h>
#include <itkGDCMImageIO.h>
#include <itkImageSeriesReader.h>
#include <itkImageToVTKImageFilter.h>
#include <itkProcessObject.h>

#include <vtkImageData.h>
#include <vtkNew.h>

#include <string>
#include <vector>

namespace {

// ============================================================================
// ITK 进度观察者：把 reader 的 ProgressEvent 转发为 0..1 回调
// (仅本文件使用，通过回调驱动 LoadThread 的进度信号)
// ============================================================================
class LoadProgressCommand : public itk::Command
{
public:
    using Self = LoadProgressCommand;
    using Pointer = itk::SmartPointer<Self>;
    itkNewMacro(Self);

    std::function<void(double)> callback;   // 进度 0..1

    void Execute(itk::Object* caller, const itk::EventObject& event) override
    {
        report(caller, event);
    }

    void Execute(const itk::Object* caller, const itk::EventObject& event) override
    {
        report(const_cast<itk::Object*>(caller), event);
    }

private:
    void report(itk::Object* caller, const itk::EventObject& event)
    {
        if (!callback || !itk::ProgressEvent().CheckEvent(&event))
            return;
        // 观察的对象是 ImageSeriesReader(ProcessObject)，取其整体进度
        auto* po = dynamic_cast<itk::ProcessObject*>(caller);
        if (po)
            callback(po->GetProgress());
    }
};

} // namespace

VolumeLoader::ImagePointer VolumeLoader::loadItk(const QStringList& filePaths,
                                                 ProgressCallback progress)
{
    if (filePaths.isEmpty()) {
        LOG_WARN(lcDicom, "体数据加载: 文件列表为空");
        return nullptr;
    }

    using ReaderType = itk::ImageSeriesReader<ImageType>;

    try {
        auto reader = ReaderType::New();
        reader->SetImageIO(itk::GDCMImageIO::New());   // GDCM 解析像素 + spacing/origin

        std::vector<std::string> names;
        names.reserve(static_cast<size_t>(filePaths.size()));
        for (const QString& p : filePaths)
            names.push_back(p.toStdString());
        reader->SetFileNames(names);

        if (progress) {
            auto cmd = LoadProgressCommand::New();
            cmd->callback = progress;
            reader->AddObserver(itk::ProgressEvent(), cmd);
        }

        reader->Update();   // 读取全部切片(纯 ITK, 线程安全)

        return reader->GetOutput();
    }
    catch (const itk::ExceptionObject& e) {
        LOG_ERROR(lcDicom, "ITK 加载失败: " << e.GetDescription());
        return nullptr;
    }
}

Volume VolumeLoader::convertToVolume(ImagePointer image)
{
    Volume volume;
    if (!image) {
        LOG_WARN(lcDicom, "convertToVolume: 输入 image 为空");
        return volume;
    }

    using BridgeType = itk::ImageToVTKImageFilter<ImageType>;

    try {
        // ITK -> VTK 桥接(须在主线程)
        auto bridge = BridgeType::New();
        bridge->SetInput(image);
        bridge->Update();

        // ITK 桥接默认引用 ITK 缓冲区(不拷贝)，这里 DeepCopy 让 vtkImageData
        // 拥有独立数据副本(消除悬空隐患)。
        vtkNew<vtkImageData> copied;
        copied->DeepCopy(bridge->GetOutput());
        volume.imageData = copied;

        // 提取几何信息(ITK 已根据 DICOM 自动解析)
        const auto& sp = image->GetSpacing();
        volume.spacing = { sp[0], sp[1], sp[2] };
        const auto& o = image->GetOrigin();
        volume.origin = { o[0], o[1], o[2] };
        const auto& sz = image->GetLargestPossibleRegion().GetSize();
        volume.dimensions = { static_cast<int>(sz[0]),
                              static_cast<int>(sz[1]),
                              static_cast<int>(sz[2]) };

        LOG_INFO(lcDicom, "体数据加载完成: " << sz[0] << 'x' << sz[1] << 'x' << sz[2]
                 << " 间距 " << sp[0] << '/' << sp[1] << '/' << sp[2]);
    }
    catch (const itk::ExceptionObject& e) {
        LOG_ERROR(lcDicom, "ITK->VTK 桥接失败: " << e.GetDescription());
        volume.reset();
    }

    return volume;
}

Volume VolumeLoader::load(const QStringList& filePaths, ProgressCallback progress)
{
    // 同步：加载 + 桥接(主线程内完成)
    return convertToVolume(loadItk(filePaths, progress));
}
