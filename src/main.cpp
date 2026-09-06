#include "MainWindow.h"
#include "common/Logging.h"
#include "common/Settings.h"
#include "core/Volume.h"
#include "dicom/DicomScanner.h"
#include "dicom/LoadThread.h"
#include "dicom/SeriesInfo.h"
#include "dicom/TestDataGenerator.h"
#include "dicom/VolumeLoader.h"
#include "resources/paths.h"
#include "resources/strings.h"

#include <QApplication>
#include <QCoreApplication>
#include <QSurfaceFormat>
#include <QTimer>
#include <QVTKOpenGLNativeWidget.h>

#include <QTextStream>

namespace {

// 命令行自检：生成合成测试数据后，用 DicomScanner 扫描并打印分组结果，
// 然后直接退出(不进入 GUI)。用于验证"扫描->分组->缩略图"链路。
int runScanSmokeTest(const QString& dirPath)
{
    QString error;
    if (!TestDataGenerator::generate(dirPath, &error)) {
        QTextStream(stdout) << "生成测试数据失败: " << error << '\n';
        return 2;
    }

    DicomScanner scanner;
    const QList<SeriesInfo> series = scanner.scanDirectory(dirPath);

    QTextStream out(stdout);
    out << "扫描到序列数: " << series.size() << '\n';
    for (const SeriesInfo& s : series) {
        out << "  [序列 " << s.seriesNumber << "] " << s.seriesDescription
            << "  张数=" << s.imageCount()
            << "  尺寸=" << s.imageSize.width() << 'x' << s.imageSize.height()
            << "  缩略图=" << (s.thumbnail.isNull() ? "无" : "有") << '\n';
    }
    return series.isEmpty() ? 3 : 0;   // 无结果视为失败
}

// 命令行自检：生成数据 -> 扫描 -> 加载第一个序列的体数据(ITK GDCM)，
// 打印尺寸/间距/中心像素值，验证 DICOM 像素读取链路。
int runLoadSmokeTest(const QString& dirPath)
{
    QString error;
    if (!TestDataGenerator::generate(dirPath, &error)) {
        QTextStream(stdout) << "生成测试数据失败: " << error << '\n';
        return 2;
    }

    DicomScanner scanner;
    const QList<SeriesInfo> series = scanner.scanDirectory(dirPath);
    if (series.isEmpty()) {
        QTextStream(stdout) << "扫描无结果\n";
        return 3;
    }

    const SeriesInfo& s = series.first();
    Volume v = VolumeLoader::load(s.filePaths);
    if (!v.isValid()) {
        QTextStream(stdout) << "体数据加载失败\n";
        return 4;
    }

    QTextStream out(stdout);
    out << "加载成功: " << s.seriesDescription
        << "  尺寸=" << v.dimensions[0] << 'x' << v.dimensions[1] << 'x' << v.dimensions[2]
        << "  间距=" << v.spacing[0] << '/' << v.spacing[1] << '/' << v.spacing[2] << '\n';

    // 打印中心体素值(应约为 HU=0，测试数据背景 rescale 后为 0)
    const int x = v.dimensions[0] / 2, y = v.dimensions[1] / 2, z = v.dimensions[2] / 2;
    const short* px = static_cast<const short*>(v.imageData->GetScalarPointer(x, y, z));
    out << "中心体素值=" << (px ? *px : 0) << '\n';
    return 0;
}

// 命令行自检：走真实异步加载路径(LoadThread -> 队列连接 -> 四视图)，3 秒后退出。
// 用于验证"扫描 -> 加载 -> 四视图渲染"完整链路不崩溃。
int runViewSmokeTest(const QString& dirPath)
{
    QString error;
    if (!TestDataGenerator::generate(dirPath, &error)) {
        QTextStream(stdout) << "生成失败: " << error << '\n';
        return 2;
    }
    DicomScanner scanner;
    const QList<SeriesInfo> series = scanner.scanDirectory(dirPath);
    if (series.isEmpty()) return 3;

    MainWindow w;
    w.resize(1280, 800);
    w.show();

    auto* thread = new LoadThread(series.first().filePaths);
    QObject::connect(thread, &LoadThread::loadFinished, &w, [&w, thread]() {
        // 主线程做 VTK 桥接(涉及 VTK, 必须主线程)
        Volume v = VolumeLoader::convertToVolume(thread->image());
        w.showVolumeForTest(v);
    });
    QObject::connect(thread, &QThread::finished, thread, &QObject::deleteLater);
    thread->start();

    QTimer::singleShot(3000, &QCoreApplication::quit);
    return QCoreApplication::exec();
}

} // namespace

int main(int argc, char* argv[])
{
    // Qt6 + VTK9: 必须在 QApplication 创建之前设置默认 OpenGL SurfaceFormat
    QSurfaceFormat::setDefaultFormat(QVTKOpenGLNativeWidget::defaultFormat());

    QApplication app(argc, argv);
    QApplication::setApplicationName(AppPaths::kAppName);
    QApplication::setOrganizationName(AppPaths::kOrgName);

    // 跨线程信号经队列连接传递自定义类型，需提前注册元类型，Qt 才能拷贝参数：
    //   ScanThread -> QList<SeriesInfo>；LoadThread 主线程取 image，无 Volume 跨线程传递
    qRegisterMetaType<QList<SeriesInfo>>("QList<SeriesInfo>");

    // ---- 命令行自检开关(无界面/冒烟测试) ----
    //   --gen-test-data <目录>    生成测试数据
    //   --scan-smoke-test <目录>  生成数据并扫描自检
    //   --load-smoke-test <目录>  生成数据并加载体数据自检
    //   --view-smoke-test <目录>  异步加载并显示四视图自检
    const QStringList args = QCoreApplication::arguments();
    for (int i = 1; i < args.size(); ++i) {
        if (args[i] == QStringLiteral("--gen-test-data") && i + 1 < args.size()) {
            QString error;
            const bool ok = TestDataGenerator::generate(args[i + 1], &error);
            QTextStream(stdout) << (ok ? "测试数据已生成: " : "失败: ") << args[i + 1]
                                << (ok ? "" : (" " + error)) << '\n';
            return ok ? 0 : 2;
        }
        if (args[i] == QStringLiteral("--scan-smoke-test") && i + 1 < args.size())
            return runScanSmokeTest(args[i + 1]);
        if (args[i] == QStringLiteral("--load-smoke-test") && i + 1 < args.size())
            return runLoadSmokeTest(args[i + 1]);
        if (args[i] == QStringLiteral("--view-smoke-test") && i + 1 < args.size())
            return runViewSmokeTest(args[i + 1]);
    }

    LOG_INFO(lcApp, "应用启动: " << AppStrings::kAppDisplayName);
    LOG_DEBUG(lcApp, "上次打开目录: " << Settings::instance().lastDirectory());

    MainWindow w;
    w.resize(1280, 800);
    w.show();

    return app.exec();
}
