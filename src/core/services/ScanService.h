#ifndef SCANSERVICE_H
#define SCANSERVICE_H

#include "core/models/FileSystemNode.h"
#include <QObject>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <memory>

class ScanWorker : public QThread {
    Q_OBJECT
public:
    ScanWorker(const QString& rootPath, int maxDepth, QObject* parent = nullptr);
    ~ScanWorker() = default;

    void cancel();
    bool isCancelled();

    std::shared_ptr<FileSystemNode> getResult() const { return m_rootNode; }
    qint64 getFileCount() const { return m_fileCount; }
    qint64 getFolderCount() const { return m_folderCount; }

signals:
    void progressUpdated(const QString& currentPath, qint64 filesScanned, qint64 bytesScanned);
    void scanFinished(bool success);

protected:
    void run() override;

private:
    void scanDirectory(const QString& path, std::shared_ptr<FileSystemNode>& parentNode, int currentDepth);
    QString classifyCategory(const QString& extension);

    QString m_rootPath;
    int m_maxDepth;
    std::shared_ptr<FileSystemNode> m_rootNode;
    qint64 m_fileCount = 0;
    qint64 m_folderCount = 0;
    qint64 m_bytesScanned = 0;
    
    QMutex m_mutex;
    bool m_cancelRequested = false;
};

class ScanService : public QObject {
    Q_OBJECT
public:
    explicit ScanService(QObject* parent = nullptr);
    ~ScanService();

    void startScan(const QString& path, int maxDepth = 4);
    void cancelScan();
    bool isScanning() const { return m_worker && m_worker->isRunning(); }

    std::shared_ptr<FileSystemNode> getScanResult() const;
    qint64 getFilesCount() const;
    qint64 getFoldersCount() const;

signals:
    void progressReported(const QString& currentPath, qint64 filesScanned, qint64 bytesScanned);
    void scanCompleted(bool success);

private slots:
    void onWorkerFinished();

private:
    ScanWorker* m_worker = nullptr;
};

#endif // SCANSERVICE_H
