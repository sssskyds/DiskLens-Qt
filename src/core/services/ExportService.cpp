#include "core/services/ExportService.h"
#include <QFile>
#include <QSaveFile>
#include <QTextStream>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>

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

bool ExportWorker::isCancelled() {
    QMutexLocker lock(&m_mutex);
    return m_cancelRequested;
}

QString ExportWorker::formatBytes(qint64 bytes) {
    if (bytes >= (qint64)1099511627776LL)
        return QString::number(bytes / 1099511627776.0, 'f', 2) + " TB";
    if (bytes >= 1073741824LL)
        return QString::number(bytes / 1073741824.0, 'f', 2) + " GB";
    if (bytes >= 1048576LL)
        return QString::number(bytes / 1048576.0, 'f', 2) + " MB";
    if (bytes >= 1024LL)
        return QString::number(bytes / 1024.0, 'f', 1) + " KB";
    return QString::number(bytes) + " B";
}

void ExportWorker::run() {
    m_cancelRequested = false;
    m_itemsWritten    = 0;

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
            // Header row
            out << "Name,Path,Size (bytes),Size,Extension,Category";
            if (m_opts.includeModifiedDate) out << ",Modified";
            if (m_opts.includeCreatedDate)  out << ",Created";
            if (m_opts.includeOwner)        out << ",Owner";
            out << "\n";
            exportCsv(out, m_root, 0);
            break;
    }

    if (isCancelled()) {
        // Discard the partial file — QSaveFile does not commit unless we call commit()
        emit exportFinished(false, m_outputPath);
        return;
    }

    if (file.commit()) {
        emit exportFinished(true, m_outputPath);
    } else {
        emit exportFinished(false, m_outputPath);
    }
}

// ── Tree text export ──────────────────────────────────────────────
// Fixed: previously wrote each node's name twice (once as the label line and
// once again inside the child loop).  Now the root prints its own label and
// recursively lets children do the same via exportTreeText.

void ExportWorker::exportTreeText(QTextStream& out,
                                  std::shared_ptr<FileSystemNode> node,
                                  int depth,
                                  const QString& prefix)
{
    if (isCancelled()) return;
    if (depth > m_opts.maxDepth) return;

    // Emit progress every 500 items
    m_itemsWritten++;
    if (m_itemsWritten % 500 == 0)
        emit progressUpdated(m_itemsWritten, node->getName());

    if (node->isDirectory()) {
        const auto& children = node->getChildren();
        for (int i = 0; i < children.size(); ++i) {
            if (isCancelled()) return;
            const bool   isLast      = (i == children.size() - 1);
            const QString connector  = isLast ? "└── " : "├── ";
            const QString childPfx   = prefix + (isLast ? "    " : "│   ");
            const auto&   child      = children[i];

            QString label = child->getName();
            if (child->isDirectory()) label += "/";
            if (m_opts.includeSize && !child->isDirectory())
                label += "  [" + formatBytes(child->getSize()) + "]";

            out << prefix << connector << label << "\n";
            exportTreeText(out, child, depth + 1, childPfx);
        }
    }
}

// ── Markdown export ───────────────────────────────────────────────

void ExportWorker::writeMarkdownHeader(QTextStream& out) {
    out << "# DiskLens Export Report\n\n";
    out << "**Generated**: "
        << QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss")
        << "\n\n";
    out << "**Root Path**: `" << m_root->getPath() << "`\n\n";
    out << "**Total Size**: " << formatBytes(m_root->getSize()) << "\n\n";
    out << "---\n\n";
    out << "## Directory Structure\n\n";
    out << "```\n";
    // Print root label
    out << m_root->getName() << "/\n";
}

void ExportWorker::writeMarkdownSummary(QTextStream& out) {
    out << "```\n\n";
    out << "---\n\n";
    out << "*Exported by DiskLens*\n";
}

void ExportWorker::exportMarkdown(QTextStream& out,
                                  std::shared_ptr<FileSystemNode> node,
                                  int depth)
{
    if (isCancelled()) return;
    if (depth > m_opts.maxDepth) return;

    QString indent(depth * 2, ' ');

    if (node->isDirectory()) {
        for (auto& child : node->getChildren()) {
            if (isCancelled()) return;
            QString entry = indent;
            if (child->isDirectory()) {
                entry += "📁 " + child->getName() + "/";
            } else {
                entry += "📄 " + child->getName();
                if (m_opts.includeSize)
                    entry += " (" + formatBytes(child->getSize()) + ")";
            }
            out << entry << "\n";
            m_itemsWritten++;
            if (m_itemsWritten % 500 == 0)
                emit progressUpdated(m_itemsWritten, child->getName());
            exportMarkdown(out, child, depth + 1);
        }
    }
}

// ── CSV export ────────────────────────────────────────────────────

void ExportWorker::exportCsv(QTextStream& out,
                             std::shared_ptr<FileSystemNode> node,
                             int depth)
{
    if (isCancelled()) return;
    if (depth > m_opts.maxDepth) return;

    auto csvEsc = [](const QString& s) -> QString {
        if (s.contains(',') || s.contains('"') || s.contains('\n'))
            return '"' + QString(s).replace('"', QStringLiteral("\"\"")) + '"';
        return s;
    };

    if (!node->isDirectory()) {
        out << csvEsc(node->getName())     << ","
            << csvEsc(node->getPath())     << ","
            << node->getSize()             << ","
            << csvEsc(formatBytes(node->getSize())) << ","
            << csvEsc(node->getExtension()) << ","
            << csvEsc(node->getCategory());

        if (m_opts.includeModifiedDate)
            out << "," << csvEsc(node->getLastModified