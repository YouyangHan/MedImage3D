#include "MainWindow.h"

#include "common/Logging.h"
#include "common/Settings.h"
#include "core/DataRepository.h"
#include "dicom/LoadThread.h"
#include "dicom/ScanThread.h"
#include "dicom/SeriesInfo.h"
#include "resources/paths.h"
#include "resources/strings.h"
#include "ui/SeriesSelectPanel.h"
#include "view/TransferFunctionFactory.h"
#include "view/ViewManager.h"

#include <vtkVersion.h>

#include <itkVersion.h>

#include <dcmtk/config/osconfig.h>

#include <QAction>
#include <QDir>
#include <QFileDialog>
#include <QIcon>
#include <QMessageBox>
#include <QProgressDialog>
#include <QStackedWidget>
#include <QStatusBar>
#include <QToolBar>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    // 工作流状态机
    m_controller = new AppController(this);
    connect(m_controller, &AppController::stateChanged,
            this, &MainWindow::onStateChanged);

    // 中央区域：用 QStackedWidget 承载"序列面板"和"四视图"两页，
    // 用 setCurrentWidget 切换(避免运行期 setCentralWidget 触发 Qt6 RHI 崩溃)。
    m_seriesPanel = new SeriesSelectPanel(this);
    m_stack = new QStackedWidget(this);
    m_stack->addWidget(m_seriesPanel);
    setCentralWidget(m_stack);
    connect(m_seriesPanel, &SeriesSelectPanel::reconstructRequested,
            this, &MainWindow::onReconstructRequested);

    setupActions();
    onStateChanged(WorkflowState::Idle);

    // 状态栏显示三方库版本, 验证链接正确
    statusBar()->showMessage(QStringLiteral("VTK %1 | ITK %2 | DCMTK %3")
        .arg(QString::fromLatin1(vtkVersion::GetVTKVersion()))
        .arg(QString::fromLatin1(ITK_VERSION))
        .arg(QString::fromLatin1(PACKAGE_VERSION)));
}

MainWindow::~MainWindow() = default;

void MainWindow::setupActions()
{
    auto* toolbar = addToolBar(QStringLiteral("main"));
    toolbar->setMovable(false);
    toolbar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

    // 动作：选择 CT 序列目录
    m_openCtAction = new QAction(QIcon(AppPaths::kIconOpenCt), AppStrings::kActionOpenCt, this);
    m_openCtAction->setToolTip(AppStrings::kActionOpenCt);
    m_openCtAction->setStatusTip(AppStrings::kActionOpenCtStatus);
    connect(m_openCtAction, &QAction::triggered, this, &MainWindow::onOpenCtDirectory);

    // 动作：三维重建(切回序列面板重新选择)
    m_reconstructAction = new QAction(QIcon(AppPaths::kIconReconstruct), AppStrings::kActionReconstruct, this);
    m_reconstructAction->setToolTip(AppStrings::kActionReconstruct);
    m_reconstructAction->setStatusTip(AppStrings::kActionReconstructStatus);
    connect(m_reconstructAction, &QAction::triggered, this, &MainWindow::onReconstructClicked);

    // 动作：窗宽窗位预设(骨窗/软组织/肌肉)
    m_presetAction = new QAction(QIcon(AppPaths::kIconPreset), AppStrings::kActionPreset, this);
    m_presetAction->setToolTip(AppStrings::kActionPreset);
    m_presetAction->setStatusTip(AppStrings::kActionPresetStatus);
    connect(m_presetAction, &QAction::triggered, this, &MainWindow::onPresetClicked);

    // 动作：重置视图
    m_resetAction = new QAction(QIcon(AppPaths::kIconReset), AppStrings::kActionReset, this);
    m_resetAction->setToolTip(AppStrings::kActionReset);
    m_resetAction->setStatusTip(AppStrings::kActionResetStatus);
    connect(m_resetAction, &QAction::triggered, this, &MainWindow::onResetClicked);

    toolbar->addAction(m_openCtAction);
    toolbar->addAction(m_reconstructAction);
    toolbar->addSeparator();
    toolbar->addAction(m_presetAction);
    toolbar->addAction(m_resetAction);
}

void MainWindow::onOpenCtDirectory()
{
    // 优先用上次打开的目录(若设置了"记住目录")
    QString startDir = QDir::homePath();
    const Settings& settings = Settings::instance();
    if (settings.rememberLastDirectory() && !settings.lastDirectory().isEmpty())
        startDir = settings.lastDirectory();

    const QString dir = QFileDialog::getExistingDirectory(
        this, AppStrings::kActionOpenCt, startDir);
    if (dir.isEmpty())
        return;   // 用户取消

    Settings::instance().setLastDirectory(dir);
    startScan(dir);
}

void MainWindow::startScan(const QString& dirPath)
{
    // 进度弹窗：无按钮(由完成关闭)，范围 0..0 表示不确定进度
    if (!m_scanProgress) {
        m_scanProgress = new QProgressDialog(this);
        m_scanProgress->setWindowTitle(AppStrings::kActionOpenCt);
        m_scanProgress->setLabelText(AppStrings::kProgressScan);
        m_scanProgress->setCancelButton(nullptr);
        m_scanProgress->setAutoClose(false);
        m_scanProgress->setMinimumDuration(0);
    }
    m_scanProgress->show();
    m_controller->transitionTo(WorkflowState::Scanning);

    // 后台扫描线程；由 Qt 父对象自动回收
    auto* thread = new ScanThread(dirPath, this);
    connect(thread, &ScanThread::progressChanged, this,
            [this](int done, int total) {
                if (!m_scanProgress) return;
                m_scanProgress->setMaximum(total);
                m_scanProgress->setValue(done);
            });
    connect(thread, &ScanThread::scanFinished, this, &MainWindow::onScanFinished);
    connect(thread, &QThread::finished, thread, &QObject::deleteLater);
    thread->start();
}

void MainWindow::onScanFinished(const QList<SeriesInfo>& series)
{
    if (m_scanProgress)
        m_scanProgress->hide();   // reset() 仅重置进度值, 需显式 hide 关闭窗口

    if (series.isEmpty()) {
        QMessageBox::warning(this, AppStrings::kAppDisplayName,
                             AppStrings::kErrNoDicomFiles);
        m_controller->transitionTo(WorkflowState::Idle);
        return;
    }

    LOG_INFO(lcDicom, "扫描得到 " << series.size() << " 个序列");
    m_seriesPanel->setSeries(series);
    m_stack->setCurrentWidget(m_seriesPanel);   // 重新扫描时切回序列面板
    m_controller->transitionTo(WorkflowState::SeriesSelect);
}

void MainWindow::onReconstructRequested(const SeriesInfo& series)
{
    LOG_INFO(lcDicom, "请求三维重建: " << series.seriesDescription
             << " (" << series.imageCount() << " 张)");
    startLoad(series);
}

void MainWindow::startLoad(const SeriesInfo& series)
{
    // 加载进度弹窗：百分比进度(0..100)
    if (!m_loadProgress) {
        m_loadProgress = new QProgressDialog(this);
        m_loadProgress->setWindowTitle(AppStrings::kActionReconstruct);
        m_loadProgress->setLabelText(AppStrings::kProgressLoad);
        m_loadProgress->setCancelButton(nullptr);
        m_loadProgress->setAutoClose(false);
        m_loadProgress->setMinimumDuration(0);
        m_loadProgress->setRange(0, 100);
    }
    m_loadProgress->setValue(0);
    m_loadProgress->show();
    m_controller->transitionTo(WorkflowState::Loading);

    // 后台加载线程；由 Qt 父对象自动回收
    auto* thread = new LoadThread(series.filePaths, this);
    connect(thread, &LoadThread::progressChanged, this,
            [this](double p) {
                if (m_loadProgress) m_loadProgress->setValue(static_cast<int>(p * 100));
            });
    connect(thread, &LoadThread::loadFinished, this, &MainWindow::onLoadFinished);
    connect(thread, &QThread::finished, thread, &QObject::deleteLater);
    thread->start();
}

void MainWindow::onLoadFinished()
{
    if (m_loadProgress)
        m_loadProgress->hide();

    // 从 sender 取后台线程，主线程做 ITK->VTK 桥接(涉及 VTK, 必须主线程)
    auto* thread = qobject_cast<LoadThread*>(sender());
    if (!thread)
        return;
    Volume volume = VolumeLoader::convertToVolume(thread->image());

    if (!volume.isValid()) {
        QMessageBox::warning(this, AppStrings::kAppDisplayName,
                             AppStrings::kErrLoadFailed);
        m_controller->transitionTo(WorkflowState::SeriesSelect);
        return;
    }

    // 存入数据仓库，供后续四视图使用
    DataRepository::instance().setCurrentVolume(volume);

    const QString info = QStringLiteral("体数据 %1x%2x%3, 间距 %4/%5/%6 mm")
        .arg(volume.dimensions[0]).arg(volume.dimensions[1]).arg(volume.dimensions[2])
        .arg(volume.spacing[0], 0, 'f', 2)
        .arg(volume.spacing[1], 0, 'f', 2)
        .arg(volume.spacing[2], 0, 'f', 2);
    statusBar()->showMessage(info);
    LOG_INFO(lcRender, "体数据已就绪: " << info);

    // 展示 MPR 三视图(步骤 6 由 ViewManager 接管)
    setupMprViews(volume);
    m_controller->transitionTo(WorkflowState::FourView);
}

void MainWindow::showVolumeForTest(const Volume& volume)
{
    setupMprViews(volume);
    m_controller->transitionTo(WorkflowState::FourView);
}

void MainWindow::setupMprViews(const Volume& volume)
{
    // 首次创建 ViewManager(四视图编排 + 联动)，加入页面栈
    if (!m_viewManager) {
        m_viewManager = new ViewManager(this);
        m_stack->addWidget(m_viewManager);
    }

    // 填充体数据并切换到四视图页
    m_viewManager->setVolume(volume);
    m_stack->setCurrentWidget(m_viewManager);
}

void MainWindow::onReconstructClicked()
{
    // 切回序列面板，让用户选择序列后点面板的"三维重建"
    m_stack->setCurrentWidget(m_seriesPanel);
    m_controller->transitionTo(WorkflowState::SeriesSelect);
}

void MainWindow::onPresetClicked()
{
    if (!m_viewManager)
        return;

    // 循环切换体绘制传递函数：骨窗 -> 软组织 -> 肌肉
    switch (m_preset) {
    case TransferPreset::Bone:       m_preset = TransferPreset::SoftTissue; break;
    case TransferPreset::SoftTissue: m_preset = TransferPreset::Muscle;     break;
    case TransferPreset::Muscle:     m_preset = TransferPreset::Bone;       break;
    }
    m_viewManager->setPreset(m_preset);
    statusBar()->showMessage(TransferFunctionFactory::presetName(m_preset), 3000);
}

void MainWindow::onResetClicked()
{
    // 重置四视图：重新加载当前体数据(重置光标中心与相机)
    const Volume& v = DataRepository::instance().currentVolume();
    if (v.isValid() && m_viewManager)
        m_viewManager->setVolume(v);
}

void MainWindow::onStateChanged(WorkflowState state)
{
    // 按工作流状态启用/禁用工具栏动作
    const bool busy = (state == WorkflowState::Scanning || state == WorkflowState::Loading);
    const bool fourView = (state == WorkflowState::FourView);

    m_openCtAction->setEnabled(!busy);
    m_reconstructAction->setEnabled(fourView || state == WorkflowState::SeriesSelect);
    m_presetAction->setEnabled(fourView);
    m_resetAction->setEnabled(fourView);
}
