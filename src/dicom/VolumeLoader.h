#pragma once

// ============================================================================
// VolumeLoader —— 体数据加载器
//
// 设计模式：门面(Facade)
//   把"ITK GDCMImageIO 读像素 -> ImageSeriesReader 组装三维 ->
//   ImageToVTKImageFilter 桥接 vtkImageData"整条链路，封装成一次 load()。
//
// 与第 2 步分工：头解析/分组/缩略图用 DCMTK(DicomScanner)，像素读取用 ITK GDCM。
//
// 使用方式(阻塞，建议放入 LoadThread 后台执行)：
//   Volume v = VolumeLoader::load(series.filePaths, [](double p){ ... });
// ============================================================================

#include <QStringList>

#include <functional>

class Volume;

class VolumeLoader
{
public:
    // 进度回调：0.0 ~ 1.0(ITK ProgressEvent)
    using ProgressCallback = std::function<void(double progress)>;

    // 从一组 DICOM 文件(同一序列, 已排序)加载为体数据。
    // 失败或空列表返回无效 Volume(isValid()==false)。
    static Volume load(const QStringList& filePaths, ProgressCallback progress = {});
};
