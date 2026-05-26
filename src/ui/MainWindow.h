#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "ui/views/DashboardView.h"
#include "ui/views/TreemapView.h"
#include "ui/views/LargeFilesView.h"
#include "ui/views/FileTypesView.h"
#include "ui/views/DuplicatesView.h"
#include "ui/views/SearchOrganizerView.h"
#include "ui/views/AdvancedSearchView.h"
#include "ui/views/OrganizerView.h"
#include "ui/views/TemplatesView.h"
#include "core/services/ScanService.h"
#include "platform/ShellIntegrationService.h"

#include <QMainWindow>
#include <QStackedWidget>
#include <QPushButton>
#include <QButtonGroup>
#include <QLabel>
#include <QProgressBar>
#include <QLineEdit>

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() = default;

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void onSidebarButtonClicked(int id);
    void onDriveScanRequested(const QString& drivePath);

    // Global directory toolbar
    void onBrowseDirectory();
    void onGlobalScanClicked();

    // Scan Progress callbacks
    void onScanProgress(const QString& currentPath, qint64 filesScanned, qint64 bytesScanned);
    void onScanCompleted(bool success);
    void onCancelScanClicked();

    // Menu actions
    void onExportTriggered();

private:
    void setupUI();
    void applyTheme();
    void setupMenuBar();
    void saveWindowState();
    void restoreWindowState();
    QString formatBytes(qint64 bytes);

    QStackedWidget* m_stackedWidget = nullptr;
    QButtonGroup* m_sidebarGroup = nullptr;

    // Views — Original 6
    DashboardView* m_dashboardView = nullptr;
    TreemapView* m_treemapView = nullptr;
    LargeFilesView* m_largeFilesView = nullptr;
    FileTypesView* m_fileTypesView = nullptr;
    DuplicatesView* m_duplicatesView = nullptr;
    SearchOrganizerView* m_searchOrganizerView = nullptr;

    // Views — New modules
    AdvancedSearchView* m_advancedSearchView = nullptr;
    OrganizerView* m_organizerView = nullptr;
    TemplatesView* m_templatesView = nullptr;

    // Scan engine
    ScanService m_scanService;
    ShellIntegrationService m_shell;

    // Network drive banner
    QFrame* m_networkBanner = nullptr;

    // Global directory toolbar (always visible)
    QFrame* m_dirToolbar = nullptr;
    QLineEdit* m_pathInput = nullptr;
    QPushButton* m_browseBtn = nullptr;
    QPushButton* m_globalScanBtn = nullptr;

    // Global scanning overlay/indicator
    QFrame* m_scanStatusBar = nullptr;
    QProgressBar* m_scanProgressBar = nullptr;
    QLabel* m_scanStatusLabel = nullptr;
    QPushButton* m_scanCancelBtn = nullptr;

    // Track last scan root
    QString m_lastScanRoot;
};

#endif // MAINWINDOW_H
