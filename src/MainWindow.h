#pragma once

#include "core/Volume.h"
#include "dicom/SeriesInfo.h"

#include <QMainWindow>

class QProgressDialog;
class SeriesSelectPanel;

/**
 * @brief 主窗口 —— 序列选择 + 体数据加载(步骤 2~3)
 *
 * 工作流：
 *   点击"选择 CT 序列目录" -> 后台扫描 -> 序列列表填充 -> 预览切层
 *   -> 点击"三维重建" -> 后台加载体数据(ITK) -> 存入 DataRepository。
 *
 * 四视图渲染在后续步骤(4~6)替换中央面板。
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

private slots:
    // 工具栏"选择 CT 序列目录"：选择目录并启动后台扫描
    void onOpenCtDirectory();
    // 扫描线程结束：隐藏进度框，填充序列面板
    void onScanFinished(const QList<SeriesInfo>& series);
    // 序列面板「三维重建」：启动后台体数据加载
    void onReconstructRequested(const SeriesInfo& series);
    // 加载线程结束：隐藏进度框，存入数据仓库
    void onLoadFinished(const Volume& volume);

private:
    void setupActions();                   // 构建工具栏与动作
    void startScan(const QString& dirPath);
    void startLoad(const SeriesInfo& series);

    SeriesSelectPanel* m_seriesPanel = nullptr;   // 中央序列选择面板
    QProgressDialog*   m_scanProgress = nullptr;  // 扫描进度弹窗
    QProgressDialog*   m_loadProgress = nullptr;  // 加载进度弹窗
};
