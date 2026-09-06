#include "MainWindow.h"
#include "common/Logging.h"
#include "common/Settings.h"
#include "resources/paths.h"
#include "resources/strings.h"

#include <QApplication>
#include <QSurfaceFormat>
#include <QVTKOpenGLNativeWidget.h>

int main(int argc, char* argv[])
{
    // Qt6 + VTK9: 必须在 QApplication 创建之前设置默认 OpenGL SurfaceFormat
    QSurfaceFormat::setDefaultFormat(QVTKOpenGLNativeWidget::defaultFormat());

    QApplication app(argc, argv);
    // 组织名/应用名统一取自 AppPaths 常量, 不写死字符串
    QApplication::setApplicationName(AppPaths::kAppName);
    QApplication::setOrganizationName(AppPaths::kOrgName);

    // 记录启动日志(验证 Logging 与 AppStrings 基础设施可用)
    LOG_INFO(lcApp, "应用启动: " << AppStrings::kAppDisplayName);
    // 读取上次打开目录(验证 Settings 单例 + QSettings 持久化可用)
    LOG_DEBUG(lcApp, "上次打开目录: " << Settings::instance().lastDirectory());

    MainWindow w;
    w.resize(1280, 800);
    w.show();

    return app.exec();
}
