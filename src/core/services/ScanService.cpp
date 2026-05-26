#include "ScanService.h"
#include "core/utils/CategoryMapper.h"
#include <QDir>
#include <QFileInfo>
#include <QDebug>

// ─── ScanWorker ──────────────────────────────────────────────────

ScanWorker::ScanWorker(const QString& rootPath, int maxDepth, QObject* parent)
    : QThread(parent), m_rootPath(rootPath), m_maxDepth(maxDepth)
{
}

void ScanWorker::cancel() {
    QMutexLocker locker(&m_mutex);
    m_cancelRequested = true;
}

bool ScanWorker::isCancelled() {
    QMutexLocker locker(&m_mutex);
    return m_cancelRequested;
}

void ScanWorker::run() {
    m_fileCount      = 0;
    m_folderCount    = 0;
    m_bytesScanned   = 0;
    m_cancelRequested = false;
    m_progressTimer.start();

    QFileInfo rootInfo(m_rootPath);
    if (!rootInfo.exists()) {
        emit scanFinished(false);
        return;
    }

    const QString rootName = rootInfo.fileName().isEmpty()
                                 ? m_rootPath
                                 : rootInfo.fileName();
    m_rootNode = std::make_shared<FileSystemNode>(rootName, m_rootPath, true);
    m_rootNode->setLastModified(rootInfo.lastModified());

    scanDirectory(m_rootPath, m_rootNode, 0);

    if (!isCancelled()) {
        // Post-order pass: compute accurate cumulative sizes for every folder.
        // This replaces the old incremental addSize() approach which only
        // updated the immediate parent and produced wrong subtree totals.
        computeSizes(m_rootNode);
    }

    emit scanFinished(!isCancelled());
}

void ScanWorker::scanDirectory(const QString& path,
                               std::shared_ptr<FileSystemNode>& parentNode,
                               int currentDepth)
{
    if (isCancelled()) return;

    QDir dir(path);
    if (!dir.exists()) return;

    const QFileInfoList entries = dir.entryInfoList(
        QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System,
        QDir::NoSort);

    for (const QFileInfo& entry : entries) {
        if (isCancelled()) return;

        // Skip symlinks to avoid infinite loops / double-counting
        if (entry.isSymLink()) continue;

        const QString name     = entry.fileName();
        const QString fullPath = entry.absoluteFilePath();

        if (entry.isDir()) {
            m_folderCount++;
            auto childNode = std::make_shared<FileSystemNode>(name, fullPath, true);
            childNode->setLastModified(entry.lastModified());
            parentNode->addChild(childNode);

            if (m_maxDepth < 0 || currentDepth < m_maxDepth) {
                scanDirectory(fullPath, childNode, currentDepth + 1);
            }
        } else {
            m_fileCount++;
            const qint64 size = entry.size();
            m_bytesScanned += size;

            auto childNode = std::make_shared<FileSystemNode>(name, fullPath, false);
            childNode->setSize(size);
            childNode->setLastModified(entry.lastModified());

            const QString ext = childNode->getExtension();
            childNode->setCategory(CategoryMapper::classify(ext));

            parentNode->addChild(childNode);
        }

        // Time-based progress: emit every PROGRESS_INTERVAL_MS regardless of
        // count, so both sparse-deep and flat-shallow trees give responsive UI.
        if (m_progressTimer.elapsed() >= PROGRESS_INTERVAL_MS) {
            emit progressUpdated(fullPath, m_fileCount, m_bytesScanned);
            m_progressTimer.restart();
        }
    }
}

quint64 ScanWorker::computeSizes(std::shared_ptr<FileSystemNode>& node) {
    if (!node->isDirectory()) {
        return static_cast<quint64>(node->getSize());
    }
    quint64 total = 0;
    for (auto& child : node->getChildren()) {
        total += computeSizes(child);
    }
    node->setSize(static_cast<qint64>(total));
    return total;
}

// ─── ScanService ─────────────────────────────────────────────────

ScanService::ScanService(QObject* parent)
    : QObject(parent)
{
}

ScanService::~ScanService() {
    cancelScan();
}

void ScanService::startScan(const QString& path, int maxDepth) {
    cancelScan();
    m_cachedResult      = nullptr;
    m_cachedFileCount   = 0;
    m_cachedFolderCount = 0;

    m_worker = new ScanWorker(path, maxDepth, this);
    connect(m_worker, &ScanWorker::progressUpdated, this, &ScanService::progressReported);
    connect(m_worker, &ScanWorker::scanFinished,    this, &ScanService::onWorkerFinished);
    m_worker->start();
}

void ScanService::cancelScan() {
    if (m_worker) {
        m_worker->cancel();
        m_worker->wait();
        delete m_worker;
        m_worker = nullptr;
    }
}

void ScanService::onWorkerFinished(bool success) {
    if (m_worker) {
        // Cache results BEFORE deleting the worker so getScanResult() is
        // always safe to call after scanCompleted fires.
        m_cachedResult      = m_worker->getResult();
        m_cachedFileCount   = m_worker->getFileCount();
        m_cachedFolderCount = m_worker->getFolderCount();
    }
    emit scanCompleted(success);
}
