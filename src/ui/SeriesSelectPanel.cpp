#include "SeriesSelectPanel.h"

#include "SeriesListModel.h"
#include "SeriesThumbDelegate.h"
#include "dicom/SliceImageLoader.h"
#include "resources/strings.h"

#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QListView>
#include <QMouseEvent>
#include <QPixmap>
#include <QPushButton>
#include <QScrollBar>
#include <QSplitter>
#include <QVBoxLayout>
#include <QWheelEvent>

namespace {
// 右侧预览图最大边长(像素)
constexpr int kPreviewSize = 512;
// 拖动每累计多少像素切换一层(数值越大越"紧")
constexpr int kDragPixelsPerSlice = 14;
}

SeriesSelectPanel::SeriesSelectPanel(QWidget* parent)
    : QWidget(parent)
{
    // ---- 数据模型与左侧列表 ----
    m_model = new SeriesListModel(this);
    m_list  = new QListView(this);
    m_list->setModel(m_model);
    m_list->setItemDelegate(new SeriesThumbDelegate(m_list));
    m_list->setSelectionMode(QAbstractItemView::SingleSelection);
    m_list->setUniformItemSizes(true);

    // ---- 右侧预览 ----
    m_preview = new QLabel(this);
    m_preview->setMinimumSize(280, 280);
    m_preview->setAlignment(Qt::AlignCenter);
    m_preview->setFrameShape(QFrame::StyledPanel);
    m_preview->setText(AppStrings::kHintSelectSeries);
    m_preview->setCursor(Qt::OpenHandCursor);   // 提示可拖动切层
    m_preview->installEventFilter(this);

    // 层号提示(预览下方)
    m_sliceInfo = new QLabel(this);
    m_sliceInfo->setAlignment(Qt::AlignCenter);

    // 垂直滚动条：拖动切层
    m_sliceSlider = new QScrollBar(Qt::Vertical, this);
    m_sliceSlider->setRange(0, 0);
    m_sliceSlider->setPageStep(1);
    m_sliceSlider->hide();
    connect(m_sliceSlider, &QScrollBar::valueChanged,
            this, &SeriesSelectPanel::onSliceChanged);

    // 右侧区域：预览 + 滚动条(水平并排)
    auto* previewArea = new QWidget(this);
    auto* previewLayout = new QHBoxLayout(previewArea);
    previewLayout->setContentsMargins(0, 0, 0, 0);
    previewLayout->addWidget(m_preview, 1);
    previewLayout->addWidget(m_sliceSlider, 0);

    // 右侧整列：预览区 + 层号
    auto* right = new QWidget(this);
    auto* rightLayout = new QVBoxLayout(right);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->addWidget(previewArea, 1);
    rightLayout->addWidget(m_sliceInfo, 0);

    // ---- 左右分栏 ----
    // 左侧列表固定为条目宽 + 滚动条/边框预留，右侧预览随窗口拉伸
    m_list->setFixedWidth(SeriesThumbDelegate::kItemWidth + 20);
    auto* splitter = new QSplitter(Qt::Horizontal, this);
    splitter->addWidget(m_list);
    splitter->addWidget(right);
    splitter->setStretchFactor(0, 0);   // 左侧不拉伸
    splitter->setStretchFactor(1, 1);   // 右侧拉伸
    splitter->setCollapsible(0, false); // 左侧不可折叠

    // ---- 底部「三维重建」按钮 ----
    m_reconstructBtn = new QPushButton(AppStrings::kButtonReconstruct, this);
    m_reconstructBtn->setEnabled(false);   // 未选序列时禁用
    connect(m_reconstructBtn, &QPushButton::clicked,
            this, &SeriesSelectPanel::onReconstructClicked);

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(splitter, 1);
    layout->addWidget(m_reconstructBtn, 0, Qt::AlignRight);

    // ---- 信号连接：列表选中变化 -> 更新预览 ----
    connect(m_list->selectionModel(), &QItemSelectionModel::currentRowChanged,
            this, &SeriesSelectPanel::onSelectionChanged);
}

SeriesSelectPanel::~SeriesSelectPanel() = default;

void SeriesSelectPanel::setSeries(QList<SeriesInfo> series)
{
    m_currentSeries = -1;

    // 更新同一 model(selection model 不变，currentRowChanged 连接保持有效)
    m_model->setSeries(std::move(series));

    if (m_model->rowCount() > 0) {
        m_list->setCurrentIndex(m_model->index(0, 0));   // 触发 onSelectionChanged
        m_reconstructBtn->setEnabled(true);
    } else {
        m_preview->setText(AppStrings::kHintSelectSeries);
        m_preview->setPixmap(QPixmap());
        m_sliceInfo->clear();
        m_sliceSlider->hide();
        m_reconstructBtn->setEnabled(false);
    }
}

int SeriesSelectPanel::selectedRow() const
{
    const QModelIndex idx = m_list->currentIndex();
    return idx.isValid() ? idx.row() : -1;
}

void SeriesSelectPanel::onSelectionChanged()
{
    const int row = selectedRow();
    if (row < 0 || row >= m_model->rowCount())
        return;
    m_currentSeries = row;

    // 多层序列显示切层滚动条，单层隐藏
    const int count = m_model->at(row).imageCount();
    const bool multi = count > 1;
    m_sliceSlider->setVisible(multi);
    if (multi) {
        m_sliceSlider->setRange(0, count - 1);
        m_sliceSlider->setValue(0);
    }

    updatePreview(0);
}

void SeriesSelectPanel::onSliceChanged(int slice)
{
    updatePreview(slice);
}

void SeriesSelectPanel::onReconstructClicked()
{
    const int row = selectedRow();
    if (row >= 0 && row < m_model->rowCount())
        emit reconstructRequested(m_model->at(row));
}

void SeriesSelectPanel::updatePreview(int slice)
{
    if (m_currentSeries < 0 || m_currentSeries >= m_model->rowCount())
        return;
    const SeriesInfo& s = m_model->at(m_currentSeries);
    if (slice < 0 || slice >= s.filePaths.size())
        return;

    // 按需加载指定层(比 96px 缩略图清晰)，拖动实时切换
    QImage image = SliceImageLoader::load(s.filePaths.at(slice), kPreviewSize);
    if (image.isNull())
        image = s.thumbnail;   // 兜底用缩略图

    m_preview->setPixmap(QPixmap::fromImage(image));

    // 层号显示 "层 N / M"
    m_sliceInfo->setText(QStringLiteral("%1 %2 / %3")
        .arg(AppStrings::kLabelSlice).arg(slice + 1).arg(s.imageCount()));
}

bool SeriesSelectPanel::eventFilter(QObject* obj, QEvent* ev)
{
    if (obj == m_preview) {
        switch (ev->type()) {
        case QEvent::MouseButtonPress: {
            auto* me = static_cast<QMouseEvent*>(ev);
            if (me->button() == Qt::LeftButton && m_sliceSlider->isVisible()) {
                m_dragging = true;
                m_lastDragY = me->pos().y();
                m_dragAccum = 0;
                m_preview->setCursor(Qt::ClosedHandCursor);
                return true;
            }
            break;
        }
        case QEvent::MouseMove: {
            auto* me = static_cast<QMouseEvent*>(ev);
            if (m_dragging) {
                const int dy = me->pos().y() - m_lastDragY;
                m_lastDragY = me->pos().y();
                m_dragAccum += dy;
                // 累计位移换算成层数，剩余量留到下一次(平滑不丢)
                const int steps = m_dragAccum / kDragPixelsPerSlice;
                if (steps != 0) {
                    m_sliceSlider->setValue(m_sliceSlider->value() + steps);
                    m_dragAccum -= steps * kDragPixelsPerSlice;
                }
                return true;
            }
            break;
        }
        case QEvent::MouseButtonRelease:
            if (m_dragging) {
                m_dragging = false;
                m_dragAccum = 0;
                m_preview->setCursor(Qt::OpenHandCursor);
                return true;
            }
            break;

        // 滚轮切层：上滚上一层，下滚下一层
        case QEvent::Wheel: {
            if (m_sliceSlider->isVisible()) {
                auto* wheel = static_cast<QWheelEvent*>(ev);
                const int delta = (wheel->angleDelta().y() > 0) ? -1 : 1;
                m_sliceSlider->setValue(m_sliceSlider->value() + delta);
                return true;
            }
            break;
        }
        default:
            break;
        }
    }
    return QWidget::eventFilter(obj, ev);
}
