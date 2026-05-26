#ifndef FILETYPESVIEW_H
#define FILETYPESVIEW_H

#include "ui/widgets/DonutChartWidget.h"
#include "core/services/FileTypeService.h"
#include <QWidget>
#include <QTableWidget>
#include <QLabel>
#include <memory>

class FileTypesView : public QWidget {
    Q_OBJECT
public:
    explicit FileTypesView(QWidget* parent = nullptr);
    ~FileTypesView() = default;

    void setScanResult(std::shared_ptr<FileSystemNode> root);
    void clear();

private slots:
    void onCategoryHovered(const QString& name, qint64 size, double percentage);
    void onCategorySelected();

private:
    std::shared_ptr<FileSystemNode> m_rootNode;
    FileTypeService m_fileTypeService;
    QVector<CategoryStat> m_currentStats;

    DonutChartWidget* m_donutChart = nullptr;
    QTableWidget* m_categoryTable = nullptr;
    QTableWidget* m_extensionDetailsTable = nullptr;
    QLabel* m_hoverDetailsLabel = nullptr;
    QLabel* m_detailsTitleLabel = nullptr;

    void setupUI();
    void populateStats();
    void populateExtensionsForCategory(int row);
    QString formatBytes(qint64 bytes);
};

#endif // FILETYPESVIEW_H
