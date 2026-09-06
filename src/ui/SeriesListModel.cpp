#include "SeriesListModel.h"

#include "resources/strings.h"

#include <QPixmap>

QVariant SeriesListModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= m_series.size())
        return {};

    const SeriesInfo& s = m_series.at(index.row());

    switch (role) {
    case Qt::DisplayRole:
        // 第一行: 序列号+描述; 第二行: 张数
        return QStringLiteral("%1%2 %3\n%4 %5")
            .arg(AppStrings::kLabelSeries)
            .arg(s.seriesNumber)
            .arg(s.seriesDescription)
            .arg(s.imageCount())
            .arg(AppStrings::kLabelImageCount);

    case Qt::DecorationRole:
        return s.thumbnail.isNull() ? QVariant{} : QVariant(QPixmap::fromImage(s.thumbnail));

    case Qt::ToolTipRole:
        // 悬浮提示：患者 / 日期 / 设备 / 尺寸 / 张数
        return QStringLiteral("%1: %2\n%3: %4\n%5: %6\n%7x%8, %9 %10")
            .arg(AppStrings::kLabelPatientName, s.patientName)
            .arg(AppStrings::kLabelStudyDate, s.studyDate)
            .arg(AppStrings::kLabelModality, s.modality)
            .arg(s.imageSize.width()).arg(s.imageSize.height())
            .arg(s.imageCount())
            .arg(AppStrings::kLabelImageCount);

    default:
        return {};
    }
}
