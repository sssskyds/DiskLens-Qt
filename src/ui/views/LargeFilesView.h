#ifndef LARGEFILESVIEW_H
#define LARGEFILESVIEW_H

#include "core/services/LargeFileService.h"
#include "core/services/FileOperationService.h"
#include <QWidget>
#include <QTableWidget>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>

class LargeFilesView : public QWidget {
    Q_OBJECT
public:
    explicit LargeFilesView(QWidget* parent = nullptr);
    ~LargeFilesView() = default;

    void setScanResult(std::shared_ptr<FileSystemNode> root);
    void clear();

private slots:
    void onLimitChanged(int index);
    void onCustomContextMenu(const QPoint& pos);
    void onTableSelectionChanged();
    void onRevealClicked();
    void onDeleteClicked();

private:
    std::shared_ptr<FileSystemNode> m_rootNode;
    LargeFileService m_largeFileService;
    FileOperationService m_fileOpService;
    QVector<FileInfoSummary> m_currentFiles;

    QTableWidget* m_tableWidget = nullptr;
    QComboBox* m_limitCombo = nullptr;
    QPushButton* m_revealBtn = nullptr;
    QPushButton* m_deleteBtn = nullptr;
    QLabel* m_selectedDetailsLabel = nullptr;

    void setupUI();
    void populateTable();
    QString formatBytes(qint64 bytes);
};

#endif // LARGEFILESVIEW_H
