#include "Settings.h"

#include "resources/paths.h"

// ============================================================================
// 配置 key 常量仅在本文件使用, 不暴露给业务代码,
// 业务代码只通过 Settings 的接口访问, 体现门面模式封装。
// ============================================================================
namespace {
const char* kKeyLastDirectory = "io/lastDirectory";
const char* kKeyRememberLast  = "io/rememberLastDirectory";
}

Settings::Settings()
    // IniFormat: 以 .ini 文件持久化; UserScope: 存到当前用户目录(由 Qt 定位)
    : m_settings(QSettings::IniFormat, QSettings::UserScope,
                 AppPaths::kOrgName, AppPaths::kAppName)
{
}

QString Settings::lastDirectory() const
{
    return m_settings.value(kKeyLastDirectory).toString();
}

void Settings::setLastDirectory(const QString& dir)
{
    m_settings.setValue(kKeyLastDirectory, dir);
}

bool Settings::rememberLastDirectory() const
{
    // 第二个参数为默认值：首次运行时返回 true
    return m_settings.value(kKeyRememberLast, true).toBool();
}

void Settings::setRememberLastDirectory(bool remember)
{
    m_settings.setValue(kKeyRememberLast, remember);
}
