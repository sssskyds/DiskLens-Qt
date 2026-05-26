#include "ScanService.h"
#include <QDir>
#include <QFileInfo>
#include <QDebug>

// --- ScanWorker Implementation ---

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
    m_fileCount = 0;
    m_folderCount = 0;
    m_bytesScanned = 0;
    m_cancelRequested = false;

    QFileInfo rootInfo(m_rootPath);
    if (!rootInfo.exists()) {
        emit scanFinished(false);
        return;
    }

    m_rootNode = std::make_shared<FileSystemNode>(rootInfo.fileName().isEmpty() ? m_rootPath : rootInfo.fileName(), m_rootPath, true);
    m_rootNode->setLastModified(rootInfo.lastModified());

    scanDirectory(m_rootPath, m_rootNode, 0);

    if (isCancelled()) {
        emit scanFinished(false);
    } else {
        emit scanFinished(true);
    }
}

void ScanWorker::scanDirectory(const QString& path, std::shared_ptr<FileSystemNode>& parentNode, int currentDepth) {
    if (isCancelled()) return;

    QDir dir(path);
    if (!dir.exists()) return;

    // Retrieve both files and directories, skipping system files, hidden (optional, let's include if accessible), and "." ".."
    QFileInfoList entries = dir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System, QDir::Size | QDir::Name);

    for (const QFileInfo& entry : entries) {
        if (isCancelled()) return;

        QString name = entry.fileName();
        QString fullPath = entry.absoluteFilePath();

        // Prevent scanning symbolic links to avoid infinite recursion/loops
        if (entry.isSymLink()) {
            continue;
        }

        if (entry.isDir()) {
            m_folderCount++;
            auto childNode = std::make_shared<FileSystemNode>(name, fullPath, true);
            childNode->setLastModified(entry.lastModified());
            parentNode->addChild(childNode);

            // Throttle notifications slightly to avoid UI thread spamming
            if (m_folderCount % 50 == 0) {
                emit progressUpdated(fullPath, m_fileCount, m_bytesScanned);
            }

            // Recursive traversal under the depth limit
            if (m_maxDepth < 0 || currentDepth < m_maxDepth) {
                scanDirectory(fullPath, childNode, currentDepth + 1);
            }
        } else {
            m_fileCount++;
            qint64 size = entry.size();
            m_bytesScanned += size;

            auto childNode = std::make_shared<FileSystemNode>(name, fullPath, false);
            childNode->setSize(size);
            childNode->setLastModified(entry.lastModified());
            
            // Classification
            QString ext = childNode->getExtension();
            childNode->setCategory(classifyCategory(ext));

            parentNode->addChild(childNode);
            parentNode->addSize(size); // Propagate sizes upwards

            if (m_fileCount % 500 == 0) {
                emit progressUpdated(fullPath, m_fileCount, m_bytesScanned);
            }
        }
    }
}

QString ScanWorker::classifyCategory(const QString& extension) {
    if (extension.isEmpty()) return "Other";

    static const QHash<QString, QString> categories = {
        // Images
        {".jpg", "Images"}, {".jpeg", "Images"}, {".png", "Images"}, {".gif", "Images"},
        {".bmp", "Images"}, {".svg", "Images"}, {".webp", "Images"}, {".ico", "Images"},
        // Videos
        {".mp4", "Videos"}, {".avi", "Videos"}, {".mkv", "Videos"}, {".mov", "Videos"},
        {".wmv", "Videos"}, {".flv", "Videos"}, {".webm", "Videos"},
        // Audio
        {".mp3", "Audio"}, {".wav", "Audio"}, {".flac", "Audio"}, {".aac", "Audio"},
        {".ogg", "Audio"}, {".wma", "Audio"},
        // Documents
        {".pdf", "Documents"}, {".doc", "Documents"}, {".docx", "Documents"}, {".xls", "Documents"},
        {".xlsx", "Documents"}, {".ppt", "Documents"}, {".pptx", "Documents"}, {".txt", "Documents"},
        // Code
        {".js", "Code"}, {".ts", "Code"}, {".py", "Code"}, {".java", "Code"},
        {".c", "Code"}, {".cpp", "Code"}, {".h", "Code"}, {".hpp", "Code"}, {".css", "Code"}, 
        {".html", "Code"}, {".json", "Code"}, {".md", "Code"}, {".qss", "Code"}, {".cmake", "Code"},
        // Archives
        {".zip", "Archives"}, {".rar", "Archives"}, {".7z", "Archives"}, {".tar", "Archives"},
        {".gz", "Archives"},
        // Executables
        {".exe", "Executables"}, {".msi", "Executables"}, {".dll", "Executables"}, {".sys", "Executables"},
        // Fonts
        {".ttf", "Fonts"}, {".otf", "Fonts"}, {".woff", "Fonts"}, {".woff2", "Fonts"},
        // Data
        {".db", "Data"}, {".sqlite", "Data"}, {".parquet", "Data"}, {".csv", "Data"}
    };

    return categories.value(extension, "Other");
}

// --- ScanService Implementation ---

ScanService::ScanService(QObject* parent)
    : QObject(parent)
{
}

ScanService::~ScanService() {
    cancelScan();
}

void ScanService::startScan(const QString& path, int maxDepth) {
    cancelScan();

    m_worker = new ScanWorker(path, maxDepth, this);
    connect(m_worker, &ScanWorker::progressUpdated, this, &ScanService::progressReported);
    connect(m_worker, &ScanWorker::scanFinished, this, &ScanService::scanCompleted);
    connect(m_worker, &ScanWorker::finished, this, &ScanService::onWorkerFinished);

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

std::shared_ptr<FileSystemNode> ScanService::getScanResult() const {
    return m_worker ? m_worker->getResult() : nullptr;
}

qint64 ScanService::getFilesCount() const {
    return m_worker ? m_worker->getFileCount() : 0;
}

qint64 ScanService::getFoldersCount() const {
    return m_worker ? m_worker->getFolderCount() : 0;
}

void ScanService::onWorkerFinished() {
    // Thread has exited, but we keep the worker object alive until next scan to retrieve results
}
