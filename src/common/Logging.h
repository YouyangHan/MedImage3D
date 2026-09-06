#pragma once

// ============================================================================
// 日志封装
//
// 基于 Qt 官方 QLoggingCategory，按模块划分日志类别，便于按类别过滤。
// 提供便捷宏，简化调用：
//   LOG_INFO(lcApp, "加载完成, 共 " << count << " 张");
//
// 设计说明：不引入第三方日志库(easylogging++ 等)，直接用 Qt 自带能力，
//           满足"通俗易懂、依赖最小"。
// ============================================================================

#include <QLoggingCategory>

// ---- 声明日志类别(按模块划分) ----
Q_DECLARE_LOGGING_CATEGORY(lcApp)      // 应用 / 工作流
Q_DECLARE_LOGGING_CATEGORY(lcDicom)    // DICOM 解析
Q_DECLARE_LOGGING_CATEGORY(lcRender)   // 渲染 / 视图

// ---- 便捷宏(自动附加模块类别, noquote 让中文不加引号) ----
#define LOG_DEBUG(cat, ...) qCDebug(cat).noquote() << __VA_ARGS__
#define LOG_INFO(cat, ...)  qCInfo(cat).noquote() << __VA_ARGS__
#define LOG_WARN(cat, ...)  qCWarning(cat).noquote() << __VA_ARGS__
#define LOG_ERROR(cat, ...) qCCritical(cat).noquote() << __VA_ARGS__
