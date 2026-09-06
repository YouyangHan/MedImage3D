#pragma once

#include "core/Volume.h"
#include "dicom/SeriesInfo.h"

#include <QList>
#include <QMainWindow>

class QProgressDialog;
class QStackedWidget;
class SeriesSelectPanel;
class ViewManager;

/**
 * @brief 主窗口 —— 序列选择 + 体数据加载 + 四视图(步骤 2~6)
 *
 * 工作流：
 *   扫描 -> 序列面板 -> 三维重建 -> 体数据加载 -> 四视图(矢状/冠状/横断 + 体绘制)联动。
 *
 * 四视图的编排与联动由 ViewManager 统一管理(中介者)。
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

    // 测试/调试用：直接显示体数据四视图(绕过扫描/选择流程)
    void showVolumeForTest(const Volume& volume);

private slots:
    void onOpenCtDirectory();
    void onScanFinished(const QList<SeriesInfo>& series);
    void onReconstructRequested(const SeriesInfo& series);
    void onLoadFinished();

private:
    void setupActions();                   // 构建工具栏与动作
    void startScan(const QString& dirPath);
    void startLoad(const SeriesInfo& series);
    void setupMprViews(const Volume& volume);   // 构建/刷新四视图

    SeriesSelectPanel* m_seriesPanel = nullptr;   // 序列选择面板
    QStackedWidget*    m_stack = nullptr;         // 页面栈(序列面板 <-> 四视图)
    ViewManager*       m_viewManager = nullptr;   // 四视图编排/联动
    QProgressDialog*   m_scanProgress = nullptr;  // 扫描进度弹窗
    QProgressDialog*   m_loadProgress = nullptr;  // 加载进度弹窗
};
