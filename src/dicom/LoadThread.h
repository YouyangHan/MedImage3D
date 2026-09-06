#pragma once

// ============================================================================
// LoadThread —— 体数据后台加载线程
//
// 设计说明：
//   - 后台线程只调用 VolumeLoader::loadItk(纯 ITK, 线程安全)，不碰 VTK；
//   - 完成后 emit loadFinished()(无参)，主线程从 image() 取 ITK image，
//     再调用 VolumeLoader::convertToVolume(涉及 VTK, 必须在主线程)。
//   这样避免"后台线程执行 VTK 桥接导致主线程渲染崩溃"的线程安全问题。
//
// 用法(见 MainWindow::startLoad)：
//   auto* t = new LoadThread(filePaths, this);
//   connect(t, &LoadThread::progressChanged, ...);
//   connect(t, &LoadThread::loadFinished, ...);   // 无参
//   t->start();
// ============================================================================

#include "VolumeLoader.h"

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

    // 主线程在 loadFinished() 之后调用：取后台线程加载好的 ITK image
    VolumeLoader::ImagePointer image() const { return m_image; }

signals:
    // 进度 0.0 ~ 1.0(从后台线程发出，队列连接到 GUI 线程)
    void progressChanged(double progress);
    // 加载完成(无参)：主线程从 image() 取结果并 convertToVolume
    void loadFinished();

protected:
    void run() override
    {
        m_image = VolumeLoader::loadItk(m_filePaths,
            [this](double p) { emit progressChanged(p); });
        emit loadFinished();
    }

private:
    QStringList m_filePaths;
    VolumeLoader::ImagePointer m_image;
};
