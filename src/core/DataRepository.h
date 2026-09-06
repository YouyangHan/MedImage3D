#pragma once

// ============================================================================
// DataRepository —— 当前数据仓库
//
// 设计模式：单例(Singleton) + 仓库(Repository)
//   全局唯一持有"当前体数据"，作为数据访问的唯一入口，视图层从这里取数据，
//   避免在多个视图/控制器之间手传指针、分散状态。
//
// 使用方式：
//   DataRepository::instance().setCurrentVolume(volume);
//   const Volume& v = DataRepository::instance().currentVolume();
// ============================================================================

#include "Volume.h"
#include "common/Singleton.h"

class DataRepository : public Singleton<DataRepository>
{
    friend class Singleton<DataRepository>;   // 允许基类访问私有构造

public:
    // 当前体数据(未加载时为无效 Volume)
    const Volume& currentVolume() const { return m_volume; }

    // 设置当前体数据
    void setCurrentVolume(Volume volume) { m_volume = std::move(volume); }

    // 是否已有可用的体数据
    bool hasVolume() const { return m_volume.isValid(); }

    // 清空所有数据
    void reset() { m_volume.reset(); }

private:
    DataRepository() = default;

    Volume m_volume;
};
