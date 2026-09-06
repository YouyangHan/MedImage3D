#pragma once

#include "dicom/SeriesInfo.h"

#include <QMainWindow>

class QProgressDialog;
class SeriesSelectPanel;

/**
 * @brief 主窗口 —— 序列选择面板嵌入中央区域(步骤 2)
 *
 * 工作流：
 *   点击"选择 CT 序列目录" -> 后台扫描(进度弹窗) -> 序列列表填充
 *   -> 预览图可拖动/滚轮切层 -> 点击"三维重建"(步骤 3 起加载体数据)。
 *
 * 渲染区(四视图)在后续步骤替换中央面板。
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
    // 序列面板「三维重建」按钮(步骤 3 起接入体数据加载)
    void onReconstructRequested(int seriesRow);

private:
    void setupActions();                 // 构建工具栏与动作
    void startScan(const QString& dirPath);

    SeriesSelectPanel* m_seriesPanel = nullptr;  // 中央序列选择面板
    QProgressDialog*   m_scanProgress = nullptr; // 扫描进度弹窗
};
