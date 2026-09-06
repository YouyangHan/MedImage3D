#pragma once

// ============================================================================
// SeriesSelectPanel —— 序列选择面板(嵌入主窗口，非弹窗)
//
// 布局(参考 3dpre 的序列选择)：
//   左侧：QListView 显示所有序列(缩略图 + 说明，SeriesListModel + 代理)
//   右侧：QLabel 大窗口预览当前选中序列；滚动条 / 滚轮 / 鼠标拖动切换层
//   底部：「三维重建」按钮
//
// 数据源：SeriesListModel 为唯一数据源(见 setSeries)。
// ============================================================================

#include <QList>
#include <QWidget>

class QLabel;
class QListView;
class QPushButton;
class QScrollBar;
class SeriesListModel;
struct SeriesInfo;

class SeriesSelectPanel : public QWidget
{
    Q_OBJECT

public:
    explicit SeriesSelectPanel(QWidget* parent = nullptr);
    ~SeriesSelectPanel() override;

    // 设置序列列表(扫描完成后调用)，自动选中第一项并刷新预览
    void setSeries(QList<SeriesInfo> series);

    // 当前选中序列下标；无选中返回 -1
    int selectedRow() const;

signals:
    // 用户点击「三维重建」，携带选中的序列下标(步骤 3 起加载体数据)
    void reconstructRequested(int seriesRow);

protected:
    // 预览区鼠标拖动 + 滚轮切层
    bool eventFilter(QObject* obj, QEvent* ev) override;

private slots:
    void onSelectionChanged();       // 左列表选中序列变化
    void onSliceChanged(int slice);  // 滚动条拖动切层
    void onReconstructClicked();     // 「三维重建」按钮

private:
    void updatePreview(int slice);   // 加载第 slice 层(0 基)到预览

    SeriesListModel*  m_model       = nullptr;
    QListView*        m_list        = nullptr;
    QLabel*           m_preview     = nullptr;   // 右侧大预览
    QScrollBar*       m_sliceSlider = nullptr;   // 垂直切层滚动条
    QLabel*           m_sliceInfo   = nullptr;   // "层 N / M"
    QPushButton*      m_reconstructBtn = nullptr;
    int               m_currentSeries = -1;      // 当前选中序列下标

    // ---- 鼠标拖动切层状态 ----
    bool m_dragging = false;
    int  m_lastDragY = 0;    // 上次鼠标 Y(用于算位移)
    int  m_dragAccum = 0;    // 累计位移(像素)，超过阈值切一层
};
