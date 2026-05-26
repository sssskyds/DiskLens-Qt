#include "FileOperationService.h"
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QDesktopServices>
#include <QUrl>
#include <QDebug>

#ifdef Q_OS_WIN
#  include <windows.h>
#  include <shellapi.h>
#endif

FileOperationService::FileOperationService(QObject* parent)
    : QObject(parent)
{
}

// ── Validation helper ─────────────────────────────────────────────

bool FileOperationService::isNameSafe(const QString& name) {
    if (name.isEmpty())           return false;
    if (name == "." || name == "..") return false;
    // Block path separators and characters illegal on Windows
    static const QString forbidden = "/\\:*?\"<>|";
    for (const QChar& ch : name) {
        if (forbidden.contains(ch)) return false;
    }
    // Block pure-whitespace or dot-only names
    if (name.trimmed().isEmpty())  return false;
    return true;
}

// ── Conflict resolution ───────────────────────────────────────────

QString FileOperationService::resolveConflict(const QString& destDir,
                                              const QString& fileName,
                                              ConflictPolicy policy)
{
    const QString candidate = destDir + QDir::separator() + fileName;
    if (!QFileInfo::exists(candidate)) return candidate;

    switch (policy) {
        case ConflictPolicy::Skip:
            return {};  // Empty string signals "skip"

        case ConflictPolicy::Overwrite:
            return candidate;

        case ConflictPolicy::AutoRename: {
            QFileInfo fi(fileName);
            const QString base   = fi.completeBaseName();
            const QString suffix = fi.suffix().isEmpty() ? QString{} : (QChar('.') + fi.suffix());
            for (int i = 1; i < 10000; ++i) {
                const QString newName = base + QString(" (%1)").arg(i) + suffix;
                const QString newPath = destDir + QDir::separator() + newName;
                if (!QFileInfo::exists(newPath)) return newPath;
            }
            return {};  // Give up
        }
    }
    return {};
}

// ── Trash / delete ────────────────────────────────────────────────

OperationResult FileOperationService::moveToTrash(const QString& filePath) {
#ifdef Q_OS_WIN
    std::wstring wpath = QDir::toNativeSeparators(filePath).toStdWString();
    wpath += L'\0';  // Double-null termination required by SHFileOperationW
    SHFILEOPSTRUCTW op = {};
    op.wFunc  = FO_DELETE;
    op.pFrom  = wpath.c_str();
    op.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SILENT;
    const int rc = SHFileOperationW(&op);
    if (rc != 0 || op.fAnyOperationsAborted)
        return OperationResult::fail(QString("SHFileOperationW failed (code %1)").arg(rc));
    return OperationResult::ok();
#else
    // QFile::moveToTrash is available since Qt 5.15 and works on Linux
    // (freedesktop spec) and macOS (Finder trash).
    QString errorString;
    if (!QFile::moveToTrash(filePath, &errorString))
        return OperationResult::fail(errorString.isEmpty()
                                         ? QStringLiteral("Move to trash failed")
                                         : errorString);
    return OperationResult::ok();
#endif
}

OperationResult FileOperationService::deletePermanently(const QString& filePath) {
    QFileInfo info(filePath);
    if (!info.exists())
        return OperationResult::fail(QStringLiteral("Path does not exist: ") + filePath);

    bool ok;
    if (info.isDir()) {
        ok = QDir(filePath).removeRecursively();
    } else {
        ok = QFile::remove(filePath);
    }
    return ok ? OperationResult::ok()
               : OperationResult::fail(QStringLiteral("Delete failed: ") + filePath);
}

BatchDeleteResult FileOperationService::batchMoveToTrash(const QStringList& paths) {
    BatchDeleteResult result;
    for (const QString& p : paths) {
        const auto r = moveToTrash(p);
        if (r.success) ++result.succeeded;
        else { ++result.failed; result.failedPaths << p; }
    }
    return result;
}

BatchDeleteResult FileOperationService::batchDeletePermanently(const QStringList& paths) {
    BatchDeleteResult result;
    for (const QString& p : paths) {
        const auto r = deletePermanently(p);
        if (r.success) ++result.succeeded;
        else { ++result.failed; result.failedPaths << p; }
    }
    return result;
}

// ── Copy / move ───────────────────────────────────────────────────

OperationResult FileOperationService::copyFile(const QString& sourcePath,
                                               const QString& destDir,
                                               ConflictPolicy policy)
{
    QFileInfo srcInfo(sourcePath);
    if (!srcInfo.exists())
        return OperationResult::fail(QStringLiteral("Source does not exist: ") + sourcePath);

    QDir dir(destDir);
    if (!dir.exists() && !dir.mkpath("."))
        return OperationResult::fail(QStringLiteral("Cannot create destination directory: ") + destDir);

    const QString destPath = resolveConflict(destDir, srcInfo.fileName(), policy);
    if (destPath.isEmpty())
        return OperationResult::cancelled();  // Skip policy or out of auto-rename slots

    // For a directory: recursive copy
    if (srcInfo.isDir()) {
        QDir srcDir(sourcePath);
        const QFileInfoList entries = srcDir.entryInfoList(
            QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
        if (!QDir().mkpath(destPath))
            return OperationResult::fail(QStringLiteral("mkpath failed: ") + destPath);
        for (const QFileInfo& e : entries) {
            auto r = copyFile(e.absoluteFilePath(), destPath, policy);
            if (!r.success && !r.userCancelled) return r;
        }
        return OperationResult::ok();
    }

    // Remove destination if Overwrite policy and it exists
    if (policy == ConflictPolicy::Overwrite && QFileInfo::exists(destPath))
        QFile::remove(destPath);

    if (!QFile::copy(sourcePath, destPath))
        return OperationResult::fail(QStringLiteral("Copy failed: ") + sourcePath
                                     + QStringLiteral(" -> ") + destPath);
    return OperationResult::ok();
}

OperationResult FileOperationService::moveFile(const QString& sourcePath,
                                               const QString& destDir,
                                               ConflictPolicy policy)
{
    QFileInfo srcInfo(sourcePath);
    if (!srcInfo.exists())
        return OperationResult::fail(QStringLiteral("Source does not exist: ") + sourcePath);

    QDir dir(destDir);
    if (!dir.exists() && !dir.mkpath("."))
        return OperationResult::fail(QStringLiteral("Cannot create destination: ") + destDir);

    const QString destPath = resolveConflict(destDir, srcInfo.fileName(), policy);
    if (destPath.isEmpty()) return OperationResult::cancelled();

    // Try atomic rename first (same volume)
    if (QFile::rename(sourcePath, destPath)) return OperationResult::ok();

    // Cross-volume fallback: copy then delete
    OperationResult copyResult = copyFile(sourcePath, destDir, policy);
    if (!copyResult.success) return copyResult;

    return deletePermanently(sourcePath);
}

// ── Rename ────────────────────────────────────────────────────────

OperationResult FileOperationService::renameFile(const QString& filePath,
                                                 const QString& newName)
{
    if (!isNameSafe(newName))
        return OperationResult::fail(
            QStringLiteral("Invalid name '") + newName +
            QStringLiteral("' — contains forbidden characters or path separators."));

    QFileInfo info(filePath);
    if (!info.exists())
        return OperationResult::fail(QStringLiteral("File does not exist: ") + filePath);

    const QString newPath = info.absoluteDir().filePath(newName);
    if (QFileInfo::exists(newPath))
        return OperationResult::fail(QStringLiteral("A file named '") + newName
                                     + QStringLiteral("' already exists."));

    return QFile::rename(filePath, newPath)
               ? OperationResult::ok()
               : OperationResult::fail(QStringLiteral("Rename failed: ") + filePath);
}

// ── Create folder ─────────────────────────────────────────────────

OperationResult FileOperationService::createFolder(const QString& parentPath,
                                                    const QString& folderName)
{
    if (!isNameSafe(folderName))
        return OperationResult::fail(QStringLiteral("Invalid folder name: ") + folderName);

    const QString newPath = QDir(parentPath).filePath(folderName);
    if (QFileInfo::exists(newPath))
        return OperationResult::fail(QStringLiteral("Folder already exists: ") + newPath);

    return QDir().mkpath(newPath)
               ? OperationResult::ok()
               : OperationResult::fail(QStringLiteral("Could not create folder: ") + newPath);
}

// ── Shell integration ─────────────────────────────────────────────

OperationResult FileOperationService::showInExplorer(const QString& filePath) {
#ifdef Q_OS_WIN
    const QString native = QDir::toNativeSeparators(filePath);
    const bool ok = QProcess::startDetached(
        "explorer.exe", {"/select," + native});
    return ok ? OperationResult::ok()
               : OperationResult::fail("Failed to launch Explorer");
#elif defined(Q_OS_MACOS)
    const bool ok = QProcess::startDetached(
        "open", {"-R", filePath});
    return ok ? OperationResult::ok()
               : OperationResult::fail("Failed to launch Finder");
#else
    // Linux: open the containing directory with xdg-open
    const QString dir = QFileInfo(filePath).absolutePath();
    const bool ok = QProcess::startDetached("xdg-open", {dir});
    return ok ? OperationResult::ok()
               : OperationResult::fail("Failed to open file manager");
#endif
}

OperationResult FileOperationService::openFile(const QString& filePath) {
    const bool ok = QDesktopServices::openUrl(QUrl::fromLocalFile(filePath));
    return ok ? OperationResult::ok()
               : OperationResult::fail(QStringLiteral("Failed to open: ") + filePath);
}
