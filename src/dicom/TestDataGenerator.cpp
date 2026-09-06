#include "TestDataGenerator.h"

#include "common/Logging.h"

#include <dcmtk/dcmdata/dcdatset.h>
#include <dcmtk/dcmdata/dcdeftag.h>   // DCM_XXX 标签常量
#include <dcmtk/dcmdata/dcfilefo.h>
#include <dcmtk/dcmdata/dctagkey.h>
#include <dcmtk/dcmdata/dcuid.h>
#include <dcmtk/ofstd/ofcond.h>

#include <QDir>

#include <cmath>
#include <vector>

namespace {

// 单张切片参数 -> 写出一个合法 CT DICOM 文件
bool writeOneSlice(const QString& filePath,
                   const char* studyUID, const char* seriesUID,
                   int seriesNumber, const char* seriesDesc,
                   int instanceNumber, double zPos,
                   int rows, int cols)
{
    char uid[128];
    DcmFileFormat file;
    DcmDataset* ds = file.getDataset();

    // ---- SOP / 标识 ----
    ds->putAndInsertString(DCM_SOPClassUID, UID_CTImageStorage);
    dcmGenerateUniqueIdentifier(uid, SITE_INSTANCE_UID_ROOT);
    ds->putAndInsertString(DCM_SOPInstanceUID, uid);
    ds->putAndInsertString(DCM_StudyInstanceUID, studyUID);
    ds->putAndInsertString(DCM_SeriesInstanceUID, seriesUID);
    dcmGenerateUniqueIdentifier(uid, SITE_INSTANCE_UID_ROOT);
    ds->putAndInsertString(DCM_FrameOfReferenceUID, uid);

    // ---- 患者/检查信息 ----
    ds->putAndInsertString(DCM_PatientName, "Test^Patient");
    ds->putAndInsertString(DCM_PatientID, "TEST001");
    ds->putAndInsertString(DCM_StudyDate, "20260101");
    ds->putAndInsertString(DCM_StudyTime, "120000");
    ds->putAndInsertString(DCM_AccessionNumber, "ACC001");
    ds->putAndInsertString(DCM_StudyID, "1");
    ds->putAndInsertString(DCM_Manufacturer, "MedImage3D");

    // ---- 序列信息 ----
    ds->putAndInsertString(DCM_Modality, "CT");
    ds->putAndInsertString(DCM_SeriesNumber, QString::number(seriesNumber).toUtf8().constData());
    ds->putAndInsertString(DCM_SeriesDescription, seriesDesc);
    ds->putAndInsertString(DCM_InstanceNumber, QString::number(instanceNumber).toUtf8().constData());

    // ---- 几何信息 ----
    const QString ipp = QStringLiteral("0\\0\\%1").arg(zPos);
    ds->putAndInsertString(DCM_ImagePositionPatient, ipp.toUtf8().constData());
    ds->putAndInsertString(DCM_ImageOrientationPatient, "1\\0\\0\\0\\1\\0");
    ds->putAndInsertString(DCM_PixelSpacing, "1.0\\1.0");
    ds->putAndInsertString(DCM_SliceThickness, "2.5");
    ds->putAndInsertString(DCM_SliceLocation, QString::number(zPos).toUtf8().constData());

    // ---- 像素描述(16 位单通道) ----
    ds->putAndInsertUint16(DCM_Rows, static_cast<Uint16>(rows));
    ds->putAndInsertUint16(DCM_Columns, static_cast<Uint16>(cols));
    ds->putAndInsertUint16(DCM_BitsAllocated, 16);
    ds->putAndInsertUint16(DCM_BitsStored, 16);
    ds->putAndInsertUint16(DCM_HighBit, 15);
    ds->putAndInsertUint16(DCM_PixelRepresentation, 0);  // 无符号
    ds->putAndInsertUint16(DCM_SamplesPerPixel, 1);
    ds->putAndInsertString(DCM_PhotometricInterpretation, "MONOCHROME2");

    // ---- HU 转换与窗宽窗位 ----
    ds->putAndInsertString(DCM_RescaleIntercept, "-1024");
    ds->putAndInsertString(DCM_RescaleSlope, "1");
    ds->putAndInsertString(DCM_RescaleType, "HU");
    ds->putAndInsertString(DCM_WindowCenter, "40");
    ds->putAndInsertString(DCM_WindowWidth, "400");

    // ---- 像素内容：背景 1024(HU=0) + 随层移动的亮圆(便于肉眼验证排序) ----
    std::vector<Uint16> pixels(static_cast<size_t>(rows) * cols, 1024);
    const double cx = cols / 2.0 + instanceNumber * 2.0;   // 圆心随层号右移
    const double cy = rows / 2.0;
    const double radius = rows / 6.0;
    for (int y = 0; y < rows; ++y) {
        for (int x = 0; x < cols; ++x) {
            const double dx = x - cx, dy = y - cy;
            if (dx * dx + dy * dy < radius * radius)
                pixels[static_cast<size_t>(y) * cols + x] = 1500;  // HU≈476
        }
    }
    ds->putAndInsertUint16Array(DCM_PixelData, pixels.data(),
                                static_cast<unsigned long>(pixels.size()));

    const OFCondition status =
        file.saveFile(filePath.toUtf8().constData(), EXS_LittleEndianExplicit);
    if (status.bad()) {
        LOG_ERROR(lcDicom, "写出失败: " << filePath << " : " << status.text());
        return false;
    }
    return true;
}

// 生成一套序列：sliceCount 张切片，层间距 2.5mm
bool makeSeries(const QString& dirPath, const char* studyUID,
                int seriesNumber, const char* desc, int sliceCount,
                QString* errorMsg)
{
    char seriesUidBuf[128];
    dcmGenerateUniqueIdentifier(seriesUidBuf, SITE_INSTANCE_UID_ROOT);

    for (int i = 1; i <= sliceCount; ++i) {
        const QString path = QStringLiteral("%1/S%2_%3.dcm")
            .arg(dirPath).arg(seriesNumber).arg(i, 3, 10, QLatin1Char('0'));
        if (!writeOneSlice(path, studyUID, seriesUidBuf, seriesNumber, desc,
                           i, (i - 1) * 2.5, 64, 64)) {
            if (errorMsg) *errorMsg = QStringLiteral("写出文件失败: %1").arg(path);
            return false;
        }
    }
    return true;
}

} // namespace

namespace TestDataGenerator {

bool generate(const QString& dirPath, QString* errorMsg)
{
    QDir dir;
    if (!dir.mkpath(dirPath)) {
        if (errorMsg) *errorMsg = QStringLiteral("无法创建目录: %1").arg(dirPath);
        return false;
    }

    char studyUid[128];
    dcmGenerateUniqueIdentifier(studyUid, SITE_INSTANCE_UID_ROOT);

    // 两套序列：12 层 "Head CT"(序列号1) 与 8 层 "Chest CT"(序列号2)
    if (!makeSeries(dirPath, studyUid, 1, "Head CT", 12, errorMsg))
        return false;
    if (!makeSeries(dirPath, studyUid, 2, "Chest CT", 8, errorMsg))
        return false;

    LOG_INFO(lcDicom, "测试数据生成完成: " << dirPath << " (2 序列, 共 20 张)");
    return true;
}

} // namespace TestDataGenerator
