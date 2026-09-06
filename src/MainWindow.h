#pragma once

#include "core/Volume.h"
#include "dicom/SeriesInfo.h"

#include <QList>
#include <QMainWindow>
#include <vtkResliceCursor.h>
#include <vtkSmartPointer.h>

class QProgressDialog;
class QStackedWidget;
class SeriesSelectPanel;
class SliceView;
class VolumeView;

/**
 * @brief 主窗口 —— 序列选择 + 体数据加载 + MPR 三视图(步骤 2~4)
 *
 * 工作流：
 *   扫描 -> 序列面板 -> 三维重建 -> 体数据加载 -> 三视图(矢状/冠状/横断)联动。
 *
 * 三视图在第 4 步先做基础展示(共享十字线光标)，步骤 6 由 ViewManager 统一管理
 * 并加入体绘制视图。
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
    void setupMprViews(const Volume& volume);   // 构建/刷新三视图

    SeriesSelectPanel* m_seriesPanel = nullptr;   // 序列选择面板
    QStackedWidget*    m_stack = nullptr;         // 页面栈(序列面板 <-> 四视图)
    QProgressDialog*   m_scanProgress = nullptr;  // 扫描进度弹窗
    QProgressDialog*   m_loadProgress = nullptr;  // 加载进度弹窗

    // ---- 四视图(步骤 4~5 临时容器, 步骤 6 由 ViewManager 接管) ----
    QWidget* m_viewContainer = nullptr;           // 视图容器
    QList<SliceView*> m_sliceViews;               // 三个切片视图
    VolumeView* m_volumeView = nullptr;           // 三维体绘制视图
    vtkSmartPointer<vtkResliceCursor> m_cursor;   // 共享十字线光标
};
