#ifndef DUPLICATESVIEW_H
#define DUPLICATESVIEW_H

#include "core/services/DuplicateService.h"
#include "core/services/FileOperationService.h"
#include <QWidget>
#include <QTreeWidget>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>
#include <memory>

class DuplicatesView : public QWidget {
    Q_OBJECT
public:
    explicit DuplicatesView(QWidget* parent = nullptr);
    ~DuplicatesView() = default;

    void setScanResult(std::shared_ptr<FileSystemNode> root);
    void clear();

private slots:
    void onSearchStart();
    void onSearchProgress(const QString& currentFile, int processedCount, int totalCount);
    void onSearchCompleted(bool success);

    void onSelectAllDuplicates();
    void onDeleteSelected();
    void onTreeItemChanged(QTreeWidgetItem* item, int column);

private:
    std::shared_ptr<FileSystemNode> m_rootNode;
    DuplicateService m_duplicateService;
    FileOperationService m_fileOpService;
    QVector<DuplicateGroup> m_duplicateGroups;

    QTreeWidget* m_treeWidget = nullptr;
    QLabel* m_wastedSpaceLabel = nullptr;
    QPushButton* m_scanBtn = nullptr;
    QPushButton* m_selectAllBtn = nullptr;
    QPushButton* m_deleteBtn = nullptr;
    
    QFrame* m_progressContainer = nullptr;
    QProgressBar* m_progressBar = nullptr;
    QLabel* m_progressLabel = nullptr;

    void setupUI();
    void populateTree();
    QString formatBytes(qint64 bytes);
};

#endif // DUPLICATESVIEW_H
