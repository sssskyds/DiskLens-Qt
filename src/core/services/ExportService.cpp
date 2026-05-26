#include "core/services/ExportService.h"
#include <QFile>
#include <QSaveFile>
#include <QTextStream>
#include <QDateTime>
#include <QDir>

// ─── ExportWorker ────────────────────────────────────────────────

ExportWorker::ExportWorker(std::shared_ptr<FileSystemNode> root,
                           const QString& outputPath,
                           const ExportOptions& opts,
                           QObject* parent)
    : QThread(parent), m_root(root), m_outputPath(outputPath), m_opts(opts)
{
}

void ExportWorker::cancel() {
    QMutexLocker lock(&m_mutex);
    m_cancelRequested = true;
}

QString ExportWorker::formatBytes(qint64 bytes) {
    if (bytes >= (qint64)1099511627776LL) return QString::number(bytes / 1099511627776.0, 'f', 2) + " TB";
    if (bytes >= 1073741824LL) return QString::number(bytes / 1073741824.0, 'f', 2) + " GB";
    if (bytes >= 1048576LL)    return QString::number(bytes / 1048576.0, 'f', 2) + " MB";
    if (bytes >= 1024LL)       return QString::number(bytes / 1024.0, 'f', 1) + " KB";
    return QString::number(bytes) + " B";
}

void ExportWorker::run() {
    m_cancelRequested = false;
    m_itemsWritten = 0;

    // Use QSaveFile for atomic writes
    QSaveFile file(m_outputPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        emit exportFinished(false, m_outputPath);
        return;
    }

    QTextStream out(&file);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    out.setEncoding(QStringConverter::Utf8);
#else
    out.setCodec("UTF-8");
#endif

    switch (m_opts.format) {
        case ExportOptions::TreeText:
            exportTreeText(out, m_root, 0, "");
            break;
        case ExportOptions::Markdown:
            writeMarkdownHeader(out);
            exportMarkdown(out, m_root, 0);
            writeMarkdownSummary(out);
            break;
        case ExportOptions::CSV:
            // CSV header
            out << "Name,Path,Size (bytes),Size,Extension,Category";
            if (m_opts.includeModifiedDate) out << ",Modified";
            if (m_opts.includeCreatedDate)  out << ",Created";
            if (m_opts.includeOwner)        out << ",Owner";
            out << "\n";
            exportCsv(out, m_root, 0);
            break;
    }

    if (file.commit()) {
        emit exportFinished(true, m_outputPath);
    } else {
        emit exportFinished(false, m_outputPath);
    }
}

void ExportWorker::exportTreeText(QTextStream& out, std::shared_ptr<FileSystemNode> node, int depth, const QString& prefix) {
    {
        QMutexLocker lock(&m_mutex);
        if (m_cancelRequested) return;
    }
    if (depth > m_opts.maxDepth) return;

    QString line = prefix + node->getName();
    if (node->isDirectory()) line += "/";
    if (m_opts.includeSize && !node->isDirectory()) {
        line += "  [" + formatBytes(node->getSize()) + "]";
    }
    out << line << "\n";
    m_itemsWritten++;

    if (m_itemsWritten % 500 == 0) {
        emit progressUpdated(m_itemsWritten, node->getName());
    }

    if (node->isDirectory()) {
        const auto& children = node->getChildren();
        for (int i = 0; i < children.size(); i++) {
            bool isLast = (i == children.size() - 1);
            QString childPrefix = prefix + (isLast ? "    " : "│   ");
            QString connector = isLast ? "└── " : "├── ";
            out << prefix << connector;

            // Write child content inline (without extra prefix for the name line)
            auto& child = children[i];
            QString childLine = child->getName();
            if (child->isDirectory()) childLine += "/";
            if (m_opts.includeSize && !child->isDirectory()) {
                childLine += "  [" + formatBytes(child->getSize()) + "]";
            }
            out << childLine << "\n";
            m_itemsWritten++;

            if (child->isDirectory()) {
                const auto& grandChildren = child->getChildren();
                for (int j = 0; j < grandChildren.size(); j++) {
                    exportTreeText(out, grandChildren[j], depth + 2, childPrefix);
                }
            }
        }
    }
}

void ExportWorker::writeMarkdownHeader(QTextStream& out) {
    out << "# DiskLens Export Report\n\n";
    out << "**Generated**: " << QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss") << "\n\n";
    out << "**Root Path**: `" << m_root->getPath() << "`\n\n";
    out << "**Total Size**: " << formatBytes(m_root->getSize()) << "\n\n";
    out << "---\n\n";
    out << "## Directory Structure\n\n";
    out << "```\n";
}

void ExportWorker::writeMarkdownSummary(QTextStream& out) {
    out << "```\n\n";
    out << "---\n\n";
    out << "*Exported by DiskLens*\n";
}

void ExportWorker::exportMarkdown(QTextStream& out, std::shared_ptr<FileSystemNode> node, int depth) {
    {
        QMutexLocker lock(&m_mutex);
        if (m_cancelRequested) return;
    }
    if (depth > m_opts.maxDepth) return;

    QString indent;
    for (int i = 0; i < depth; i++) indent += "  ";

    QString entry = indent;
    if (node->isDirectory()) {
        entry += "📁 " + node->getName() + "/";
    } else {
        entry += "📄 " + node->getName();
        if (m_opts.includeSize) entry += " (" + formatBytes(node->getSize()) + ")";
    }
    out << entry << "\n";
    m_itemsWritten++;

    if (m_itemsWritten % 500 == 0) {
        emit progressUpdated(m_itemsWritten, node->getName());
    }

    if (node->isDirectory()) {
        for (auto& child : node->getChildren()) {
            exportMarkdown(out, child, depth + 1);
        }
    }
}

void ExportWorker::exportCsv(QTextStream& out, std::shared_ptr<FileSystemNode> node, int depth) {
    {
        QMutexLocker lock(&m_mutex);
        if (m_cancelRequested) return;
    }
    if (depth > m_opts.maxDepth) return;

    if (!node->isDirectory()) {
        // Escape CSV fields
        auto csvField = [](const QString& s) -> QString {
            if (s.contains(',') || s.contains('"') || s.contains('\n')) {
                return "\"" + QString(s).replace("\"", "\"\"") + "\"";
            }
            return s;
        };

        out << csvField(node->getName()) << ","
            << csvField(node->getPath()) << ","
            << node->getSize() << ","
            << csvField(formatBytes(node->getSize())) << ","
            << csvField(node->getExtension()) << ","
            << csvField(node->getCategory());

        if (m_opts.includeModifiedDate) {
            out << "," << csvField(node->getLastModified().toString("yyyy-MM-dd HH:mm:ss"));
        }
        if (m_opts.includeCreatedDate) {
            // Qt port doesn't track creation date natively in this model, fallback to modified or empty
            out << "," << csvField(node->getLastModified().toString("yyyy-MM-dd HH:mm:ss"));
        }
        if (m_opts.includeOwner) {
            out << "," << csvField(""); // Owner tracking not in Qt port
        }
        out << "\n";
        m_itemsWritten++;

        if (m_itemsWritten % 1000 == 0) {
            emit progressUpdated(m_itemsWritten, node->getName());
        }
    }

    if (node->isDirectory()) {
        for (auto& child : node->getChildren()) {
            exportCsv(out, child, depth + 1);
        }
    }
}

// ─── ExportService ───────────────────────────────────────────────

ExportService::ExportService(QObject* parent) : QObject(parent) {}

ExportService::~ExportService() {
    cancelExport();
}

void ExportService::startExport(std::shared_ptr<FileSystemNode> root,
                                const QString& outputPath,
                                const ExportOptions& opts) {
    cancelExport();

    m_worker = new ExportWorker(root, outputPath, opts);
    connect(m_worker, &ExportWorker::progressUpdated, this, &ExportService::progressUpdated);
    connect(m_worker, &ExportWorker::exportFinished, this, &ExportService::exportFinished);
    connect(m_worker, &ExportWorker::finished, m_worker, &QObject::deleteLater);
    connect(m_worker, &ExportWorker::finished, this, [this]() { m_worker = nullptr; });
    m_worker->start();
}

void ExportService::cancelExport() {
    if (m_worker && m_worker->isRunning()) {
        m_worker->cancel();
        m_worker->wait(5000);
    }
}

bool ExportService::isExporting() const {
    return m_worker && m_worker->isRunning();
}
