#pragma once

// ============================================================================
// 数学工具函数集合
//
// 集中提供常用数学操作，避免各业务代码重复实现。
// 设计：纯静态函数(无状态)，用命名空间内聚，无需实例化。
// ============================================================================

#include <algorithm>
#include <cmath>

namespace MathUtils {

// 圆周率
constexpr double kPi = 3.14159265358979323846;

// 范围钳制：把 value 限制在 [lo, hi] 闭区间内 (复用 C++17 std::clamp)
template <typename T>
constexpr T clamp(T value, T lo, T hi)
{
    return std::clamp(value, lo, hi);
}

// 判断 value 是否落在 [lo, hi] 闭区间内
template <typename T>
constexpr bool inRange(T value, T lo, T hi)
{
    return value >= lo && value <= hi;
}

// 角度转弧度
constexpr double degToRad(double deg)
{
    return deg * kPi / 180.0;
}

// 弧度转角度
constexpr double radToDeg(double rad)
{
    return rad * 180.0 / kPi;
}

// 四舍五入取整(返回 long, 兼容负数, 基于 std::lround)
inline long roundToLong(double v)
{
    return static_cast<long>(std::lround(v));
}

} // namespace MathUtils
