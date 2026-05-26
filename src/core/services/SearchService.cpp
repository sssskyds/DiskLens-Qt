#include "core/services/SearchService.h"

#include <QDirIterator>
#include <QFileInfo>
#include <QRegularExpression>
#include <QTextStream>
#include <QFile>

// ─── SearchWorker ────────────────────────────────────────────────

SearchWorker::SearchWorker(const QString& rootPath, const SearchCriteria& criteria, QObject* parent)
    : QThread(parent), m_rootPath(rootPath), m_criteria(criteria)
{
}

void SearchWorker::cancel() {
    QMutexLocker lock(&m_mutex);
    m_cancelRequested = true;
}

void SearchWorker::run() {
    m_filesChecked = 0;
    m_matchesFound = 0;
    m_batch.clear();
    m_cancelRequested = false;

    searchDirectory(m_rootPath, 0);

    // Flush remaining batch
    if (!m_batch.isEmpty()) {
        emit resultsBatch(m_batch);
        m_batch.clear();
    }

    bool cancelled;
    {
        QMutexLocker lock(&m_mutex);
        cancelled = m_cancelRequested;
    }
    emit searchFinished(m_matchesFound, cancelled);
}

void SearchWorker::searchDirectory(const QString& path, int depth) {
    {
        QMutexLocker lock(&m_mutex);
        if (m_cancelRequested) return;
    }

    if (depth > m_criteria.maxDepth) return;

    QDir dir(path);
    if (!dir.exists()) return;

    QDir::Filters filters = QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot;
    if (m_criteria.includeHidden) filters |= QDir::Hidden;

    QFileInfoList entries = dir.entryInfoList(filters, QDir::DirsFirst);
    for (const QFileInfo& fi : entries) {
        {
            QMutexLocker lock(&m_mutex);
            if (m_cancelRequested) return;
        }

        // Check excluded patterns
        bool excluded = false;
        for (const QString& pat : m_criteria.excludePatterns) {
            QRegularExpression rx(QRegularExpression::wildcardToRegularExpression(pat),
                                  QRegularExpression::CaseInsensitiveOption);
            if (rx.match(fi.fileName()).hasMatch()) {
                excluded = true;
                break;
            }
        }
        if (excluded) continue;

        if (fi.isDir()) {
            searchDirectory(fi.absoluteFilePath(), depth + 1);
            continue;
        }

        m_filesChecked++;
        QString category = classifyCategory(fi.suffix().toLower());

        if (matchesCriteria(fi, category)) {
            auto node = std::make_shared<FileSystemNode>(fi.fileName(), fi.absoluteFilePath(), false);
            node->setSize(fi.size());
            node->setLastModified(fi.lastModified());
            node->setExtension(fi.suffix().toLower());
            node->setCategory(category);

            m_matchesFound++;
            m_batch.append(node);

            if (m_batch.size() >= BATCH_SIZE) {
                emit resultsBatch(m_batch);
                m_batch.clear();
            }
        }

        // Progress update every 500 files
        if (m_filesChecked % 500 == 0) {
            emit progressUpdated(m_filesChecked, m_matchesFound, fi.absolutePath());
        }
    }
}

bool SearchWorker::matchesCriteria(const QFileInfo& fi, const QString& category) {
    // Cheapest-first filter evaluation order:

    // 1. Name text filter
    if (!m_criteria.nameText.isEmpty()) {
        Qt::CaseSensitivity cs = m_criteria.caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive;
        if (m_criteria.regex) {
            QRegularExpression::PatternOptions opts = m_criteria.caseSensitive
                ? QRegularExpression::NoPatternOption
                : QRegularExpression::CaseInsensitiveOption;
            QRegularExpression rx(m_criteria.nameText, opts);
            if (!rx.match(fi.fileName()).hasMatch()) return false;
        } else {
            if (!fi.fileName().contains(m_criteria.nameText, cs)) return false;
        }
    }

    // 2. Glob pattern
    if (!m_criteria.globPattern.isEmpty()) {
        QRegularExpression rx(QRegularExpression::wildcardToRegularExpression(m_criteria.globPattern),
                              QRegularExpression::CaseInsensitiveOption);
        if (!rx.match(fi.fileName()).hasMatch()) return false;
    }

    // 3. Extension filter
    if (!m_criteria.extensions.isEmpty()) {
        QString ext = fi.suffix().toLower();
        bool found = false;
        for (const QString& e : m_criteria.extensions) {
            if (e.compare(ext, Qt::CaseInsensitive) == 0) { found = true; break; }
        }
        if (!found) return false;
    }

    // 4. Size filter
    if (m_criteria.minSize.has_value() && fi.size() < (qint64)m_criteria.minSize.value()) return false;
    if (m_criteria.maxSize.has_value() && fi.size() > (qint64)m_criteria.maxSize.value()) return false;

    // 5. Date filters
    if (m_criteria.modifiedAfter.has_value() && fi.lastModified() < m_criteria.modifiedAfter.value()) return false;
    if (m_criteria.modifiedBefore.has_value() && fi.lastModified() > m_criteria.modifiedBefore.value()) return false;
    if (m_criteria.createdAfter.has_value() && fi.birthTime().isValid() && fi.birthTime() < m_criteria.createdAfter.value()) return false;
    if (m_criteria.createdBefore.has_value() && fi.birthTime().isValid() && fi.birthTime() > m_criteria.createdBefore.value()) return false;

    // 6. Category filter
    if (!m_criteria.categories.isEmpty()) {
        if (!m_criteria.categories.contains(category, Qt::CaseInsensitive)) return false;
    }

    // 7. Content search (most expensive — last!)
    if (!m_criteria.containsText.isEmpty()) {
        if (!matchesContent(fi.absoluteFilePath())) return false;
    }

    return true;
}

bool SearchWorker::matchesContent(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return false;

    // Limit content search to first 10 MB
    const qint64 MAX_CONTENT_SIZE = 10 * 1024 * 1024;
    QTextStream stream(&file);
    qint64 bytesRead = 0;

    while (!stream.atEnd() && bytesRead < MAX_CONTENT_SIZE) {
        QString line = stream.readLine();
        bytesRead += line.size();
        if (m_criteria.regex) {
            QRegularExpression::PatternOptions opts = m_criteria.caseSensitive
                ? QRegularExpression::NoPatternOption
                : QRegularExpression::CaseInsensitiveOption;
            QRegularExpression rx(m_criteria.containsText, opts);
            if (rx.match(line).hasMatch()) return true;
        } else {
            Qt::CaseSensitivity cs = m_criteria.caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive;
            if (line.contains(m_criteria.containsText, cs)) return true;
        }
    }
    return false;
}

QString SearchWorker::classifyCategory(const QString& ext) {
    static const QStringList images = {"jpg","jpeg","png","gif","bmp","svg","ico","tiff","webp","raw","cr2","nef"};
    static const QStringList videos = {"mp4","avi","mkv","mov","wmv","flv","webm","m4v","mpg","mpeg"};
    static const QStringList audio  = {"mp3","wav","flac","aac","ogg","wma","m4a","opus","aiff"};
    static const QStringList docs   = {"pdf","doc","docx","xls","xlsx","ppt","pptx","txt","rtf","odt","ods","csv"};
    static const QStringList code   = {"cpp","c","h","hpp","py","js","ts","java","cs","go","rs","rb","php","html","css","sql","sh","bat"};
    static const QStringList archives = {"zip","rar","7z","tar","gz","bz2","xz","iso","dmg"};
    static const QStringList exes   = {"exe","msi","dll","so","dylib","app","deb","rpm","apk"};
    static const QStringList fonts  = {"ttf","otf","woff","woff2","eot"};
    static const QStringList data   = {"json","xml","yaml","yml","toml","ini","cfg","conf","db","sqlite"};

    if (images.contains(ext)) return "Images";
    if (videos.contains(ext)) return "Videos";
    if (audio.contains(ext))  return "Audio";
    if (docs.contains(ext))   return "Documents";
    if (code.contains(ext))   return "Code";
    if (archives.contains(ext)) return "Archives";
    if (exes.contains(ext))   return "Executables";
    if (fonts.contains(ext))  return "Fonts";
    if (data.contains(ext))   return "Data";
    return "Other";
}

// ─── SearchService ───────────────────────────────────────────────

SearchService::SearchService(QObject* parent) : QObject(parent) {}

SearchService::~SearchService() {
    cancelSearch();
}

void SearchService::startSearch(const QString& rootPath, const SearchCriteria& criteria) {
    cancelSearch();

    m_worker = new SearchWorker(rootPath, criteria);
    connect(m_worker, &SearchWorker::resultsBatch, this, &SearchService::resultsBatch);
    connect(m_worker, &SearchWorker::progressUpdated, this, &SearchService::progressUpdated);
    connect(m_worker, &SearchWorker::searchFinished, this, &SearchService::searchFinished);
    connect(m_worker, &SearchWorker::finished, m_worker, &QObject::deleteLater);
    connect(m_worker, &SearchWorker::finished, this, [this]() { m_worker = nullptr; });
    m_worker->start();
}

void SearchService::cancelSearch() {
    if (m_worker && m_worker->isRunning()) {
        m_worker->cancel();
        m_worker->wait(5000);
    }
}

bool SearchService::isSearching() const {
    return m_worker && m_worker->isRunning();
}
