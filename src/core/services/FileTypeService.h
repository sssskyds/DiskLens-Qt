#ifndef FILETYPESERVICE_H
#define FILETYPESERVICE_H

#include "core/models/FileSystemNode.h"
#include <QObject>
#include <QHash>
#include <QVector>
#include <memory>

struct ExtensionStat {
    QString ext;
    qint64 size = 0;
    qint64 count = 0;
};

struct CategoryStat {
    QString categoryName;
    qint64 totalSize = 0;
    qint64 fileCount = 0;
    double percentage = 0.0;
    QString color;
    QVector<ExtensionStat> extensionDetails;
};

class FileTypeService : public QObject {
    Q_OBJECT
public:
    explicit FileTypeService(QObject* parent = nullptr);
    ~FileTypeService() = default;

    QVector<CategoryStat> analyzeFileTypes(std::shared_ptr<FileSystemNode> root);

private:
    void aggregateNode(std::shared_ptr<FileSystemNode> node, 
                       QHash<QString, qint64>& catSizes, 
                       QHash<QString, qint64>& catCounts,
                       QHash<QString, QHash<QString, ExtensionStat>>& extDetails);
};

#endif // FILETYPESERVICE_H
