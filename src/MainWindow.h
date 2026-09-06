#pragma once

#include "app/AppController.h"
#include "core/Volume.h"
#include "dicom/SeriesInfo.h"
#include "view/TransferFunctionFactory.h"

#include <QList>
#include <QMainWindow>

class QAction;
class QProgressDialog;
class QStackedWidget;
class SeriesSelectPanel;
class ViewManager;

/**
 * @brief 主窗口 —— 序列选择 + 体数据加载 + 四视图(步骤 2~8)
 *
 * 工作流(由 AppController 状态机驱动)：
 *   Idle -> Scanning -> SeriesSelect -> Loading -> FourView
 *
 * 工具栏四个动作(均带图标 + 中文 ToolTip)：
 *   选择 CT 目录 / 三维重建 / 窗宽窗位预设 / 重置视图。
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
    void onOpenCtDirectory();                       // 选目录 + 扫描
    void onScanFinished(const QList<SeriesInfo>& series);
    void onReconstructRequested(const SeriesInfo& series);  // 序列面板"三维重建"
    void onLoadFinished();
    void onReconstructClicked();                    // 工具栏"三维重建"(切回序列面板)
    void onPresetClicked();                         // 切换体绘制传递函数
    void onResetClicked();                          // 重置四视图
    void onStateChanged(WorkflowState state);       // 状态机驱动 UI

private:
    void setupActions();                            // 构建工具栏与动作(图标+ToolTip)
    void startScan(const QString& dirPath);
    void startLoad(const SeriesInfo& series);
    void setupMprViews(const Volume& volume);       // 构建/刷新四视图

    AppController*     m_controller = nullptr;      // 工作流状态机
    SeriesSelectPanel* m_seriesPanel = nullptr;     // 序列选择面板
    QStackedWidget*    m_stack = nullptr;           // 页面栈(序列面板 <-> 四视图)
    ViewManager*       m_viewManager = nullptr;     // 四视图编排/联动
    QProgressDialog*   m_scanProgress = nullptr;    // 扫描进度弹窗
    QProgressDialog*   m_loadProgress = nullptr;    // 加载进度弹窗

    QAction* m_openCtAction = nullptr;              // 选择 CT 目录
    QAction* m_reconstructAction = nullptr;         // 三维重建
    QAction* m_presetAction = nullptr;              // 窗宽窗位预设
    QAction* m_resetAction = nullptr;               // 重置视图

    TransferPreset m_preset = TransferPreset::Bone; // 当前传递函数预设
};
