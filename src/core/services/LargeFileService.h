#ifndef LARGEFILESERVICE_H
#define LARGEFILESERVICE_H

#include "core/models/FileSystemNode.h"
#include <QObject>
#include <QThread>
#include <QMutex>
#include <QPointer>
#include <QVector>
#include <QDateTime>
#include <memory>

struct FileInfoSummary {
    QString  name;
    QString  path;
    qint64   size = 0;
    QDateTime lastModified;
    QString  category;
    QString  extension;
};

/**
 * @brief Worker that collects the top-N largest files using a bounded
 *        min-heap, so memory usage is O(limit) rather than O(all files).
 */
class LargeFileWorker : public QThread {
    Q_OBJECT
public:
    LargeFileWorker(std::shared_ptr<FileSystemNode> root, int limit,
                    QObject* parent = nullptr);

    void cancel();
    bool isCancelled();

    QVector<FileInfoSummary> getResult() const { return m_results; }

signals:
    void progressUpdated(qint64 filesChecked);
    void searchFinished(bool success);

protected:
    void run() override;

private:
    void traverse(std::shared_ptr<FileSystemNode> node);

    std::shared_ptr<FileSystemNode> m_root;
    int  m_limit;
    QVector<FileInfoSummary> m_results;  // acts as min-heap
    qint64 m_checked = 0;

    QMutex m_mutex;
    bool   m_cancelRequested = false;

    static constexpr qint64 PROGRESS_INTERVAL = 1000;
};

class LargeFileService : public QObject {
    Q_OBJECT
public:
    explicit LargeFileService(QObject* parent = nullptr);
    ~LargeFileService();

    void startSearch(std::shared_ptr<FileSystemNode> root, int limit = 100);
    void cancelSearch();
    bool isSearching() const;

    QVector<FileInfoSummary> getResults() const { return m_cachedResults; }

signals:
    void progressUpdated(qint64 filesChecked);
    void searchCompleted(bool success);

private slots:
    void onWorkerFinished(bool success);

private:
    QPointer<LargeFileWorker> m_worker;
    QVector<FileInfoSummary>  m_cachedResults;
};

#endif // LARGEFILESERVICE_H
