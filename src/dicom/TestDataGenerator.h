#pragma once

// ============================================================================
// TestDataGenerator —— 合成 DICOM 测试数据生成器
//
// 用途：在没有真实 CT 数据时验证"扫描 -> 分组 -> 缩略图"全流程。
//   生成两套假 CT 序列(不同 SeriesInstanceUID/序列号/层数)，
//   每张为 64x64 的 16 位灰度图，像素为移动圆形图案，含窗宽窗位标签。
//
// 调用入口：main.cpp 的命令行开关 --gen-test-data <目录>
// ============================================================================

#include <QString>

namespace TestDataGenerator {

// 在 dirPath 下生成两套测试序列；成功返回 true，失败时 errorMsg 带原因
bool generate(const QString& dirPath, QString* errorMsg = nullptr);

} // namespace TestDataGenerator
