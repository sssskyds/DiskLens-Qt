#ifndef ADVANCEDSEARCHVIEW_H
#define ADVANCEDSEARCHVIEW_H

#include "core/services/SearchService.h"
#include "core/models/FileSystemNode.h"
#include "platform/ShellIntegrationService.h"
#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QDateTimeEdit>
#include <QCheckBox>
#include <QTableWidget>
#include <QPushButton>
#include <QLabel>
#include <QProgressBar>
#include <memory>

class AdvancedSearchView : public QWidget {
    Q_OBJECT
public:
    explicit AdvancedSearchView(QWidget* parent = nullptr);
    void setRootPath(const QString& path);
    void clear();

private slots:
    void onSearchClicked();
    void onCancelClicked();
    void onResultsBatch(QVector<std::shared_ptr<FileSystemNode>> results);
    void onProgressUpdated(qint64 filesChecked, qint64 matchesFound, const QString& currentPath);
    void onSearchFinished(qint64 totalMatches, bool wasCancelled);
    void onResultDoubleClicked(int row, int col);
    void onContextMenu(const QPoint& pos);

private:
    void setupUI();
    void setupFilterPanel();
    void setupResultsTable();
    SearchCriteria buildCriteria();
    QString formatBytes(qint64 bytes);

    QString m_rootPath;
    SearchService m_searchService;
    ShellIntegrationService m_shell;

    // Filter controls
    QLineEdit*     m_nameEdit = nullptr;
    QLineEdit*     m_globEdit = nullptr;
    QLineEdit*     m_extensionsEdit = nullptr;
    QSpinBox*      m_minSizeMB = nullptr;
    QSpinBox*      m_maxSizeMB = nullptr;
    QDateTimeEdit* m_modifiedAfter = nullptr;
    QDateTimeEdit* m_modifiedBefore = nullptr;
    QComboBox*     m_categoryCombo = nullptr;
    QSpinBox*      m_depthSpin = nullptr;
    QCheckBox*     m_includeHidden = nullptr;
    QCheckBox*     m_caseSensitive = nullptr;
    QCheckBox*     m_regexCheck = nullptr;
    QLineEdit*     m_contentEdit = nullptr;
    QLineEdit*     m_excludeEdit = nullptr;

    // Results
    QTableWidget*  m_resultsTable = nullptr;
    QLabel*        m_statusLabel = nullptr;
    QProgressBar*  m_progressBar = nullptr;
    QPushButton*   m_searchBtn = nullptr;
    QPushButton*   m_cancelBtn = nullptr;

    QVector<std::shared_ptr<FileSystemNode>> m_allResults;
};

#endif // ADVANCEDSEARCHVIEW_H
