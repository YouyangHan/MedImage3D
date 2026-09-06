#pragma once

// ============================================================================
// SeriesInfo —— 一个 DICOM 序列的纯数据结构
//
// 设计说明：
//   - 纯数据（POD 风格），不含任何业务逻辑，充当各层之间传递的"数据载体"。
//   - 由 DicomScanner 填充，ui 层(SeriesListModel/SeriesSelectDialog)读取，
//     后续步骤 VolumeLoader 也会用到其中的 filePaths。
//   - filePaths 已按 InstanceNumber(实例号) 升序排好，可直接按序加载。
// ============================================================================

#include <QImage>
#include <QMetaType>
#include <QSize>
#include <QString>
#include <QStringList>

struct SeriesInfo
{
    QString seriesUID;         // SeriesInstanceUID，序列分组键
    QString seriesDescription; // 序列描述，如 "Head CT"
    int     seriesNumber = 0;  // 序列号（同一检查内编号）
    QString modality;          // 设备类型，如 "CT" / "MR"
    QString patientName;       // 患者姓名
    QString studyDate;         // 检查日期 (YYYYMMDD)
    QSize   imageSize;         // 单张图像尺寸 (宽 x 高)

    QStringList filePaths;     // 该序列全部文件（已按实例号排序）
    QImage      thumbnail;     // 首图缩略图（扫描时生成）

    // 序列包含的图像张数
    int imageCount() const { return filePaths.size(); }
};

// 注册为 Qt 元类型：使 QList<SeriesInfo> 能跨线程通过信号传递(队列连接需拷贝)
Q_DECLARE_METATYPE(SeriesInfo)
