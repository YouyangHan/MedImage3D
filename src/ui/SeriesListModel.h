#pragma once

// ============================================================================
// SeriesListModel —— 序列列表数据模型
//
// 设计模式：模型-视图(Model-View)
//   把 QList<SeriesInfo> 适配成 QAbstractListModel，供 QListView 显示。
//   视图只关心 role 数据，不直接触碰 SeriesInfo 结构。
//
// 提供的数据角色：
//   DisplayRole    两行文字: "序列N 描述" / "N张"
//   DecorationRole 首图缩略图(QPixmap)
//   ToolTipRole    完整信息
//
// 注意：用 setSeries() 更新数据，而不是重建 model —— 重建会导致
//       QListView 的 selection model 被替换，信号连接失效。
// ============================================================================

#include "dicom/SeriesInfo.h"

#include <QAbstractListModel>
#include <QList>

class SeriesListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit SeriesListModel(QObject* parent = nullptr)
        : QAbstractListModel(parent)
    {
    }

    explicit SeriesListModel(QList<SeriesInfo> series, QObject* parent = nullptr)
        : QAbstractListModel(parent), m_series(std::move(series))
    {
    }

    // 更新序列数据并通知视图刷新(同一 model，selection model 保持不变)
    void setSeries(QList<SeriesInfo> series)
    {
        beginResetModel();
        m_series = std::move(series);
        endResetModel();
    }

    // 按行号取原始数据（供面板加载对应序列切片）
    const SeriesInfo& at(int row) const { return m_series.at(row); }

    // ---- QAbstractListModel 接口 ----
    int rowCount(const QModelIndex& parent = {}) const override
    {
        return parent.isValid() ? 0 : m_series.size();
    }

    QVariant data(const QModelIndex& index, int role) const override;

private:
    QList<SeriesInfo> m_series;
};
