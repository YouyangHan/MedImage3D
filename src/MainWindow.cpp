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
#include "view/SliceView.h"
#include "view/TransferFunctionFactory.h"
#include "view/VolumeView.h"

#include <vtkImageViewer2.h>
#include <vtkResliceCursor.h>
#include <vtkVersion.h>

#include <itkVersion.h>

#include <dcmtk/config/osconfig.h>

#include <QAction>
#include <QDir>
#include <QFileDialog>
#include <QGridLayout>
#include <QMessageBox>
#include <QProgressDialog>
#include <QStackedWidget>
#include <QStatusBar>
#include <QToolBar>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    // 中央区域：用 QStackedWidget 承载"序列面板"和"四视图"两页，
    // 用 setCurrentWidget 切换(避免运行期 setCentralWidget 触发 Qt6 RHI 崩溃)。
    m_seriesPanel = new SeriesSelectPanel(this);
    m_stack = new QStackedWidget(this);
    m_stack->addWidget(m_seriesPanel);
    setCentralWidget(m_stack);
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
    m_stack->setCurrentWidget(m_seriesPanel);   // 重新扫描时切回序列面板
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
}

void MainWindow::showVolumeForTest(const Volume& volume)
{
    setupMprViews(volume);
}

void MainWindow::setupMprViews(const Volume& volume)
{
    // 共享十字线光标：三个视图的光标中心/轴向全局唯一，十字线自动同步
    if (!m_cursor)
        m_cursor = vtkSmartPointer<vtkResliceCursor>::New();
    m_cursor->SetImage(volume.imageData);
    m_cursor->SetCenter(volume.imageData->GetCenter());

    if (!m_viewContainer) {
        // 首次创建：三个方向的切片视图，2x2 网格(右下留待步骤 5 的体绘制)
        m_viewContainer = new QWidget(this);
        auto* grid = new QGridLayout(m_viewContainer);
        grid->setContentsMargins(0, 0, 0, 0);

        // 方向：0=矢状(YZ), 1=冠状(XZ), 2=横断(XY)
        const int orientations[3] = {
            vtkImageViewer2::SLICE_ORIENTATION_XY,   // 横断
            vtkImageViewer2::SLICE_ORIENTATION_YZ,   // 矢状
            vtkImageViewer2::SLICE_ORIENTATION_XZ,   // 冠状
        };
        const int positions[3][2] = { {0, 0}, {0, 1}, {1, 0} };

        for (int i = 0; i < 3; ++i) {
            auto* sv = new SliceView(m_viewContainer);
            sv->setVolume(volume.imageData);
            sv->setOrientation(orientations[i]);
            sv->setSharedCursor(m_cursor);
            grid->addWidget(sv, positions[i][0], positions[i][1]);
            m_sliceViews << sv;
        }

        // 右下：三维体绘制视图(默认骨窗)
        m_volumeView = new VolumeView(m_viewContainer);
        m_volumeView->setVolume(volume.imageData, TransferPreset::Bone);
        grid->addWidget(m_volumeView, 1, 1);

        // 加入页面栈并切换(首次)
        m_stack->addWidget(m_viewContainer);
    } else {
        // 已创建：重新加载新体数据到各视图
        for (SliceView* sv : m_sliceViews) {
            sv->setVolume(volume.imageData);
            sv->setSharedCursor(m_cursor);
        }
        m_volumeView->setVolume(volume.imageData, TransferPreset::Bone);
    }

    m_stack->setCurrentWidget(m_viewContainer);
}
