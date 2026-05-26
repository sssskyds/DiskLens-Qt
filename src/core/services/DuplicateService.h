#ifndef DUPLICATESERVICE_H
#define DUPLICATESERVICE_H

#include "core/models/FileSystemNode.h"
#include <QObject>
#include <QVector>
#include <QThread>
#include <QMutex>
#include <QPointer>
#include <memory>

/**
 * @brief Represents a confirmed group of identical files.
 *
 * @note quickHash is the partial-hash pre-filter result.
 *       fullHash  is the definitive SHA-256 used for confirmation.
 *       Only entries with matching fullHash are reported as duplicates.
 */
struct DuplicateGroup {
    qint64  fileSize  = 0;
    QString quickHash;
    QString fullHash;          ///< SHA-256 — confirmed identical
    QVector<QString> filePaths;
};

class DuplicateWorker : public QThread {
    Q_OBJECT
public:
    DuplicateWorker(std::shared_ptr<FileSystemNode> root, QObject* parent = nullptr);
    ~DuplicateWorker() = default;

    void cancel();
    bool isCancelled();

    QVector<DuplicateGroup> getResult()     const { return m_duplicates; }
    qint64                  getWastedSpace() const { return m_wastedSpace; }

signals:
    void progressUpdated(const QString& currentFile, int processedCount, int totalCount);
    void searchFinished(bool success);

protected:
    void run() override;

private:
    void collectAllFiles(std::shared_ptr<FileSystemNode> node,
                         QVector<std::shared_ptr<FileSystemNode>>& fileList);
    QString computePartialHash(const QString& filePath, qint64 size);
    QString computeFullHash   (const QString& filePath);

    std::shared_ptr<FileSystemNode> m_root;
    QVector<DuplicateGroup> m_duplicates;
    qint64 m_wastedSpace = 0;

    QMutex m_mutex;
    bool   m_cancelRequested = false;

    static constexpr qint64 CHUNK_SIZE      = 64 * 1024;   // 64 KB
    static constexpr qint64 PARTIAL_THRESH  = 4  * 1024 * 1024; // 4 MB
};

class DuplicateService : public QObject {
    Q_OBJECT
public:
    explicit DuplicateService(QObject* parent = nullptr);
    ~DuplicateService();

    void startSearch(std::shared_ptr<FileSystemNode> root);
    void cancelSearch();
    bool isSearching() const;

    QVector<DuplicateGroup> getDuplicates()  const { return m_cachedDuplicates; }
    qint64                  getWastedSpace() const { return m_cachedWastedSpace; }

signals:
    void progressReported(const QString& currentFile, int processedCount, int totalCount);
    void searchCompleted(bool success);

private slots:
    void onWorkerFinished(bool success);

private:
    QPointer<DuplicateWorker> m_worker;
    QVector<DuplicateGroup>   m_cachedDuplicates;
    qint64                    m_cachedWastedSpace = 0;
};

#endif // DUPLICATESERVICE_H
