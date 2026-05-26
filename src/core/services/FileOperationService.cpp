#include "FileOperationService.h"
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QDesktopServices>
#include <QUrl>
#include <QDebug>

#ifdef Q_OS_WIN
#include <windows.h>
#include <shellapi.h>
#endif

FileOperationService::FileOperationService(QObject* parent)
    : QObject(parent)
{
}

bool FileOperationService::moveToTrash(const QString& filePath) {
#ifdef Q_OS_WIN
    // Double null termination is required for SHFileOperationW
    std::wstring wpath = QDir::toNativeSeparators(filePath).toStdWString();
    wpath.append(L"\0\0", 2); // Append double-null character

    SHFILEOPSTRUCTW fileOp;
    memset(&fileOp, 0, sizeof(SHFILEOPSTRUCTW));
    fileOp.wFunc = FO_DELETE;
    fileOp.pFrom = wpath.c_str();
    fileOp.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SILENT;

    int result = SHFileOperationW(&fileOp);
    return (result == 0 && !fileOp.fAnyOperationsAborted);
#else
    // Fallback: trash folders/files via QDesktopServices or standard deletion
    // Usually on macOS/Linux we could call standard CLI commands, but we'll try QFile::moveToTrash if Qt 6+ is used,
    // otherwise fallback to renaming/removing.
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    return QFile::moveToTrash(filePath);
#else
    // Just delete permanently if no trash is available
    return deletePermanently(filePath);
#endif
#endif
}

bool FileOperationService::deletePermanently(const QString& filePath) {
    QFileInfo info(filePath);
    if (!info.exists()) return false;

    if (info.isDir()) {
        QDir dir(filePath);
        return dir.removeRecursively();
    } else {
        return QFile::remove(filePath);
    }
}

bool FileOperationService::renameFile(const QString& filePath, const QString& newName) {
    QFileInfo info(filePath);
    if (!info.exists()) return false;

    QString newPath = info.absoluteDir().filePath(newName);
    return QFile::rename(filePath, newPath);
}

bool FileOperationService::showInExplorer(const QString& filePath) {
#ifdef Q_OS_WIN
    QString nativePath = QDir::toNativeSeparators(filePath);
    QStringList args;
    args << "/select," << nativePath;
    return QProcess::startDetached("explorer.exe", args);
#else
    // Fallback for macOS / Linux
    QFileInfo info(filePath);
    QString dirPath = info.absoluteDir().absolutePath();
    return QDesktopServices::openUrl(QUrl::fromLocalFile(dirPath));
#endif
}

bool FileOperationService::openFile(const QString& filePath) {
    return QDesktopServices::openUrl(QUrl::fromLocalFile(filePath));
}
