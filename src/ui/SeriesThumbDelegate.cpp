#include "SeriesThumbDelegate.h"

#include <QPainter>
#include <QPixmap>

namespace {
constexpr int kMargin    = 8;    // 内边距
constexpr int kTextLineH = 18;   // 单行文字高
}

QSize SeriesThumbDelegate::sizeHint(const QStyleOptionViewItem& /*option*/,
                                    const QModelIndex& /*index*/) const
{
    return QSize(kItemWidth, kItemHeight);
}

void SeriesThumbDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option,
                                const QModelIndex& index) const
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    const QRect rect = option.rect;

    // ---- 背景与选中态 ----
    const bool selected = option.state & QStyle::State_Selected;
    painter->fillRect(rect, selected ? option.palette.highlight().color().lighter(160)
                                     : option.palette.base());
    if (selected) {
        painter->setPen(QPen(option.palette.highlight(), 2));
        painter->drawRect(rect.adjusted(1, 1, -2, -2));
    }

    // ---- 缩略图(居中) ----
    const QPixmap thumb = index.data(Qt::DecorationRole).value<QPixmap>();
    const QRect thumbRect(rect.center().x() - kThumbSize / 2, rect.top() + kMargin,
                          kThumbSize, kThumbSize);
    if (!thumb.isNull())
        painter->drawPixmap(thumbRect, thumb);
    else {
        // 无缩略图时画占位框
        painter->setPen(option.palette.mid().color());
        painter->drawRect(thumbRect);
    }

    // ---- 文字(缩略图下方, 最多两行, 超长省略) ----
    const QString text = index.data(Qt::DisplayRole).toString();
    const QStringList lines = text.split(QLatin1Char('\n'));
    painter->setPen(selected ? option.palette.highlightedText().color()
                             : option.palette.text().color());
    int y = thumbRect.bottom() + 4;
    for (const QString& line : lines) {
        const QRect lineRect(rect.left() + kMargin, y,
                             rect.width() - 2 * kMargin, kTextLineH);
        painter->drawText(lineRect, Qt::AlignHCenter | Qt::AlignVCenter,
                          painter->fontMetrics().elidedText(line, Qt::ElideRight,
                                                            lineRect.width()));
        y += kTextLineH;
    }

    painter->restore();
}
