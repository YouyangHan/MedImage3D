#pragma once

// ============================================================================
// SliceImageLoader —— 单张 DICOM 切片 -> QImage 转换器
//
// 设计说明：
//   - 基于 DCMTK 的 DicomImage 类，自动应用文件中记录的窗宽窗位(VOI LUT)，
//     输出 8 位灰度 QImage，用于缩略图与序列预览。
//   - 纯静态函数集合(无状态)，可在任意线程调用(QImage 不依赖 GUI 线程)。
//   - 仅处理单帧灰度图(CT 场景)；多帧/彩色文件返回空图。
// ============================================================================

#include <QImage>
#include <QString>

namespace SliceImageLoader {

// 加载一张 DICOM 切片为 QImage。
//   filePath : DICOM 文件路径
//   maxSize  : >0 时把结果等比缩放到不超过该边长(缩略图用)；0 表示原尺寸
// 返回：成功为有效 QImage；失败(非DICOM/彩色/多帧等)为空 QImage
QImage load(const QString& filePath, int maxSize = 0);

} // namespace SliceImageLoader
