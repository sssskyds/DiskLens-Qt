#include "DuplicateService.h"
#include <QFile>
#include <QCryptographicHash>
#include <QHash>
#include <QDebug>
#include <algorithm>

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
    m_wastedSpace = 0;
    m_cancelRequested = false;

    if (!m_root) {
        emit searchFinished(false);
        return;
    }

    // Step 1: Collect all files
    QVector<std::shared_ptr<FileSystemNode>> allFiles;
    collectAllFiles(m_root, allFiles);

    if (isCancelled()) {
        emit searchFinished(false);
        return;
    }

    // Step 2: Group by size
    QHash<qint64, QVector<QString>> sizeGroups;
    for (const auto& node : allFiles) {
        sizeGroups[node->getSize()].push_back(node->getPath());
    }

    // Filter sizes that have more than 1 file
    QVector<qint64> candidateSizes;
    int totalCandidates = 0;
    for (auto it = sizeGroups.begin(); it != sizeGroups.end(); ++it) {
        if (it.value().size() > 1 && it.key() > 0) {
            candidateSizes.push_back(it.key());
            totalCandidates += it.value().size();
        }
    }

    if (isCancelled() || candidateSizes.isEmpty()) {
        emit searchFinished(true);
        return;
    }

    // Step 3: Hash files with identical sizes
    QHash<QString, QVector<QString>> checksumGroups;
    int processedCount = 0;

    for (qint64 size : candidateSizes) {
        if (isCancelled()) break;

        const auto& paths = sizeGroups[size];
        QHash<QString, QVector<QString>> localHashGroups;

        for (const QString& path : paths) {
            if (isCancelled()) break;

            processedCount++;
            if (processedCount % 5 == 0) {
                emit progressUpdated(path, processedCount, totalCandidates);
            }

            QString hash = calculateChecksum(path, size);
            if (!hash.isEmpty()) {
                localHashGroups[hash].push_back(path);
            }
        }

        // Add matching groups to global duplicates
        for (auto it = localHashGroups.begin(); it != localHashGroups.end(); ++it) {
            if (it.value().size() > 1) {
                DuplicateGroup group;
                group.fileSize = size;
                group.checksum = it.key();
                group.filePaths = it.value();
                m_duplicates.push_back(group);

                // Wasted space is size * (count - 1)
                m_wastedSpace += size * (it.value().size() - 1);
            }
        }
    }

    // Sort duplicates by size descending
    std::sort(m_duplicates.begin(), m_duplicates.end(), [](const DuplicateGroup& a, const DuplicateGroup& b) {
        return a.fileSize > b.fileSize;
    });

    if (isCancelled()) {
        emit searchFinished(false);
    } else {
        emit searchFinished(true);
    }
}

void DuplicateWorker::collectAllFiles(std::shared_ptr<FileSystemNode> node, QVector<std::shared_ptr<FileSystemNode>>& fileList) {
    if (!node || isCancelled()) return;

    if (!node->isDirectory()) {
        fileList.push_back(node);
    } else {
        for (const auto& child : node->getChildren()) {
            collectAllFiles(child, fileList);
        }
    }
}

QString DuplicateWorker::calculateChecksum(const QString& filePath, qint64 size) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return "";
    }

    QCryptographicHash hash(QCryptographicHash::Md5);
    
    // Partial hashing optimization for large files (> 100MB)
    const qint64 HUNDRED_MB = 100LL * 1024 * 1024;
    const qint64 CHUNK_SIZE = 8 * 1024; // 8KB

    if (size > HUNDRED_MB) {
        // Read first chunk
        QByteArray buffer = file.read(CHUNK_SIZE);
        hash.addData(buffer);

        // Read middle chunk
        if (file.seek(size / 2)) {
            buffer = file.read(CHUNK_SIZE);
            hash.addData(buffer);
        }

        // Read last chunk
        if (file.seek(size - CHUNK_SIZE)) {
            buffer = file.read(CHUNK_SIZE);
            hash.addData(buffer);
        }
        
        // Append size to salt the partial hash
        hash.addData(QByteArray::number(size));
    } else {
        // Full file hashing for smaller files
        char buffer[16384];
        while (!file.atEnd() && !isCancelled()) {
            qint64 bytesRead = file.read(buffer, sizeof(buffer));
            if (bytesRead > 0) {
                hash.addData(QByteArrayView(buffer, bytesRead));
            }
        }
    }

    file.close();
    return hash.result().toHex();
}

// --- DuplicateService Implementation ---

DuplicateService::DuplicateService(QObject* parent)
    : QObject(parent)
{
}

DuplicateService::~DuplicateService() {
    cancelSearch();
}

void DuplicateService::startSearch(std::shared_ptr<FileSystemNode> root) {
    cancelSearch();

    m_worker = new DuplicateWorker(root, this);
    connect(m_worker, &DuplicateWorker::progressUpdated, this, &DuplicateService::progressReported);
    connect(m_worker, &DuplicateWorker::searchFinished, this, &DuplicateService::searchCompleted);
    connect(m_worker, &DuplicateWorker::finished, this, &DuplicateService::onWorkerFinished);

    m_worker->start();
}

void DuplicateService::cancelSearch() {
    if (m_worker) {
        m_worker->cancel();
        m_worker->wait();
        delete m_worker;
        m_worker = nullptr;
    }
}

QVector<DuplicateGroup> DuplicateService::getDuplicates() const {
    return m_worker ? m_worker->getResult() : QVector<DuplicateGroup>();
}

qint64 DuplicateService::getWastedSpace() const {
    return m_worker ? m_worker->getWastedSpace() : 0;
}

void DuplicateService::onWorkerFinished() {
    // Worker completed
}
