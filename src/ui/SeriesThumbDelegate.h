#pragma once

// ============================================================================
// SeriesThumbDelegate —— 序列缩略图列表项绘制代理
//
// 设计说明：
//   自定义 QListView 每个条目的外观：缩略图在上、两行文字在下，
//   选中时绘制高亮边框。视图(QListView)只需设置此代理即可获得
//   统一风格的"卡片式"序列列表。
//
// 条目尺寸常量(kItemWidth/kItemHeight)对外暴露，供面板据此设置列表宽度。
// ============================================================================

#include <QStyledItemDelegate>

class SeriesThumbDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    // 条目固定尺寸(面板据此把列表宽度对齐到条目宽)
    static constexpr int kItemWidth  = 150;
    static constexpr int kItemHeight = 150;
    static constexpr int kThumbSize  = 96;   // 缩略图边长(与 DicomScanner 生成一致)

    using QStyledItemDelegate::QStyledItemDelegate;

    // 每个条目的固定尺寸(缩略图 + 两行文字 + 边距)
    QSize sizeHint(const QStyleOptionViewItem& option,
                   const QModelIndex& index) const override;

    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override;
};
