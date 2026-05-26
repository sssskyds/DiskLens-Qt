#include "FileTypeService.h"
#include <algorithm>

FileTypeService::FileTypeService(QObject* parent)
    : QObject(parent)
{
}

QVector<CategoryStat> FileTypeService::analyzeFileTypes(std::shared_ptr<FileSystemNode> root) {
    QVector<CategoryStat> stats;
    if (!root) return stats;

    QHash<QString, qint64> catSizes;
    QHash<QString, qint64> catCounts;
    QHash<QString, QHash<QString, ExtensionStat>> extDetails;

    // Start aggregation
    aggregateNode(root, catSizes, catCounts, extDetails);

    qint64 grandTotalSize = root->getSize();

    // Standard Category Colors from Design Tokens
    QHash<QString, QString> catColors = {
        {"Images", "#00d4ff"},       // Cyan
        {"Videos", "#8b5cf6"},       // Purple
        {"Audio", "#ec4899"},        // Pink
        {"Documents", "#eab308"},    // Yellow
        {"Code", "#10b981"},         // Emerald
        {"Archives", "#f97316"},     // Orange
        {"Executables", "#ef4444"},  // Red
        {"Fonts", "#14b8a6"},        // Teal
        {"Data", "#3b82f6"},         // Blue
        {"Other", "#64748b"}         // Slate
    };

    QStringList allCategories = {
        "Images", "Videos", "Audio", "Documents", "Code", 
        "Archives", "Executables", "Fonts", "Data", "Other"
    };

    for (const QString& cat : allCategories) {
        qint64 size = catSizes.value(cat, 0);
        qint64 count = catCounts.value(cat, 0);

        if (count == 0) continue; // Skip empty categories

        CategoryStat catStat;
        catStat.categoryName = cat;
        catStat.totalSize = size;
        catStat.fileCount = count;
        catStat.color = catColors.value(cat, "#64748b");
        
        if (grandTotalSize > 0) {
            catStat.percentage = (static_cast<double>(size) / grandTotalSize) * 100.0;
        } else {
            catStat.percentage = 0.0;
        }

        // Add extensions sorted by size descending
        auto extHash = extDetails.value(cat);
        for (const auto& val : extHash) {
            catStat.extensionDetails.push_back(val);
        }

        std::sort(catStat.extensionDetails.begin(), catStat.extensionDetails.end(), 
                  [](const ExtensionStat& a, const ExtensionStat& b) {
                      return a.size > b.size;
                  });

        stats.push_back(catStat);
    }

    // Sort categories by size descending
    std::sort(stats.begin(), stats.end(), [](const CategoryStat& a, const CategoryStat& b) {
        return a.totalSize > b.totalSize;
    });

    return stats;
}

void FileTypeService::aggregateNode(std::shared_ptr<FileSystemNode> node, 
                                   QHash<QString, qint64>& catSizes, 
                                   QHash<QString, qint64>& catCounts,
                                   QHash<QString, QHash<QString, ExtensionStat>>& extDetails) 
{
    if (!node) return;

    if (!node->isDirectory()) {
        QString cat = node->getCategory();
        if (cat.isEmpty()) {
            cat = "Other";
        }
        qint64 size = node->getSize();
        QString ext = node->getExtension();
        if (ext.isEmpty()) {
            ext = "no extension";
        }

        catSizes[cat] += size;
        catCounts[cat]++;

        auto& catExts = extDetails[cat];
        if (!catExts.contains(ext)) {
            ExtensionStat est;
            est.ext = ext;
            est.size = 0;
            est.count = 0;
            catExts[ext] = est;
        }
        catExts[ext].size += size;
        catExts[ext].count++;
    } else {
        for (const auto& child : node->getChildren()) {
            aggregateNode(child, catSizes, catCounts, extDetails);
        }
    }
}
