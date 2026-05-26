#ifndef SEARCHSERVICE_H
#define SEARCHSERVICE_H

#include "core/models/FileSystemNode.h"
#include <QObject>
#include <QThread>
#include <QMutex>
#include <QDateTime>
#include <QStringList>
#include <QFileInfo>
#include <optional>
#include <memory>

/**
 * @brief Full search criteria with all supported filters.
 * Filters are evaluated cheapest-first: name → extension → hidden →
 * size → date → category → content (most expensive, last).
 */
struct SearchCriteria {
    QString nameText;                          // filename contains
    QString globPattern;                       // glob match (e.g., "*.log")
    QStringList extensions;                    // filter by extensions
    std::optional<quint64> minSize;
    std::optional<quint64> maxSize;
    std::optional<QDateTime> modifiedAfter;
    std::optional<QDateTime> modifiedBefore;
    std::optional<QDateTime> createdAfter;
    std::optional<QDateTime> createdBefore;
    QStringList categories;                    // e.g., "Images", "Videos"
    int maxDepth = 10;
    bool includeHidden = false;
    QStringList excludePatterns;
    QString containsText;                      // content search (text files only)
    bool regex = false;
    bool caseSensitive = false;
};

/**
 * @brief Worker thread that performs the actual search traversal.
 * Streams results back in batches via signals.
 */
class SearchWorker : public QThread {
    Q_OBJECT
public:
    SearchWorker(const QString& rootPath, const SearchCriteria& criteria, QObject* parent = nullptr);
    void cancel();

signals:
    void resultsBatch(QVector<std::shared_ptr<FileSystemNode>> results);
    void progressUpdated(qint64 filesChecked, qint64 matchesFound, const QString& currentPath);
    void searchFinished(qint64 totalMatches, bool wasCancelled);

protected:
    void run() override;

private:
    void searchDirectory(const QString& path, int depth);
    bool matchesCriteria(const QFileInfo& fi, const QString& category);
    bool matchesContent(const QString& filePath);
    QString classifyCategory(const QString& ext);

    QString m_rootPath;
    SearchCriteria m_criteria;
    QMutex m_mutex;
    bool m_cancelRequested = false;

    qint64 m_filesChecked = 0;
    qint64 m_matchesFound = 0;
    QVector<std::shared_ptr<FileSystemNode>> m_batch;
    static const int BATCH_SIZE = 50;
};

/**
 * @brief Service facade for advanced search.
 * Manages the worker thread lifecycle.
 */
class SearchService : public QObject {
    Q_OBJECT
public:
    explicit SearchService(QObject* parent = nullptr);
    ~SearchService();

    void startSearch(const QString& rootPath, const SearchCriteria& criteria);
    void cancelSearch();
    bool isSearching() const;

signals:
    void resultsBatch(QVector<std::shared_ptr<FileSystemNode>> results);
    void progressUpdated(qint64 filesChecked, qint64 matchesFound, const QString& currentPath);
    void searchFinished(qint64 totalMatches, bool wasCancelled);

private:
    SearchWorker* m_worker = nullptr;
};

#endif // SEARCHSERVICE_H
