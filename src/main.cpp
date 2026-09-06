#include "MainWindow.h"
#include "common/Logging.h"
#include "common/Settings.h"
#include "dicom/DicomScanner.h"
#include "dicom/SeriesInfo.h"
#include "dicom/TestDataGenerator.h"
#include "resources/paths.h"
#include "resources/strings.h"

#include <QApplication>
#include <QCoreApplication>
#include <QSurfaceFormat>
#include <QVTKOpenGLNativeWidget.h>

#include <QFileInfo>
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

} // namespace

int main(int argc, char* argv[])
{
    // Qt6 + VTK9: 必须在 QApplication 创建之前设置默认 OpenGL SurfaceFormat
    QSurfaceFormat::setDefaultFormat(QVTKOpenGLNativeWidget::defaultFormat());

    QApplication app(argc, argv);
    QApplication::setApplicationName(AppPaths::kAppName);
    QApplication::setOrganizationName(AppPaths::kOrgName);

    // 跨线程信号(ScanThread -> 主窗口)经队列连接传递 QList<SeriesInfo>，
    // 需提前注册元类型，Qt 才能在两个线程间拷贝该参数
    qRegisterMetaType<QList<SeriesInfo>>("QList<SeriesInfo>");

    // ---- 命令行自检开关(无界面) ----
    //   --gen-test-data <目录>   生成测试数据
    //   --scan-smoke-test <目录> 生成数据并扫描自检
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
    }

    LOG_INFO(lcApp, "应用启动: " << AppStrings::kAppDisplayName);
    LOG_DEBUG(lcApp, "上次打开目录: " << Settings::instance().lastDirectory());

    MainWindow w;
    w.resize(1280, 800);
    w.show();

    return app.exec();
}
