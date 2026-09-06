#include "DicomTagReader.h"

#include <dcmtk/dcmdata/dcdatset.h>
#include <dcmtk/dcmdata/dcdeftag.h>   // DCM_XXX 标签常量
#include <dcmtk/dcmdata/dcfilefo.h>
#include <dcmtk/dcmdata/dctagkey.h>
#include <dcmtk/ofstd/ofcond.h>
#include <dcmtk/ofstd/ofstd.h>

// 构造/析构在 .cpp 定义：此处 DcmFileFormat 已是完整类型，unique_ptr 可安全 delete
DicomTagReader::DicomTagReader() = default;
DicomTagReader::~DicomTagReader() = default;

// ============================================================================
// 本文件内部工具函数：从 DcmDataset 读取字符串/整数/浮点标签
// 仅在本编译单元使用(匿名命名空间)，业务代码只通过 DicomTagReader 接口访问。
// ============================================================================
namespace {

// 读取字符串标签；标签缺失时返回空串
QString tagString(DcmDataset* ds, const DcmTagKey& key)
{
    if (!ds) return {};
    OFString value;
    if (ds->findAndGetOFString(key, value).good())
        return QString::fromUtf8(value.c_str()).trimmed();
    return {};
}

// 读取整数标签；缺失或解析失败时返回 fallback
int tagInt(DcmDataset* ds, const DcmTagKey& key, int fallback = 0)
{
    bool ok = false;
    const int v = tagString(ds, key).toInt(&ok);
    return ok ? v : fallback;
}

// 读取浮点标签；缺失或解析失败时返回 fallback
double tagDouble(DcmDataset* ds, const DcmTagKey& key, double fallback = 0.0)
{
    bool ok = false;
    const double v = tagString(ds, key).toDouble(&ok);
    return ok ? v : fallback;
}

} // namespace

// 头标签区通常只有几 KB，限制最大读取长度可避免把整个像素数据读进内存，
// 对目录里混有的大文件(非 DICOM)也能快速失败。
static constexpr Uint32 kMaxHeaderReadBytes = 64 * 1024;

bool DicomTagReader::open(const QString& filePath)
{
    auto file = std::make_unique<DcmFileFormat>();
    const OFCondition status = file->loadFile(
        filePath.toUtf8().constData(),
        EXS_Unknown,            // 自动识别传输语法
        EGL_withoutGL,          // 不需要组长度
        kMaxHeaderReadBytes,    // 只读头部
        ERM_autoDetect);

    if (status.bad() || !file->getDataset())
        return false;           // 非 DICOM 或文件损坏

    m_fileFormat = std::move(file);
    m_dataset = m_fileFormat->getDataset();
    return true;
}

QString DicomTagReader::seriesInstanceUID() const { return tagString(m_dataset, DCM_SeriesInstanceUID); }
QString DicomTagReader::seriesDescription() const { return tagString(m_dataset, DCM_SeriesDescription); }
int     DicomTagReader::seriesNumber() const      { return tagInt(m_dataset, DCM_SeriesNumber); }
int     DicomTagReader::instanceNumber() const    { return tagInt(m_dataset, DCM_InstanceNumber); }
QString DicomTagReader::modality() const          { return tagString(m_dataset, DCM_Modality); }
QString DicomTagReader::patientName() const       { return tagString(m_dataset, DCM_PatientName); }
QString DicomTagReader::studyDate() const         { return tagString(m_dataset, DCM_StudyDate); }
double  DicomTagReader::sliceLocation() const     { return tagDouble(m_dataset, DCM_SliceLocation); }

QSize DicomTagReader::imageSize() const
{
    // DICOM 中 Rows=高, Columns=宽
    const int rows = tagInt(m_dataset, DCM_Rows);
    const int cols = tagInt(m_dataset, DCM_Columns);
    return (rows > 0 && cols > 0) ? QSize(cols, rows) : QSize();
}
