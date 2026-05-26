#include "MainWindow.h"
#include "ui/dialogs/ExportDialog.h"
#include "app/SettingsManager.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFile>
#include <QMessageBox>
#include <QIcon>
#include <QApplication>
#include <QDebug>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QCloseEvent>
#include <QFileInfo>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle("DiskLens — Premium Storage Analyzer");
    resize(1200, 800);

    setupUI();
    setupMenuBar();
    applyTheme();
    restoreWindowState();

    // Connect scan service slots
    connect(&m_scanService, &ScanService::progressReported, this, &MainWindow::onScanProgress);
    connect(&m_scanService, &ScanService::scanCompleted, this, &MainWindow::onScanCompleted);
}

void MainWindow::setupMenuBar() {
    QMenuBar* menuBar = this->menuBar();
    menuBar->setStyleSheet("QMenuBar { background: #0c1424; color: #e5e7eb; }"
                           "QMenuBar::item:selected { background: #1e293b; }"
                           "QMenu { background: #1e1e2e; color: #e5e7eb; border: 1px solid #374151; }"
                           "QMenu::item:selected { background: #7c3aed; }");

    QMenu* fileMenu = menuBar->addMenu("&File");
    QAction* exportAction = fileMenu->addAction("📤 &Export Scan Results...");
    exportAction->setShortcut(QKeySequence("Ctrl+E"));
    connect(exportAction, &QAction::triggered, this, &MainWindow::onExportTriggered);

    fileMenu->addSeparator();
    QAction* exitAction = fileMenu->addAction("E&xit");
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, this, &QWidget::close);

    QMenu* viewMenu = menuBar->addMenu("&View");
    QStringList moduleNames = {"&Dashboard", "&Space Map", "&Large Files", "File &Types",
                                "D&uplicates", "S&earch/Clean", "&Advanced Search",
                                "&Organizer", "Te&mplates"};
    for (int i = 0; i < moduleNames.size(); i++) {
        QAction* a = viewMenu->addAction(moduleNames[i]);
        a->setShortcut(QKeySequence(QString("Ctrl+%1").arg(i + 1)));
        connect(a, &QAction::triggered, this, [this, i]() {
            m_sidebarGroup->button(i)->setChecked(true);
            onSidebarButtonClicked(i);
        });
    }

    QMenu* helpMenu = menuBar->addMenu("&Help");
    QAction* aboutAction = helpMenu->addAction("&About DiskLens");
    connect(aboutAction, &QAction::triggered, this, [this]() {
        QMessageBox::about(this, "About DiskLens",
            "<h2>DiskLens v1.0.0</h2>"
            "<p>Premium Storage Analyzer</p>"
            "<p>Native C++ / Qt Framework</p>"
            "<p>Modules: Dashboard, Space Map, Large Files, File Types, "
            "Duplicates, Search, Organizer, Export, Templates</p>"
            "<hr><p style='color:#94a3b8;'>Built with modern C++17 and Qt Widgets.</p>");
    });
}

void MainWindow::setupUI() {
    auto centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    auto mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // ── 1. Sidebar Container ──
    auto sidebar = new QFrame(this);
    sidebar->setObjectName("sidebarFrame");
    sidebar->setFixedWidth(240);

    auto sidebarLayout = new QVBoxLayout(sidebar);
    sidebarLayout->setContentsMargins(15, 20, 15, 20);
    sidebarLayout->setSpacing(10);

    // Logo
    auto logoLabel = new QLabel("DISKLENS", this);
    logoLabel->setObjectName("logoLabel");
    logoLabel->setAlignment(Qt::AlignCenter);
    logoLabel->setStyleSheet("font-size: 22px; font-weight: 800; letter-spacing: 2px; color: #00d4ff; padding: 15px 0px;");
    sidebarLayout->addWidget(logoLabel);

    auto subLogo = new QLabel("SYSTEM STORAGE ANALYZER", this);
    subLogo->setAlignment(Qt::AlignCenter);
    subLogo->setStyleSheet("font-size: 8px; font-weight: bold; color: #64748b; margin-top: -15px; margin-bottom: 25px; letter-spacing: 1px;");
    sidebarLayout->addWidget(subLogo);

    // Navigation buttons — all 9 modules
    m_sidebarGroup = new QButtonGroup(this);
    m_sidebarGroup->setExclusive(true);

    QStringList navItems = {
        "📊 Dashboard",
        "🗺️ Space Map",
        "🔍 Large Files",
        "🍩 File Types",
        "👯 Duplicate Finder",
        "⚙️ Search / Clean",
        "🔎 Advanced Search",
        "📂 Organizer",
        "📁 Templates"
    };

    for (int i = 0; i < navItems.size(); ++i) {
        auto navBtn = new QPushButton(navItems[i], this);
        navBtn->setCheckable(true);
        navBtn->setProperty("class", "sidebar-btn");
        m_sidebarGroup->addButton(navBtn, i);
        sidebarLayout->addWidget(navBtn);
    }

    m_sidebarGroup->button(0)->setChecked(true);
    connect(m_sidebarGroup, &QButtonGroup::idClicked,
            this, &MainWindow::onSidebarButtonClicked);

    sidebarLayout->addStretch();

    auto footerLabel = new QLabel("v1.0.0 (Native C++ / Qt6)", this);
    footerLabel->setAlignment(Qt::AlignCenter);
    footerLabel->setStyleSheet("color: #334155; font-size: 10px; font-weight: 500;");
    sidebarLayout->addWidget(footerLabel);

    mainLayout->addWidget(sidebar);

    // ── 2. Right Pane ──
    auto rightPane = new QWidget(this);
    auto rightLayout = new QVBoxLayout(rightPane);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(0);

    // Network drive banner (hidden by default)
    m_networkBanner = new QFrame(this);
    m_networkBanner->setStyleSheet("background-color: #1e1b4b; border-bottom: 2px solid #7c3aed; padding: 8px 16px;");
    m_networkBanner->setFixedHeight(40);
    m_networkBanner->hide();
    auto bannerLayout = new QHBoxLayout(m_networkBanner);
    bannerLayout->setContentsMargins(16, 0, 16, 0);
    auto bannerLabel = new QLabel("🌐 Network drive detected — optimized mode active (reduced depth, lower concurrency)", this);
    bannerLabel->setStyleSheet("color: #c4b5fd; font-size: 11px; font-weight: 600;");
    bannerLayout->addWidget(bannerLabel);
    rightLayout->addWidget(m_networkBanner);

    // Scan Status Banner
    m_scanStatusBar = new QFrame(this);
    m_scanStatusBar->setStyleSheet("background-color: #0c1424; border-bottom: 1px solid rgba(0, 212, 255, 0.15);");
    m_scanStatusBar->setFixedHeight(50);
    m_scanStatusBar->hide();

    auto scanBarLayout = new QHBoxLayout(m_scanStatusBar);
    scanBarLayout->setContentsMargins(20, 0, 20, 0);

    m_scanStatusLabel = new QLabel("Scanning filesystem tree...", this);
    m_scanStatusLabel->setStyleSheet("color: #e2e8f0; font-size: 12px; font-weight: bold;");
    scanBarLayout->addWidget(m_scanStatusLabel, 1);

    m_scanProgressBar = new QProgressBar(this);
    m_scanProgressBar->setRange(0, 0);
    m_scanProgressBar->setFixedWidth(180);
    m_scanProgressBar->setFixedHeight(12);
    scanBarLayout->addWidget(m_scanProgressBar);

    m_scanCancelBtn = new QPushButton("Cancel Scan", this);
    m_scanCancelBtn->setProperty("class", "danger-btn");
    m_scanCancelBtn->setFixedWidth(100);
    m_scanCancelBtn->setStyleSheet("padding: 4px 8px; font-size: 11px;");
    connect(m_scanCancelBtn, &QPushButton::clicked, this, &MainWindow::onCancelScanClicked);
    scanBarLayout->addWidget(m_scanCancelBtn);

    rightLayout->addWidget(m_scanStatusBar);

    // ── Stacked Widget — All 9 views ──
    m_stackedWidget = new QStackedWidget(this);

    // 0: Dashboard
    m_dashboardView = new DashboardView(this);
    connect(m_dashboardView, &DashboardView::driveSelectedForScan, this, &MainWindow::onDriveScanRequested);
    m_stackedWidget->addWidget(m_dashboardView);

    // 1: Treemap
    m_treemapView = new TreemapView(this);
    m_stackedWidget->addWidget(m_treemapView);

    // 2: Large Files
    m_largeFilesView = new LargeFilesView(this);
    m_stackedWidget->addWidget(m_largeFilesView);

    // 3: File Types
    m_fileTypesView = new FileTypesView(this);
    m_stackedWidget->addWidget(m_fileTypesView);

    // 4: Duplicates
    m_duplicatesView = new DuplicatesView(this);
    m_stackedWidget->addWidget(m_duplicatesView);

    // 5: Search / Clean (legacy combined view)
    m_searchOrganizerView = new SearchOrganizerView(this);
    m_stackedWidget->addWidget(m_searchOrganizerView);

    // 6: Advanced Search (new)
    m_advancedSearchView = new AdvancedSearchView(this);
    m_stackedWidget->addWidget(m_advancedSearchView);

    // 7: Organizer (new)
    m_organizerView = new OrganizerView(this);
    m_stackedWidget->addWidget(m_organizerView);

    // 8: Templates (new)
    m_templatesView = new TemplatesView(this);
    m_stackedWidget->addWidget(m_templatesView);

    rightLayout->addWidget(m_stackedWidget, 1);
    mainLayout->addWidget(rightPane, 1);
}

void MainWindow::applyTheme() {
    QFile file(":/styles/dark_theme.qss");
    if (file.open(QFile::ReadOnly | QFile::Text)) {
        QString styleSheet = QLatin1String(file.readAll());
        qApp->setStyleSheet(styleSheet);
        file.close();
    } else {
        qDebug() << "Warning: Could not open dark_theme.qss from resource system.";
    }
}

void MainWindow::onSidebarButtonClicked(int id) {
    m_stackedWidget->setCurrentIndex(id);
}

void MainWindow::onDriveScanRequested(const QString& drivePath) {
    if (m_scanService.isScanning()) {
        QMessageBox::warning(this, "Active Scan",
            "A scan is already in progress. Please cancel or wait for it to complete.");
        return;
    }

    m_lastScanRoot = drivePath;

    // Check if it's a network drive
    bool isNetwork = m_shell.isNetworkPath(drivePath);
    m_networkBanner->setVisible(isNetwork);

    // Show scan bar
    m_scanStatusBar->show();
    m_scanStatusLabel->setText(QString("Scanning storage tree on %1...").arg(drivePath));
    m_scanProgressBar->setRange(0, 0);

    // Determine scan depth based on drive type
    int depth = 4;
    if (isNetwork) {
        depth = SettingsManager::instance().networkScanDepth();
    } else {
        depth = SettingsManager::instance().defaultScanDepth();
    }

    m_scanService.startScan(drivePath, depth);
    SettingsManager::instance().setLastScanPath(drivePath);
    SettingsManager::instance().addRecentPath(drivePath);
}

void MainWindow::onScanProgress(const QString& currentPath, qint64 filesScanned, qint64 bytesScanned) {
    QFileInfo info(currentPath);
    QString pathSnippet = info.fileName();
    if (pathSnippet.isEmpty()) pathSnippet = currentPath;
    if (pathSnippet.length() > 30) pathSnippet = pathSnippet.left(27) + "...";

    m_scanStatusLabel->setText(QString("Indexing: %1 | Checked %2 files (%3 total size)")
                                   .arg(pathSnippet)
                                   .arg(filesScanned)
                                   .arg(formatBytes(bytesScanned)));
}

void MainWindow::onScanCompleted(bool success) {
    m_scanStatusBar->hide();

    if (!success) {
        QMessageBox::warning(this, "Scan Interrupted",
            "Scanning was cancelled or failed due to permission constraints.");
        return;
    }

    auto result = m_scanService.getScanResult();
    if (result) {
        // Load data into all views
        m_treemapView->setScanResult(result);
        m_largeFilesView->setScanResult(result);
        m_fileTypesView->setScanResult(result);
        m_duplicatesView->setScanResult(result);
        m_searchOrganizerView->setScanResult(result);

        // Set root path for new views
        m_advancedSearchView->setRootPath(m_lastScanRoot);
        m_organizerView->setRootPath(m_lastScanRoot);

        QMessageBox::information(this, "Scan Finished",
            QString("Scanning complete!\n\nIndexed %1 folders and %2 files.\nCumulative Footprint: %3.")
                .arg(m_scanService.getFoldersCount())
                .arg(m_scanService.getFilesCount())
                .arg(formatBytes(result->getSize())));

        // Navigate to Space Map
        m_sidebarGroup->button(1)->setChecked(true);
        m_stackedWidget->setCurrentIndex(1);
    }
}

void MainWindow::onCancelScanClicked() {
    m_scanService.cancelScan();
}

void MainWindow::onExportTriggered() {
    auto result = m_scanService.getScanResult();
    if (!result) {
        QMessageBox::warning(this, "No Data", "Scan a drive first before exporting.");
        return;
    }
    ExportDialog dlg(result, this);
    dlg.exec();
}

void MainWindow::closeEvent(QCloseEvent* event) {
    saveWindowState();
    event->accept();
}

void MainWindow::saveWindowState() {
    SettingsManager::instance().setWindowGeometry(saveGeometry());
    SettingsManager::instance().setWindowState(saveState());
}

void MainWindow::restoreWindowState() {
    QByteArray geo = SettingsManager::instance().windowGeometry();
    QByteArray state = SettingsManager::instance().windowState();
    if (!geo.isEmpty()) restoreGeometry(geo);
    if (!state.isEmpty()) restoreState(state);
}

QString MainWindow::formatBytes(qint64 bytes) {
    double size = bytes;
    QStringList units = {"B", "KB", "MB", "GB", "TB"};
    int u = 0;
    while (size >= 1024.0 && u < units.size() - 1) {
        size /= 1024.0;
        u++;
    }
    return QString::number(size, 'f', u > 0 ? 1 : 0) + " " + units[u];
}
