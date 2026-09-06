#include "DicomScanner.h"

#include "DicomTagReader.h"
#include "SliceImageLoader.h"
#include "common/Logging.h"

#include <QDir>
#include <QDirIterator>
#include <QFileInfo>

#include <algorithm>

namespace {

// 明显不是 DICOM 的扩展名，直接跳过（DICOM 文件经常没有扩展名，所以不能用白名单）
const QSet<QString>& skippedExtensions()
{
    static const QSet<QString> kSkipped = {
        QStringLiteral("exe"),  QStringLiteral("dll"), QStringLiteral("txt"),
        QStringLiteral("md"),   QStringLiteral("log"), QStringLiteral("json"),
        QStringLiteral("xml"),  QStringLiteral("zip"), QStringLiteral("pdf"),
        QStringLiteral("html"), QStringLiteral("htm"), QStringLiteral("lnk"),
        QStringLiteral("bat"),  QStringLiteral("db"),
    };
    return kSkipped;
}

// 缩略图最大边长(像素)
constexpr int kThumbnailSize = 96;

} // namespace

QStringList DicomScanner::collectCandidateFiles(const QString& dirPath)
{
    QStringList files;
    // 递归遍历；跳过子目录本身、符号链接、过小的文件(DICOM 头部都不止几百字节)
    QDirIterator it(dirPath, QDir::Files | QDir::NoDotAndDotDot | QDir::NoSymLinks,
                    QDirIterator::Subdirectories);
    while (it.hasNext()) {
        const QString path = it.next();
        const QFileInfo info(path);
        if (info.size() < 256)
            continue;
        if (skippedExtensions().contains(info.suffix().toLower()))
            continue;
        files << path;
    }
    return files;
}

void DicomScanner::absorbFile(const QString& filePath, DicomTagReader& reader,
                              QList<SeriesInfo>& groups)
{
    const QString uid = reader.seriesInstanceUID();
    if (uid.isEmpty())
        return;   // 没有序列 UID 无法分组，跳过

    // 线性查找分组（同一目录下序列数量通常 < 100，无需哈希表）
    auto it = std::find_if(groups.begin(), groups.end(),
                           [&uid](const SeriesInfo& s) { return s.seriesUID == uid; });
    if (it == groups.end()) {
        SeriesInfo info;
        info.seriesUID = uid;
        groups << info;
        it = std::prev(groups.end());
    }

    // 首次遇到该序列时记录公共信息；各文件间这些字段理应一致
    SeriesInfo& group = *it;
    if (group.seriesDescription.isEmpty()) {
        group.seriesDescription = reader.seriesDescription();
        group.seriesNumber      = reader.seriesNumber();
        group.modality          = reader.modality();
        group.patientName       = reader.patientName();
        group.studyDate         = reader.studyDate();
        group.imageSize         = reader.imageSize();
    }

    // 文件路径与排序键一起暂存：用 "实例号|层面位置|路径" 编码进列表，
    // 扫描完成后统一排序再剥离，避免额外的平行数组
    const QString sortKey = QStringLiteral("%1|%2|%3")
        .arg(reader.instanceNumber(), 8, 10, QLatin1Char('0'))
        .arg(reader.sliceLocation(), 12, 'f', 4, QLatin1Char('0'))
        .arg(filePath);
    group.filePaths << sortKey;
}

QList<SeriesInfo> DicomScanner::scanDirectory(const QString& dirPath)
{
    m_cancelRequested.store(false);

    const QStringList files = collectCandidateFiles(dirPath);
    LOG_INFO(lcDicom, "扫描目录: " << dirPath << " 候选文件 " << files.size() << " 个");
    if (files.isEmpty())
        return {};

    QList<SeriesInfo> groups;
    DicomTagReader reader;
    int done = 0;

    for (const QString& path : files) {
        if (m_cancelRequested.load()) {
            LOG_INFO(lcDicom, "扫描已取消");
            return {};
        }
        if (reader.open(path))
            absorbFile(path, reader, groups);
        ++done;
        if (m_progressCb)
            m_progressCb(done, files.size());
    }

    // ---- 组内排序：剥离排序键，得到纯路径列表 ----
    for (SeriesInfo& group : groups) {
        group.filePaths.sort();   // 键已固定位宽，字典序 == 数值序
        for (QString& p : group.filePaths)
            p = p.section(QLatin1Char('|'), 2);   // 去掉 "实例号|层面位置|" 前缀

        // 首图缩略图
        if (!group.filePaths.isEmpty())
            group.thumbnail = SliceImageLoader::load(group.filePaths.first(), kThumbnailSize);
    }

    // ---- 序列间排序：序列号升序（相同则按描述，保证结果稳定）----
    std::sort(groups.begin(), groups.end(),
              [](const SeriesInfo& a, const SeriesInfo& b) {
                  if (a.seriesNumber != b.seriesNumber)
                      return a.seriesNumber < b.seriesNumber;
                  return a.seriesDescription < b.seriesDescription;
              });

    LOG_INFO(lcDicom, "扫描完成: 共 " << groups.size() << " 个序列");
    return groups;
}
