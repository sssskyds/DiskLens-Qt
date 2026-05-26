#ifndef DUPLICATESERVICE_H
#define DUPLICATESERVICE_H

#include "core/models/FileSystemNode.h"
#include <QObject>
#include <QVector>
#include <QThread>
#include <QMutex>
#include <memory>

struct DuplicateGroup {
    qint64 fileSize = 0;
    QString checksum;
    QVector<QString> filePaths;
};

class DuplicateWorker : public QThread {
    Q_OBJECT
public:
    DuplicateWorker(std::shared_ptr<FileSystemNode> root, QObject* parent = nullptr);
    ~DuplicateWorker() = default;

    void cancel();
    bool isCancelled();

    QVector<DuplicateGroup> getResult() const { return m_duplicates; }
    qint64 getWastedSpace() const { return m_wastedSpace; }

signals:
    void progressUpdated(const QString& currentFile, int processedCount, int totalCount);
    void searchFinished(bool success);

protected:
    void run() override;

private:
    void collectAllFiles(std::shared_ptr<FileSystemNode> node, QVector<std::shared_ptr<FileSystemNode>>& fileList);
    QString calculateChecksum(const QString& filePath, qint64 size);

    std::shared_ptr<FileSystemNode> m_root;
    QVector<DuplicateGroup> m_duplicates;
    qint64 m_wastedSpace = 0;

    QMutex m_mutex;
    bool m_cancelRequested = false;
};

class DuplicateService : public QObject {
    Q_OBJECT
public:
    explicit DuplicateService(QObject* parent = nullptr);
    ~DuplicateService();

    void startSearch(std::shared_ptr<FileSystemNode> root);
    void cancelSearch();
    bool isSearching() const { return m_worker && m_worker->isRunning(); }

    QVector<DuplicateGroup> getDuplicates() const;
    qint64 getWastedSpace() const;

signals:
    void progressReported(const QString& currentFile, int processedCount, int totalCount);
    void searchCompleted(bool success);

private slots:
    void onWorkerFinished();

private:
    DuplicateWorker* m_worker = nullptr;
};

#endif // DUPLICATESERVICE_H
