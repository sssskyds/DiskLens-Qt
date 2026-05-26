#include "LargeFileService.h"
#include <algorithm>
#include <functional>  // std::greater

// ─── LargeFileWorker ─────────────────────────────────────────────

LargeFileWorker::LargeFileWorker(std::shared_ptr<FileSystemNode> root,
                                 int limit, QObject* parent)
    : QThread(parent), m_root(root), m_limit(limit)
{
}

void LargeFileWorker::cancel() {
    QMutexLocker lk(&m_mutex);
    m_cancelRequested = true;
}

bool LargeFileWorker::isCancelled() {
    QMutexLocker lk(&m_mutex);
    return m_cancelRequested;
}

void LargeFileWorker::run() {
    m_results.clear();
    m_results.reserve(m_limit + 1);
    m_checked         = 0;
    m_cancelRequested = false;

    if (!m_root) { emit searchFinished(false); return; }

    traverse(m_root);

    // Final sort: largest first for display
    std::sort(m_results.begin(), m_results.end(),
              [](const FileInfoSummary& a, const FileInfoSummary& b) {
                  return a.size > b.size;
              });

    emit searchFinished(!isCancelled());
}

void LargeFileWorker::traverse(std::shared_ptr<FileSystemNode> node) {
    if (!node || isCancelled()) return;

    if (!node->isDirectory()) {
        m_checked++;
        if (m_checked % PROGRESS_INTERVAL == 0)
            emit progressUpdated(m_checked);

        // Maintain a min-heap: only keep the top `m_limit` files.
        // The vector is kept as a min-heap (smallest at front) so we can
        // cheaply check whether the new file beats the current minimum.
        FileInfoSummary entry;
        entry.name         = node->getName();
        entry.path         = node->getPath();
        entry.size         = node->getSize();
        entry.lastModified = node->getLastModified();
        entry.category     = node->getCategory();
        entry.extension    = node->getExtension();

        if (static_cast<int>(m_results.size()) < m_limit) {
            m_results.push_back(entry);
            // push_heap: O(log n)
            std::push_heap(m_results.begin(), m_results.end(),
                           [](const FileInfoSummary& a, const FileInfoSummary& b) {
                               return a.size > b.size; // min-heap: smallest on top
                           });
        } else if (!m_results.isEmpty() && entry.size > m_results.front().size) {
            // Replace the current minimum with the larger file
            std::pop_heap(m_results.begin(), m_results.end(),
                          [](const FileInfoSummary& a, const FileInfoSummary& b) {
                              return a.size > b.size;
                          });
            m_results.back() = entry;
            std::push_heap(m_results.begin(), m_results.end(),
                           [](const FileInfoSummary& a, const FileInfoSummary& b) {
                               return a.size > b.size;
                           });
        }
    } else {
        for (const auto& child : node->getChildren())
            traverse(child);
    }
}

// ─── LargeFileService ────────────────────────────────────────────

LargeFileService::LargeFileService(QObject* parent)
    : QObject(parent)
{
}

LargeFileService::~LargeFileService() {
    cancelSearch();
}

void LargeFileService::startSearch(std::shared_ptr<FileSystemNode> root, int limit) {
    cancelSearch();
    m_cachedResults.clear();

    auto* worker = new LargeFileWorker(root, limit);
    m_worker = worker;
    connect(worker, &LargeFileWorker::progressUpdated, this, &LargeFileService::progressUpdated);
    connect(worker, &LargeFileWorker::searchFinished,  this, &LargeFileService::onWorkerFinished);
    connect(worker, &LargeFileWorker::finished, worker, &QObject::deleteLater);
    worker->start();
}

void LargeFileService::cancelSearch() {
    if (!m_worker.isNull() && m_worker->isRunning()) {
        m_worker->cancel();
        m_worker->wait(5000);
    }
}

bool LargeFileService::isSearching() const {
    return !m_worker.isNull() && m_worker->isRunning();
}

void LargeFileService::onWorkerFinished(bool success) {
    if (!m_worker.isNull())
        m_cachedResults = m_worker->getResult();
    emit searchCompleted(success);
}
