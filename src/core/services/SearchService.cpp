#include "core/services/SearchService.h"
#include "core/utils/CategoryMapper.h"
#include "platform/DriveClassificationService.h"

#include <QDirIterator>
#include <QFileInfo>
#include <QRegularExpression>
#include <QTextStream>
#include <QFile>
#include <QStack>
#include <QPair>

// ─── SearchWorker ────────────────────────────────────────────────

SearchWorker::SearchWorker(const QString& rootPath,
                           const SearchCriteria& criteria,
                           QObject* parent)
    : QThread(parent), m_rootPath(rootPath), m_criteria(criteria)
{
}

void SearchWorker::cancel() {
    QMutexLocker lock(&m_mutex);
    m_cancelRequested = true;
}

void SearchWorker::run() {
    m_filesChecked    = 0;
    m_matchesFound    = 0;
    m_cancelRequested = false;
    m_batch.clear();

    // Warn immediately if content search is requested on a network path —
    // do not silently run it, but do not block the whole search either.
    if (!m_criteria.containsText.isEmpty() &&
        DriveClassificationService::isNetworkPath(m_rootPath))
    {
        emit networkDriveWarning(m_rootPath);
        // Remove content filter; continue with cheaper filters only
        m_criteria.containsText.clear();
    }

    searchIterative();

    // Flush remaining batch
    if (!m_batch.isEmpty()) {
        emit resultsBatch(m_batch);
        m_batch.clear();
    }

    bool cancelled;
    { QMutexLocker lock(&m_mutex); cancelled = m_cancelRequested; }
    emit searchFinished(m_matchesFound, cancelled);
}

void SearchWorker::searchIterative() {
    // Iterative depth-first traversal using an explicit stack.
    // This avoids stack overflow on deeply nested directory trees (depth >500).
    struct WorkItem {
        QString path;
        int     depth;
    };
    QStack<WorkItem> stack;
    stack.push({m_rootPath, 0});

    while (!stack.isEmpty()) {
        { QMutexLocker lock(&m_mutex); if (m_cancelRequested) return; }

        const WorkItem item = stack.pop();
        if (item.depth > m_criteria.maxDepth) continue;

        QDir dir(item.path);
        if (!dir.exists()) continue;

        QDir::Filters filters = QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot;
        if (m_criteria.includeHidden) filters |= QDir::Hidden;

        const QFileInfoList entries = dir.entryInfoList(filters, QDir::DirsFirst);
        for (const QFileInfo& fi : entries) {
            { QMutexLocker lock(&m_mutex); if (m_cancelRequested) return; }

            // Exclude patterns
            bool excluded = false;
            for (const QString& pat : m_criteria.excludePatterns) {
                QRegularExpression rx(
                    QRegularExpression::wildcardToRegularExpression(pat),
                    QRegularExpression::CaseInsensitiveOption);
                if (rx.match(fi.fileName()).hasMatch()) { excluded = true; break; }
            }
            if (excluded) continue;

            if (fi.isDir()) {
                stack.push({fi.absoluteFilePath(), item.depth + 1});
                continue;
            }

            m_filesChecked++;
            const QString category = CategoryMapper::classify(fi.suffix().toLower());

            if (matchesCriteria(fi, category)) {
                auto node = std::make_shared<FileSystemNode>(
                    fi.fileName(), fi.absoluteFilePath(), false);
                node->setSize(fi.size());
                node->setLastModified(fi.lastModified());
                node->setExtension(fi.suffix().toLower());
                node->setCategory(category);

                m_matchesFound++;
                m_batch.append(node);

                if (m_batch.size() >= BATCH_SIZE) {
                    emit resultsBatch(m_batch);
                    m_batch.clear();
                }
            }

            if (m_filesChecked % 500 == 0)
                emit progressUpdated(m_filesChecked, m_matchesFound,
                                     fi.absolutePath());
        }
    }
}

bool SearchWorker::matchesCriteria(const QFileInfo& fi, const QString& category) {
    // Filters in cheapest-first order ────────────────────────────────

    // 1. Name text
    if (!m_criteria.nameText.isEmpty()) {
        const Qt::CaseSensitivity cs = m_criteria.caseSensitive
                                           ? Qt::CaseSensitive : Qt::CaseInsensitive;
        if (m_criteria.regex) {
            QRegularExpression::PatternOptions opts =
                m_criteria.caseSensitive ? QRegularExpression::NoPatternOption
                                        : QRegularExpression::CaseInsensitiveOption;
            if (!QRegularExpression(m_criteria.nameText, opts)
                     .match(fi.fileName()).hasMatch()) return false;
        } else {
            if (!fi.fileName().contains(m_criteria.nameText, cs)) return false;
        }
    }

    // 2. Glob pattern
    if (!m_criteria.globPattern.isEmpty()) {
        QRegularExpression rx(
            QRegularExpression::wildcardToRegularExpression(m_criteria.globPattern),
            QRegularExpression::CaseInsensitiveOption);
        if (!rx.match(fi.fileName()).hasMatch()) return false;
    }

    // 3. Extension list
    if (!m_criteria.extensions.isEmpty()) {
        const QString ext = fi.suffix().toLower();
        bool found = false;
        for (const QString& e : m_criteria.extensions)
            if (e.compare(ext, Qt::CaseInsensitive) == 0) { found = true; break; }
        if (!found) return false;
    }

    // 4. Size
    if (m_criteria.minSize.has_value() && fi.size() < (qint64)m_criteria.minSize.value()) return false;
    if (m_criteria.maxSize.has_value() && fi.size() > (qint64)m_criteria.maxSize.value()) return false;

    // 5. Dates
    if (m_criteria.modifiedAfter.has_value()  && fi.lastModified() < m_criteria.modifiedAfter.value())  return false;
    if (m_criteria.modifiedBefore.has_value() && fi.lastModified() > m_criteria.modifiedBefore.value()) return false;
    if (m_criteria.createdAfter.has_value()  && fi.birthTime().isValid()
            && fi.birthTime() < m_criteria.createdAfter.value())  return false;
    if (m_criteria.createdBefore.has_value() && fi.birthTime().isValid()
            && fi.birthTime() > m_criteria.createdBefore.value()) return false;

    // 6. Category
    if (!m_criteria.categories.isEmpty())
        if (!m_criteria.categories.contains(category, Qt::CaseInsensitive)) return false;

    // 7. Content (most expensive — last)
    if (!m_criteria.containsText.isEmpty())
        if (!matchesContent(fi.absoluteFilePath())) return false;

    return true;
}

bool SearchWorker::matchesContent(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return false;

    constexpr qint64 MAX_BYTES = 10 * 1024 * 1024; // 10 MB cap
    QTextStream stream(&file);
    qint64 bytesRead = 0;

    while (!stream.atEnd() && bytesRead < MAX_BYTES) {
        const QString line = stream.readLine();
        bytesRead += line.size() + 1;
        if (m_criteria.regex) {
            QRegularExpression::PatternOptions opts =
                m_criteria.caseSensitive ? QRegularExpression::NoPatternOption
                                        : QRegularExpression::CaseInsensitiveOption;
            if (QRegularExpression(m_criteria.containsText, opts)
                    .match(line).hasMatch()) return true;
        } else {
            const Qt::CaseSensitivity cs = m_criteria.caseSensitive
                                               ? Qt::CaseSensitive : Qt::CaseInsensitive;
            if (line.contains(m_criteria.containsText, cs)) return true;
        }
    }
    return false;
}

// classifyCategory now delegates to the shared CategoryMapper
QString SearchWorker::classifyCategory(const QString& ext) {
    return CategoryMapper::classify(ext);
}

// ─── SearchService ───────────────────────────────────────────────

SearchService::SearchService(QObject* parent) : QObject(parent) {}

SearchService::~SearchService() { cancelSearch(); }

void SearchService::startSearch(const QString& rootPath,
                                const SearchCriteria& criteria)
{
    cancelSearch();

    auto* worker = new SearchWorker(rootPath, criteria);
    m_worker = worker;
    connect(worker, &SearchWorker::resultsBatch,       this, &SearchService::resultsBatch);
    connect(worker, &SearchWorker::progressUpdated,    this, &SearchService::progressUpdated);
    connect(worker, &SearchWorker::searchFinished,     this, &SearchService::searchFinished);
    connect(worker, &SearchWorker::networkDriveWarning,this, &SearchService::networkDriveWarning);
    connect(worker, &SearchWorker::finished, worker,   &QObject::deleteLater);
    worker->start();
}

void SearchService::cancelSearch() {
    if (!m_worker.isNull() && m_worker->isRunning()) {
        m_worker->cancel();
        m_worker->wait(5000);
    }
}

bool SearchService::isSearching() const {
    return !m_worker.isNull() && m_worker->isRunning();
}
