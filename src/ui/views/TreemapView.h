#ifndef TREEMAPVIEW_H
#define TREEMAPVIEW_H

#include "ui/widgets/TreemapWidget.h"
#include "core/services/FileOperationService.h"
#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <memory>

class TreemapView : public QWidget {
    Q_OBJECT
public:
    explicit TreemapView(QWidget* parent = nullptr);
    ~TreemapView() = default;

    void setScanResult(std::shared_ptr<FileSystemNode> root);
    void clear();

private slots:
    void onNodeHovered(std::shared_ptr<FileSystemNode> node);
    void onNodeClicked(std::shared_ptr<FileSystemNode> node);
    void onNodeDoubleClicked(std::shared_ptr<FileSystemNode> node);
    void onRevealClicked();
    void onDeleteClicked();
    void onColorModeChanged(int index);

private:
    TreemapWidget* m_treemapWidget = nullptr;
    FileOperationService m_fileOpService;
    std::shared_ptr<FileSystemNode> m_selectedNode;

    QLabel* m_hoveredPathLabel = nullptr;
    QLabel* m_selectedNameLabel = nullptr;
    QLabel* m_selectedSizeLabel = nullptr;
    QLabel* m_selectedTypeLabel = nullptr;
    QPushButton* m_revealBtn = nullptr;
    QPushButton* m_deleteBtn = nullptr;

    void setupUI();
    QString formatBytes(qint64 bytes);
};

#endif // TREEMAPVIEW_H
