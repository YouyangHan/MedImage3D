#include "MainWindow.h"

#include "common/Logging.h"
#include "common/Settings.h"
#include "dicom/ScanThread.h"
#include "dicom/SeriesInfo.h"
#include "resources/paths.h"
#include "resources/strings.h"
#include "ui/SeriesSelectPanel.h"

#include <vtkVersion.h>

#include <itkVersion.h>

#include <dcmtk/config/osconfig.h>

#include <QAction>
#include <QDir>
#include <QFileDialog>
#include <QMessageBox>
#include <QProgressDialog>
#include <QStatusBar>
#include <QToolBar>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    // 中央区域：序列选择面板(替代圆锥与环境验证渲染)
    m_seriesPanel = new SeriesSelectPanel(this);
    setCentralWidget(m_seriesPanel);
    connect(m_seriesPanel, &SeriesSelectPanel::reconstructRequested,
            this, &MainWindow::onReconstructRequested);

    setupActions();

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

    // 动作：选择 CT 序列目录
    auto* openCtAction = new QAction(AppStrings::kActionOpenCt, this);
    openCtAction->setStatusTip(AppStrings::kActionOpenCtStatus);
    connect(openCtAction, &QAction::triggered, this, &MainWindow::onOpenCtDirectory);
    toolbar->addAction(openCtAction);
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
        return;
    }

    LOG_INFO(lcDicom, "扫描得到 " << series.size() << " 个序列");
    m_seriesPanel->setSeries(series);
}

void MainWindow::onReconstructRequested(int seriesRow)
{
    LOG_INFO(lcDicom, "请求三维重建: 序列 " << seriesRow);
    // 步骤 3 起：在此对选中序列进行体数据加载 + 四视图重建
}
