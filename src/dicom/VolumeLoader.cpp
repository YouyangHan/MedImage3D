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

Volume VolumeLoader::load(const QStringList& filePaths, ProgressCallback progress)
{
    Volume volume;
    if (filePaths.isEmpty()) {
        LOG_WARN(lcDicom, "体数据加载: 文件列表为空");
        return volume;
    }

    // CT 序列存 signed short(HU 值可正可负)，与 DCMTK 头解析约定一致
    using PixelType = signed short;
    using ImageType = itk::Image<PixelType, 3>;
    using ReaderType = itk::ImageSeriesReader<ImageType>;
    using BridgeType = itk::ImageToVTKImageFilter<ImageType>;

    try {
        auto reader = ReaderType::New();
        reader->SetImageIO(itk::GDCMImageIO::New());   // GDCM 解析像素 + spacing/origin

        // 把 QStringList 转成 ITK 需要的 std::vector<std::string>
        std::vector<std::string> names;
        names.reserve(static_cast<size_t>(filePaths.size()));
        for (const QString& p : filePaths)
            names.push_back(p.toStdString());
        reader->SetFileNames(names);

        // 挂进度观察者
        if (progress) {
            auto cmd = LoadProgressCommand::New();
            cmd->callback = progress;
            reader->AddObserver(itk::ProgressEvent(), cmd);
        }

        reader->Update();   // 读取全部切片

        // ITK -> VTK 桥接，得到 vtkImageData
        auto bridge = BridgeType::New();
        bridge->SetInput(reader->GetOutput());
        bridge->Update();

        // ITK 桥接默认用 vtkImageImport 引用 ITK image 的缓冲区(不拷贝)，
        // reader 在 load() 返回后销毁会导致标量数据悬空。这里 DeepCopy 让
        // vtkImageData 拥有独立数据副本(体数据内存翻倍，但消除悬空隐患)。
        vtkNew<vtkImageData> copied;
        copied->DeepCopy(bridge->GetOutput());
        volume.imageData = copied;

        // 提取几何信息(ITK 已根据 DICOM 自动解析)
        const ImageType* img = reader->GetOutput();
        const auto& sp = img->GetSpacing();
        volume.spacing = { sp[0], sp[1], sp[2] };
        const auto& o = img->GetOrigin();
        volume.origin = { o[0], o[1], o[2] };
        const auto& sz = img->GetLargestPossibleRegion().GetSize();
        volume.dimensions = { static_cast<int>(sz[0]),
                              static_cast<int>(sz[1]),
                              static_cast<int>(sz[2]) };

        LOG_INFO(lcDicom, "体数据加载完成: " << sz[0] << 'x' << sz[1] << 'x' << sz[2]
                 << " 间距 " << sp[0] << '/' << sp[1] << '/' << sp[2]);
    }
    catch (const itk::ExceptionObject& e) {
        LOG_ERROR(lcDicom, "ITK 加载失败: " << e.GetDescription());
        volume.reset();
    }

    return volume;
}
