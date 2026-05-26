#include "FileTypeService.h"
#include "core/utils/CategoryMapper.h"
#include <algorithm>

FileTypeService::FileTypeService(QObject* parent)
    : QObject(parent)
{
}

QVector<CategoryStat> FileTypeService::analyzeFileTypes(
    std::shared_ptr<FileSystemNode> root)
{
    QVector<CategoryStat> stats;
    if (!root) return stats;

    QHash<QString, qint64> catSizes;
    QHash<QString, qint64> catCounts;
    QHash<QString, QHash<QString, ExtensionStat>> extDetails;

    aggregateNode(root, catSizes, catCounts, extDetails);

    const qint64 grandTotal = root->getSize(); // correct after computeSizes pass

    static const QHash<QString, QString> catColors = {
        {"Images",      "#00d4ff"},
        {"Videos",      "#8b5cf6"},
        {"Audio",       "#ec4899"},
        {"Documents",   "#eab308"},
        {"Code",        "#10b981"},
        {"Archives",    "#f97316"},
        {"Executables", "#ef4444"},
        {"Fonts",       "#14b8a6"},
        {"Data",        "#3b82f6"},
        {"Other",       "#64748b"}
    };

    static const QStringList order = {
        "Images","Videos","Audio","Documents","Code",
        "Archives","Executables","Fonts","Data","Other"
    };

    for (const QString& cat : order) {
        const qint64 count = catCounts.value(cat, 0);
        if (count == 0) continue;

        CategoryStat cs;
        cs.categoryName = cat;
        cs.totalSize    = catSizes.value(cat, 0);
        cs.fileCount    = count;
        cs.color        = catColors.value(cat, "#64748b");
        cs.percentage   = grandTotal > 0
                              ? (static_cast<double>(cs.totalSize) / grandTotal) * 100.0
                              : 0.0;

        const auto& exts = extDetails.value(cat);
        cs.extensionDetails.reserve(exts.size());
        for (const auto& v : exts) cs.extensionDetails.push_back(v);
        std::sort(cs.extensionDetails.begin(), cs.extensionDetails.end(),
                  [](const ExtensionStat& a, const ExtensionStat& b) {
                      return a.size > b.size;
                  });
        stats.push_back(cs);
    }

    std::sort(stats.begin(), stats.end(), [](const CategoryStat& a, const CategoryStat& b) {
        return a.totalSize > b.totalSize;
    });
    return stats;
}

void FileTypeService::aggregateNode(
    std::shared_ptr<FileSystemNode> node,
    QHash<QString, qint64>& catSizes,
    QHash<QString, qint64>& catCounts,
    QHash<QString, QHash<QString, ExtensionStat>>& extDetails)
{
    if (!node) return;

    if (!node->isDirectory()) {
        // Use CategoryMapper so the classification is always consistent with
        // ScanWorker and SearchWorker — no divergence.
        const QString cat = CategoryMapper::classify(node->getExtension());
        const qint64  sz  = node->getSize();

        QString ext = node->getExtension();
        if (ext.startsWith('.')) ext = ext.mid(1); // normalise: remove leading dot
        if (ext.isEmpty()) ext = QStringLiteral("(no extension)");

        catSizes[cat]  += sz;
        catCounts[cat] += 1;

        auto& catExts = extDetails[cat];
        if (!catExts.contains(ext)) {
            catExts[ext] = ExtensionStat{ext, 0, 0};
        }
        catExts[ext].size  += sz;
        catExts[ext].count += 1;
    } else {
        for (const auto& child : node->getChildren())
            aggregateNode(child, catSizes, catCounts, extDetails);
    }
}
