#pragma once

// ============================================================================
// DicomScanner —— DICOM 目录扫描器
//
// 设计模式：门面(Facade)
//   把"递归枚举文件 -> 读头(DicomTagReader) -> 按 SeriesInstanceUID 分组 ->
//   组内按实例号排序 -> 生成首图缩略图"这一整套流程，封装成一次
//   scanDirectory() 调用。
//
// 使用方式：
//   DicomScanner scanner;
//   scanner.setProgressCallback([](int done, int total){ ... });
//   QList<SeriesInfo> list = scanner.scanDirectory(dir);   // 阻塞调用
//
// 线程模型：本类不含 Qt 对象，可在任意线程运行；cancel() 线程安全。
// 取消后 scanDirectory() 返回空列表（与"没有 DICOM"的区分由调用方处理）。
// ============================================================================

#include "SeriesInfo.h"

#include <QList>
#include <QString>

#include <atomic>
#include <functional>

class DicomScanner
{
public:
    // 进度回调：done=已处理文件数, total=候选文件总数
    using ProgressCallback = std::function<void(int done, int total)>;

    void setProgressCallback(ProgressCallback cb) { m_progressCb = std::move(cb); }

    // 请求取消（线程安全）；扫描循环在下一个文件处停下
    void cancel() { m_cancelRequested.store(true); }

    // 扫描目录(递归)，返回按序列号升序排列的序列列表；失败/取消返回空列表
    QList<SeriesInfo> scanDirectory(const QString& dirPath);

private:
    // 收集目录下所有候选文件(跳过明显不可能的)，返回绝对路径列表
    static QStringList collectCandidateFiles(const QString& dirPath);

    // 把单个文件的标签信息归并入分组表
    void absorbFile(const QString& filePath, class DicomTagReader& reader,
                    QList<SeriesInfo>& groups);

    ProgressCallback  m_progressCb;
    std::atomic<bool> m_cancelRequested{false};
};
