#ifndef LARGEFILESERVICE_H
#define LARGEFILESERVICE_H

#include "core/models/FileSystemNode.h"
#include <QVector>
#include <QObject>
#include <memory>

struct FileInfoSummary {
    QString name;
    QString path;
    qint64 size = 0;
    QDateTime lastModified;
    QString category;
};

class LargeFileService : public QObject {
    Q_OBJECT
public:
    explicit LargeFileService(QObject* parent = nullptr);
    ~LargeFileService() = default;

    QVector<FileInfoSummary> findLargeFiles(std::shared_ptr<FileSystemNode> root, int limit = 100);

private:
    void collectFiles(std::shared_ptr<FileSystemNode> node, QVector<FileInfoSummary>& files);
};

#endif // LARGEFILESERVICE_H
