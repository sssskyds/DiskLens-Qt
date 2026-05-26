#ifndef SEARCHORGANIZERVIEW_H
#define SEARCHORGANIZERVIEW_H

#include "core/models/FileSystemNode.h"
#include "core/services/FileOperationService.h"
#include "core/services/OrganizerService.h"
#include <QWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <memory>

class SearchOrganizerView : public QWidget {
    Q_OBJECT
public:
    explicit SearchOrganizerView(QWidget* parent = nullptr);
    ~SearchOrganizerView() = default;

    void setScanResult(std::shared_ptr<FileSystemNode> root);
    void clear();

private slots:
    // Search Actions
    void onSearchQueryChanged(const QString& query);
    void onSearchTableDoubleClicked(int row, int col);

    // Organizer Actions
    void onPreviewOrganizer();
    void onApplyOrganizer();

    // Export Actions
    void onExportCsv();
    void onExportMarkdown();

private:
    std::shared_ptr<FileSystemNode> m_rootNode;
    FileOperationService m_fileOpService;
    
    // UI elements: Search
    QLineEdit* m_searchEdit = nullptr;
    QTableWidget* m_searchResultsTable = nullptr;
    QLabel* m_searchCountLabel = nullptr;
    QVector<std::shared_ptr<FileSystemNode>> m_matchingNodes;

    // UI elements: Organizer
    QComboBox* m_organizerRuleCombo = nullptr;
    QTableWidget* m_organizerPreviewTable = nullptr;
    QPushButton* m_organizeApplyBtn = nullptr;
    QVector<OrganizeAction> m_pendingActions;

    // Helpers
    void setupUI();
    void recursiveSearch(std::shared_ptr<FileSystemNode> node, const QString& query);
    void collectOrganizerActions(std::shared_ptr<FileSystemNode> node, const QString& ruleType, QVector<OrganizeAction>& actions);
    void exportToCsvFile(const QString& filePath);
    void exportToMarkdownFile(const QString& filePath);
    void collectAllFilesCsv(std::shared_ptr<FileSystemNode> node, QTextStream& stream);
    void collectAllFilesMarkdown(std::shared_ptr<FileSystemNode> node, QTextStream& stream, int indent);
    QString formatBytes(qint64 bytes);
};

#endif // SEARCHORGANIZERVIEW_H
