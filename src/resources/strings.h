#pragma once

// ============================================================================
// 集中管理所有中文 UI 文案
//
// 设计说明：
//   - 所有按钮文字 / ToolTip / 对话框标题 / 进度提示 / 错误消息都集中定义在此，
//     业务代码通过 AppStrings::kXxx 引用，禁止在业务代码里散落字符串字面量。
//   - 采用 C++17 inline 变量，多个编译单元共享同一实例，无重复定义问题。
//   - 后续若需国际化，只需在此切换到 QObject::tr() 翻译机制，调用侧无需改动。
// ============================================================================

#include <QString>

namespace AppStrings {

// ---- 应用信息 ----
inline const QString kAppDisplayName = QStringLiteral("医学图像三维重建");

// ---- 工具栏按钮文字 ----
inline const QString kActionOpenCt       = QStringLiteral("选择 CT 序列目录");
inline const QString kActionReconstruct  = QStringLiteral("三维重建");
inline const QString kActionPreset       = QStringLiteral("窗宽窗位预设");
inline const QString kActionReset        = QStringLiteral("重置视图");

// ---- 状态栏提示(StatusTip) ----
inline const QString kActionOpenCtStatus      = QStringLiteral("选择包含 DICOM 文件的目录，扫描并分组序列");
inline const QString kActionReconstructStatus = QStringLiteral("对选中序列进行三维重建");
inline const QString kActionPresetStatus      = QStringLiteral("切换骨窗 / 软组织 / 肌肉传递函数");
inline const QString kActionResetStatus       = QStringLiteral("恢复默认视角与窗宽窗位");

// ---- 进度提示 ----
inline const QString kProgressScan = QStringLiteral("正在扫描 DICOM 序列…");
inline const QString kProgressLoad = QStringLiteral("正在重建三维体数据…");

// ---- 对话框标题 ----
inline const QString kDialogTitleSeries = QStringLiteral("选择序列");

// ---- 通用按钮 ----
inline const QString kButtonOk     = QStringLiteral("确定");
inline const QString kButtonCancel = QStringLiteral("取消");
inline const QString kButtonReconstruct = QStringLiteral("三维重建");

// ---- 序列信息标签 ----
inline const QString kLabelPatientName = QStringLiteral("患者");
inline const QString kLabelStudyDate   = QStringLiteral("检查日期");
inline const QString kLabelSeries      = QStringLiteral("序列");
inline const QString kLabelImageCount  = QStringLiteral("张");

// ---- 错误提示 ----
inline const QString kErrNoDicomFiles = QStringLiteral("所选目录中没有找到 DICOM 文件");
inline const QString kErrLoadFailed   = QStringLiteral("体数据加载失败");

} // namespace AppStrings
