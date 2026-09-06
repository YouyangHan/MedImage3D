#pragma once

// ============================================================================
// ScanThread —— 在后台线程执行 DicomScanner
//
// 设计说明：
//   - 继承 QThread，把阻塞式扫描放入后台，避免界面卡死；
//   - 进度与结果通过 Qt 信号发回 GUI 线程(自动队列连接, 线程安全)；
//   - cancel() 仅置标志位，扫描循环自行检查并尽快退出(协作式取消)。
//
// 用法（详见 MainWindow::onOpenCtDirectory）：
//   auto* t = new ScanThread(dir, this);
//   connect(t, &ScanThread::progressChanged, ...);
//   connect(t, &ScanThread::scanFinished, ...);
//   t->start();
// ============================================================================

#include "DicomScanner.h"

#include <QThread>

class ScanThread : public QThread
{
    Q_OBJECT

public:
    explicit ScanThread(QString dirPath, QObject* parent = nullptr)
        : QThread(parent), m_dirPath(std::move(dirPath))
    {
    }

    // 请求取消扫描(线程安全)
    void cancel() { m_scanner.cancel(); }

signals:
    // 进度：done/total（从后台线程发出，Qt 自动排队到 GUI 线程）
    void progressChanged(int done, int total);
    // 扫描结束：分组结果(取消或无 DICOM 时为空列表)
    void scanFinished(const QList<SeriesInfo>& series);

protected:
    void run() override
    {
        m_scanner.setProgressCallback(
            [this](int done, int total) { emit progressChanged(done, total); });
        const QList<SeriesInfo> result = m_scanner.scanDirectory(m_dirPath);
        emit scanFinished(result);
    }

private:
    QString      m_dirPath;
    DicomScanner m_scanner;
};
