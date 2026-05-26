#include "DuplicateService.h"
#include <QFile>
#include <QCryptographicHash>
#include <QHash>
#include <QDebug>
#include <algorithm>

// ─── DuplicateWorker ─────────────────────────────────────────────

DuplicateWorker::DuplicateWorker(std::shared_ptr<FileSystemNode> root, QObject* parent)
    : QThread(parent), m_root(root)
{
}

void DuplicateWorker::cancel() {
    QMutexLocker locker(&m_mutex);
    m_cancelRequested = true;
}

bool DuplicateWorker::isCancelled() {
    QMutexLocker locker(&m_mutex);
    return m_cancelRequested;
}

void DuplicateWorker::run() {
    m_duplicates.clear();
    m_wastedSpace     = 0;
    m_cancelRequested = false;

    if (!m_root) { emit searchFinished(false); return; }

    // ── Stage 1: collect all file nodes ──────────────────────────
    QVector<std::shared_ptr<FileSystemNode>> allFiles;
    collectAllFiles(m_root, allFiles);
    if (isCancelled()) { emit searchFinished(false); return; }

    // ── Stage 2: group by exact file size (O(1) per file) ────────
    QHash<qint64, QVector<QString>> sizeGroups;
    for (const auto& node : allFiles) {
        if (node->getSize() > 0)          // ignore empty files
            sizeGroups[node->getSize()].push_back(node->getPath());
    }

    // Keep only sizes with multiple candidates
    QVector<QPair<qint64, QVector<QString>>> candidates;
    candidates.reserve(sizeGroups.size());
    for (auto it = sizeGroups.begin(); it != sizeGroups.end(); ++it) {
        if (it.value().size() > 1)
            candidates.append({it.key(), it.value()});
    }
    if (candidates.isEmpty()) { emit searchFinished(true); return; }

    // Sort largest first so wasted-space estimates stay meaningful during scan
    std::sort(candidates.begin(), candidates.end(),
              [](const auto& a, const auto& b){ return a.first > b.first; });

    int totalCandidates = 0;
    for (const auto& p : candidates) totalCandidates += p.second.size();
    int processed = 0;

    // ── Stage 3a: quick partial-hash pre-filter ───────────────────
    // Groups surviving this stage are still CANDIDATES, not confirmed dupes.
    QVector<QPair<qint64, QVector<QString>>> quickMatches;
    for (const auto& [size, paths] : candidates) {
        if (isCancelled()) { emit searchFinished(false); return; }

        QHash<QString, QVector<QString>> quickGroups;
        for (const QString& p : paths) {
            if (isCancelled()) { emit searchFinished(false); return; }
            const QString h = computePartialHash(p, size);
            if (!h.isEmpty()) quickGroups[h].push_back(p);
            processed++;
            if (processed % 10 == 0)
                emit progressUpdated(p, processed, totalCandidates * 2);
        }
        for (auto it = quickGroups.begin(); it != quickGroups.end(); ++it) {
            if (it.value().size() > 1)
                quickMatches.append({size, it.value()});
        }
    }
    if (quickMatches.isEmpty()) { emit searchFinished(true); return; }

    // ── Stage 3b: full SHA-256 confirmation ───────────────────────
    // Only files that survived the partial-hash filter are fully hashed.
    // This is the ONLY stage that produces confirmed duplicate groups.
    int fullTotal = 0;
    for (const auto& p : quickMatches) fullTotal += p.second.size();
    int fullProcessed = 0;

    for (const auto& [size, paths] : quickMatches) {
        if (isCancelled()) { emit searchFinished(false); return; }

        QHash<QString, QVector<QString>> fullGroups;
        for (const QString& p : paths) {
            if (isCancelled()) { emit searchFinished(false); return; }
            const QString h = computeFullHash(p);
            if (!h.isEmpty()) fullGroups[h].push_back(p);
            fullProcessed++;
            emit progressUpdated(p,
                                 totalCandidates + fullProcessed,
                                 totalCandidates * 2);
        }
        for (auto it = fullGroups.begin(); it != fullGroups.end(); ++it) {
            if (it.value().size() > 1) {
                DuplicateGroup group;
                group.fileSize  = size;
                group.fullHash  = it.key();
                group.filePaths = it.value();
                m_duplicates.push_back(group);
                m_wastedSpace += size * static_cast<qint64>(it.value().size() - 1);
            }
        }
    }

    // Sort by wasted space descending for best UX
    std::sort(m_duplicates.begin(), m_duplicates.end(),
              [](const DuplicateGroup& a, const DuplicateGroup& b) {
                  return (a.fileSize * (a.filePaths.size()-1))
                       > (b.fileSize * (b.filePaths.size()-1));
              });

    emit searchFinished(true);
}

void DuplicateWorker::collectAllFiles(
    std::shared_ptr<FileSystemNode> node,
    QVector<std::shared_ptr<FileSystemNode>>& fileList)
{
    if (!node || isCancelled()) return;
    if (!node->isDirectory()) {
        fileList.push_back(node);
    } else {
        for (const auto& child : node->getChildren())
            collectAllFiles(child, fileList);
    }
}

QString DuplicateWorker::computePartialHash(const QString& filePath, qint64 size) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) return {};

    QCryptographicHash hash(QCryptographicHash::Sha256);

    if (size > PARTIAL_THRESH) {
        // Sample: first chunk, middle chunk, last chunk
        auto readChunk = [&](qint64 offset) {
            if (file.seek(offset)) {
                const QByteArray buf = file.read(CHUNK_SIZE);
                if (!buf.isEmpty()) hash.addData(buf);
            }
        };
        readChunk(0);
        readChunk(size / 2);
        readChunk(qMax(0LL, size - CHUNK_SIZE));
        // Salt with file size so two files of different sizes never share a hash
        hash.addData(QByteArray::number(size));
    } else {
        // Small files: hash fully (cheap enough)
        char buf[16384];
        while (!file.atEnd() && !isCancelled()) {
            const qint64 n = file.read(buf, sizeof(buf));
            if (n > 0) hash.addData(QByteArrayView(buf, n));
        }
    }
    return hash.result().toHex();
}

QString DuplicateWorker::computeFullHash(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) return {};

    QCryptographicHash hash(QCryptographicHash::Sha256);
    char buf[65536];
    while (!file.atEnd() && !isCancelled()) {
        const qint64 n = file.read(buf, sizeof(buf));
        if (n > 0) hash.addData(QByteArrayView(buf, n));
    }
    return isCancelled() ? QString{} : hash.result().toHex();
}

// ─── DuplicateService ────────────────────────────────────────────

DuplicateService::DuplicateService(QObject* parent)
    : QObject(parent)
{
}

DuplicateService::~DuplicateService() {
    cancelSearch();
}

void DuplicateService::startSearch(std::shared_ptr<FileSystemNode> root) {
    cancelSearch();
    m_cachedDuplicates.clear();
    m_cachedWastedSpace = 0;

    auto* worker = new DuplicateWorker(root);
    m_worker = worker;
    connect(worker, &DuplicateWorker::progressUpdated, this, &DuplicateService::progressReported);
    connect(worker, &DuplicateWorker::searchFinished,  this, &DuplicateService::onWorkerFinished);
    connect(worker, &DuplicateWorker::finished, worker, &QObject::deleteLater);
    worker->start();
}

void DuplicateService::cancelSearch() {
    if (!m_worker.isNull() && m_worker->isRunning()) {
        m_worker->cancel();
        m_worker->wait(8000);
    }
}

bool DuplicateService::isSearching() const {
    return !m_worker.isNull() && m_worker->isRunning();
}

void DuplicateService::onWorkerFinished(bool success) {
    if (!m_worker.isNull()) {
        m_cachedDuplicates  = m_worker->getResult();
        m_cachedWastedSpace = m_worker->getWastedSpace();
    }
    emit searchCompleted(success);
}
