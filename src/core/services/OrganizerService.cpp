#include "core/services/OrganizerService.h"
#include <QDir>
#include <QFileInfo>
#include <QDirIterator>
#include <QRegularExpression>

OrganizerService::OrganizerService(QObject* parent) : QObject(parent) {}

QString OrganizerService::ruleToString(OrganizerRule rule) {
    switch (rule) {
        case OrganizerRule::ByExtensionCategory: return "By File Type Category";
        case OrganizerRule::ByYearMonth:         return "By Year/Month (Modified Date)";
        case OrganizerRule::BySizeTier:          return "By Size Tier";
        case OrganizerRule::ByFirstLetter:       return "By First Letter";
        case OrganizerRule::ByCustomGlob:        return "By Custom Pattern";
    }
    return "Unknown";
}

QStringList OrganizerService::availableRules() {
    return {
        ruleToString(OrganizerRule::ByExtensionCategory),
        ruleToString(OrganizerRule::ByYearMonth),
        ruleToString(OrganizerRule::BySizeTier),
        ruleToString(OrganizerRule::ByFirstLetter),
        ruleToString(OrganizerRule::ByCustomGlob)
    };
}

QVector<OrganizeAction> OrganizerService::preview(const QString& sourceDir,
                                                    OrganizerRule rule,
                                                    bool recursive,
                                                    const QString& customGlob,
                                                    const QString& customFolder) {
    QVector<OrganizeAction> actions;
    QDir dir(sourceDir);
    if (!dir.exists()) return actions;

    QDir::Filters filters = QDir::Files | QDir::NoDotAndDotDot;
    QDirIterator::IteratorFlags itFlags = recursive
        ? QDirIterator::Subdirectories
        : QDirIterator::NoIteratorFlags;

    QDirIterator it(sourceDir, filters, itFlags);
    while (it.hasNext()) {
        it.next();
        QFileInfo fi = it.fileInfo();
        QString targetFolder;

        switch (rule) {
            case OrganizerRule::ByExtensionCategory:
                targetFolder = classifyByExtension(fi.suffix().toLower());
                break;
            case OrganizerRule::ByYearMonth:
                targetFolder = classifyByYearMonth(fi.lastModified());
                break;
            case OrganizerRule::BySizeTier:
                targetFolder = classifyBySizeTier(fi.size());
                break;
            case OrganizerRule::ByFirstLetter: {
                targetFolder = classifyByFirstLetter(fi.fileName());
                break;
            }
            case OrganizerRule::ByCustomGlob: {
                if (!customGlob.isEmpty()) {
                    QRegularExpression rx(QRegularExpression::wildcardToRegularExpression(customGlob),
                                          QRegularExpression::CaseInsensitiveOption);
                    if (rx.match(fi.fileName()).hasMatch()) {
                        targetFolder = customFolder.isEmpty() ? "Matched" : customFolder;
                    } else {
                        continue; // Skip non-matching files
                    }
                }
                break;
            }
        }

        if (targetFolder.isEmpty()) continue;

        QString destPath = sourceDir + "/" + targetFolder + "/" + fi.fileName();

        OrganizeAction action;
        action.sourcePath = fi.absoluteFilePath();
        action.destinationPath = destPath;
        action.ruleApplied = ruleToString(rule) + " → " + targetFolder;
        action.sizeBytes = fi.size();

        // Check for conflict
        if (QFile::exists(destPath)) {
            action.hasConflict = true;
            action.conflictInfo = "File already exists at destination";
        }

        actions.append(action);
    }

    return actions;
}

void OrganizerService::apply(const QVector<OrganizeAction>& actions,
                              ConflictResolution conflictMode) {
    m_cancelRequested = false;
    int succeeded = 0, failed = 0, skipped = 0;

    for (int i = 0; i < actions.size(); i++) {
        if (m_cancelRequested) {
            skipped += actions.size() - i;
            break;
        }

        const OrganizeAction& action = actions[i];
        emit applyProgress(i + 1, actions.size(), action.sourcePath);

        // Create destination directory
        QFileInfo destInfo(action.destinationPath);
        QDir().mkpath(destInfo.absolutePath());

        QString finalDest = action.destinationPath;

        if (action.hasConflict) {
            switch (conflictMode) {
                case ConflictResolution::Skip:
                    skipped++;
                    continue;
                case ConflictResolution::Overwrite:
                    QFile::remove(finalDest);
                    break;
                case ConflictResolution::AutoRename:
                    finalDest = resolveConflict(finalDest, conflictMode);
                    break;
            }
        }

        if (QFile::rename(action.sourcePath, finalDest)) {
            succeeded++;
        } else {
            failed++;
            emit error(QString("Failed to move: %1").arg(action.sourcePath));
        }
    }

    emit applyFinished(succeeded, failed, skipped);
}

void OrganizerService::cancel() {
    m_cancelRequested = true;
}

// ─── Classification Helpers ──────────────────────────────────────

QString OrganizerService::classifyByExtension(const QString& ext) {
    static const QStringList images = {"jpg","jpeg","png","gif","bmp","svg","ico","tiff","webp","raw","cr2","nef"};
    static const QStringList videos = {"mp4","avi","mkv","mov","wmv","flv","webm","m4v"};
    static const QStringList audio  = {"mp3","wav","flac","aac","ogg","wma","m4a","opus"};
    static const QStringList docs   = {"pdf","doc","docx","xls","xlsx","ppt","pptx","txt","rtf","odt","csv"};
    static const QStringList code   = {"cpp","c","h","hpp","py","js","ts","java","cs","go","rs","rb","php","html","css","sql"};
    static const QStringList archives = {"zip","rar","7z","tar","gz","bz2","xz","iso"};
    static const QStringList exes   = {"exe","msi","dll","so","dylib","app"};
    static const QStringList fonts  = {"ttf","otf","woff","woff2"};
    static const QStringList data   = {"json","xml","yaml","yml","toml","ini","db","sqlite"};

    if (images.contains(ext))   return "Images";
    if (videos.contains(ext))   return "Videos";
    if (audio.contains(ext))    return "Audio";
    if (docs.contains(ext))     return "Documents";
    if (code.contains(ext))     return "Code";
    if (archives.contains(ext)) return "Archives";
    if (exes.contains(ext))     return "Executables";
    if (fonts.contains(ext))    return "Fonts";
    if (data.contains(ext))     return "Data";
    return "Other";
}

QString OrganizerService::classifyBySizeTier(qint64 bytes) {
    if (bytes < 1024)                return "Tiny (< 1 KB)";
    if (bytes < 1024 * 1024)         return "Small (1 KB - 1 MB)";
    if (bytes < 100 * 1024 * 1024)   return "Medium (1 MB - 100 MB)";
    if (bytes < 1024LL * 1024 * 1024) return "Large (100 MB - 1 GB)";
    return "Huge (> 1 GB)";
}

QString OrganizerService::classifyByYearMonth(const QDateTime& modified) {
    if (!modified.isValid()) return "Unknown Date";
    return modified.toString("yyyy") + "/" + modified.toString("MM - MMMM");
}

QString OrganizerService::classifyByFirstLetter(const QString& name) {
    if (name.isEmpty()) return "#";
    QChar first = name[0].toUpper();
    if (first.isLetter()) return QString(first);
    if (first.isDigit()) return "0-9";
    return "#";
}

QString OrganizerService::resolveConflict(const QString& destPath, ConflictResolution mode) {
    if (mode != ConflictResolution::AutoRename) return destPath;

    QFileInfo fi(destPath);
    QString baseName = fi.completeBaseName();
    QString suffix = fi.suffix();
    QString dir = fi.absolutePath();

    for (int i = 2; i < 10000; i++) {
        QString newName = QString("%1/%2 (%3).%4").arg(dir, baseName).arg(i).arg(suffix);
        if (!QFile::exists(newName)) return newName;
    }
    return destPath; // Fallback
}
