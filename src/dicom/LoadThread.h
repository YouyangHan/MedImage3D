#pragma once

// ============================================================================
// LoadThread —— 体数据后台加载线程
//
// 设计说明：
//   继承 QThread，在后台执行 VolumeLoader(阻塞式 ITK 读取)，避免界面卡死。
//   进度与结果通过 Qt 信号发回 GUI 线程(自动队列连接, 线程安全)。
//
// 用法(见 MainWindow::startLoad)：
//   auto* t = new LoadThread(filePaths, this);
//   connect(t, &LoadThread::progressChanged, ...);
//   connect(t, &LoadThread::loadFinished, ...);
//   t->start();
// ============================================================================

#include "VolumeLoader.h"
#include "core/Volume.h"

#include <QStringList>
#include <QThread>

class LoadThread : public QThread
{
    Q_OBJECT

public:
    explicit LoadThread(QStringList filePaths, QObject* parent = nullptr)
        : QThread(parent), m_filePaths(std::move(filePaths))
    {
    }

signals:
    // 进度 0.0 ~ 1.0(从后台线程发出，队列连接到 GUI 线程)
    void progressChanged(double progress);
    // 加载完成：体数据(失败时为无效 Volume)
    void loadFinished(const Volume& volume);

protected:
    void run() override
    {
        const Volume volume = VolumeLoader::load(m_filePaths,
            [this](double p) { emit progressChanged(p); });
        emit loadFinished(volume);
    }

private:
    QStringList m_filePaths;
};
