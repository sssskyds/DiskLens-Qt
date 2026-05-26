#ifndef SEARCHSERVICE_H
#define SEARCHSERVICE_H

#include "core/models/FileSystemNode.h"
#include <QObject>
#include <QThread>
#include <QMutex>
#include <QPointer>
#include <QDateTime>
#include <QStringList>
#include <QFileInfo>
#include <optional>
#include <memory>

/**
 * @brief Full search criteria — filters evaluated cheapest-first:
 *   name → glob → extension → hidden/exclude → size → date → category → content.
 */
struct SearchCriteria {
    QString     nameText;        ///< filename contains (plain or regex)
    QString     globPattern;     ///< glob  e.g. "*.log"
    QStringList extensions;      ///< filter by extension list
    std::optional<quint64> minSize;
    std::optional<quint64> maxSize;
    std::optional<QDateTime> modifiedAfter;
    std::optional<QDateTime> modifiedBefore;
    std::optional<QDateTime> createdAfter;
    std::optional<QDateTime> createdBefore;
    QStringList categories;      ///< e.g. {"Images", "Videos"}
    int         maxDepth    = 10;
    bool        includeHidden   = false;
    QStringList excludePatterns;
    QString     containsText;    ///< content search — expensive, runs last
    bool        regex           = false;
    bool        caseSensitive   = false;
};

class SearchWorker : public QThread {
    Q_OBJECT
public:
    SearchWorker(const QString& rootPath, const SearchCriteria& criteria,
                 QObject* parent = nullptr);
    void cancel();

signals:
    void resultsBatch(QVector<std::shared_ptr<FileSystemNode>> results);
    void progressUpdated(qint64 filesChecked, qint64 matchesFound,
                         const QString& currentPath);
    void searchFinished(qint64 totalMatches, bool wasCancelled);
    void networkDriveWarning(const QString& path);  ///< emitted when content search on network path

protected:
    void run() override;

private:
    // Iterative traversal — avoids stack overflow on deep trees
    void searchIterative();
    bool matchesCriteria(const QFileInfo& fi, const QString& category);
    bool matchesContent (const QString& filePath);
    QString classifyCategory(const QString& ext);

    QString        m_rootPath;
    SearchCriteria m_criteria;
    QMutex         m_mutex;
    bool           m_cancelRequested = false;

    qint64 m_filesChecked = 0;
    qint64 m_matchesFound = 0;
    QVector<std::shared_ptr<FileSystemNode>> m_batch;
    static constexpr int BATCH_SIZE = 50;
};

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
    void progressUpdated(qint64 filesChecked, qint64 matchesFound,
                         const QString& currentPath);
    void searchFinished(qint64 totalMatches, bool wasCancelled);
    void networkDriveWarning(const QString& path);

private:
    QPointer<SearchWorker> m_worker;
};

#endif // SEARCHSERVICE_H
