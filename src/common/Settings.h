#pragma once

// ============================================================================
// 应用配置封装 (门面 Facade + 单例 Singleton)
//
// 统一封装 QSettings，集中管理配置 key 与默认值。
// 业务代码通过 Settings::instance().lastDirectory() 读写，不直接接触 QSettings。
//
// 配置文件位置：由 Qt 按 UserScope + IniFormat 自动定位到用户目录，
//              不硬编码路径(延续"相对路径 + 环境变量"约定)。
// ============================================================================

#include "Singleton.h"

#include <QSettings>
#include <QString>

class Settings : public Singleton<Settings>
{
    friend class Singleton<Settings>;   // 允许基类访问私有构造

public:
    // 最近一次打开的 DICOM 目录(空串表示无)
    QString lastDirectory() const;
    void setLastDirectory(const QString& dir);

    // 是否记住上次打开的目录(默认 true)
    bool rememberLastDirectory() const;
    void setRememberLastDirectory(bool remember);

private:
    Settings();   // 私有构造, 只能经 Singleton::instance() 获取

    QSettings m_settings;
};
