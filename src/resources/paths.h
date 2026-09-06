#pragma once

// ============================================================================
// 集中管理路径 / 目录 / 组织名等常量
//
// 设计说明：
//   - 组织名、应用名、资源路径等"字符串路径类"常量统一放这里，
//     避免在业务代码里写死路径字面量。
//   - 图标资源经 .qrc 打包进可执行文件，运行时用 ":/icons/xxx.svg" 引用。
// ============================================================================

#include <QString>

namespace AppPaths {

// ---- 组织名 / 应用名(QSettings 持久化用) ----
inline const QString kOrgName = QStringLiteral("MedImage3D");
inline const QString kAppName = QStringLiteral("MedImage3D");

// ---- 图标资源前缀(qrc 路径前缀) ----
inline const QString kIconPrefix = QStringLiteral(":/icons/");

// ---- 具体图标文件路径 ----
inline const QString kIconOpenCt      = QStringLiteral(":/icons/action_open_ct.svg");
inline const QString kIconReconstruct = QStringLiteral(":/icons/action_reconstruct.svg");
inline const QString kIconPreset      = QStringLiteral(":/icons/action_preset.svg");
inline const QString kIconReset       = QStringLiteral(":/icons/action_reset.svg");

} // namespace AppPaths
