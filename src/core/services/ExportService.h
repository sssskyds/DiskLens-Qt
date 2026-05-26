#ifndef EXPORTSERVICE_H
#define EXPORTSERVICE_H

#include "core/models/FileSystemNode.h"
#include <QObject>
#include <QThread>
#include <QMutex>
#include <QStringList>
#include <memory>

/**
 * @brief Export configuration options.
 */
struct ExportOptions {
    enum Format { TreeText, Markdown, CSV };
    Format format = CSV;
    int maxDepth = 6;
    bool includeHidden = false;
    bool includeSize = true;
    bool includeCreatedDate = true;
    bool includeModifiedDate = true;
    bool includeOwner = false;
    bool includeCategory = true;
    bool includeExtension = true;
    bool selectedPathsOnly = false;
    QStringList selectedPaths;
};

/**
 * @brief Worker thread for heavy export jobs (streaming writes).
 */
class ExportWorker : public QThread {
    Q_OBJECT
public:
    ExportWorker(std::shared_ptr<FileSystemNode> root,
                 const QString& outputPath,
                 const ExportOptions& opts,
                 QObject* parent = nullptr);
    void cancel();

signals:
    void progressUpdated(int itemsWritten, const QString& currentItem);
    void exportFinished(bool success, const QString& outputPath);

protected:
    void run() override;

private:
    void exportTreeText(QTextStream& out, std::shared_ptr<FileSystemNode> node, int depth, const QString& prefix);
    void exportMarkdown(QTextStream& out, std::shared_ptr<FileSystemNode> node, int depth);
    void exportCsv(QTextStream& out, std::shared_ptr<FileSystemNode> node, int depth);
    void writeMarkdownHeader(QTextStream& out);
    void writeMarkdownSummary(QTextStream& out);
    QString formatBytes(qint64 bytes);

    std::shared_ptr<FileSystemNode> m_root;
    QString m_outputPath;
    ExportOptions m_opts;
    QMutex m_mutex;
    bool m_cancelRequested = false;
    int m_itemsWritten = 0;
};

/**
 * @brief Service facade for export operations.
 */
class ExportService : public QObject {
    Q_OBJECT
public:
    explicit ExportService(QObject* parent = nullptr);
    ~ExportService();

    void startExport(std::shared_ptr<FileSystemNode> root,
                     const QString& outputPath,
                     const ExportOptions& opts);
    void cancelExport();
    bool isExporting() const;

signals:
    void progressUpdated(int itemsWritten, const QString& currentItem);
    void exportFinished(bool success, const QString& outputPath);

private:
    ExportWorker* m_worker = nullptr;
};

#endif // EXPORTSERVICE_H
