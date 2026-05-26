#include "LargeFileService.h"
#include <algorithm>

LargeFileService::LargeFileService(QObject* parent)
    : QObject(parent)
{
}

QVector<FileInfoSummary> LargeFileService::findLargeFiles(std::shared_ptr<FileSystemNode> root, int limit) {
    QVector<FileInfoSummary> allFiles;
    if (!root) return allFiles;

    collectFiles(root, allFiles);

    // Sort descending by size
    std::sort(allFiles.begin(), allFiles.end(), [](const FileInfoSummary& a, const FileInfoSummary& b) {
        return a.size > b.size;
    });

    if (allFiles.size() > limit) {
        allFiles.resize(limit);
    }

    return allFiles;
}

void LargeFileService::collectFiles(std::shared_ptr<FileSystemNode> node, QVector<FileInfoSummary>& files) {
    if (!node) return;

    if (!node->isDirectory()) {
        FileInfoSummary summary;
        summary.name = node->getName();
        summary.path = node->getPath();
        summary.size = node->getSize();
        summary.lastModified = node->getLastModified();
        summary.category = node->getCategory();
        files.push_back(summary);
    } else {
        for (const auto& child : node->getChildren()) {
            collectFiles(child, files);
        }
    }
}
