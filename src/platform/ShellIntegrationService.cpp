#include "platform/ShellIntegrationService.h"

#include <QDesktopServices>
#include <QUrl>
#include <QFileInfo>
#include <QDir>
#include <QProcess>
#include <QClipboard>
#include <QGuiApplication>
#include <QStorageInfo>

#ifdef Q_OS_WIN
#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>
#endif

ShellIntegrationService::ShellIntegrationService(QObject* parent)
    : QObject(parent)
{
}

// --- Shell Actions ---

bool ShellIntegrationService::revealInExplorer(const QString& filePath) {
#ifdef Q_OS_WIN
    QString nativePath = QDir::toNativeSeparators(filePath);
    QString cmd = QString("explorer.exe /select,\"%1\"").arg(nativePath);
    return QProcess::startDetached("explorer.exe", QStringList() << "/select," << nativePath);
#elif defined(Q_OS_MACOS)
    return QProcess::startDetached("open", QStringList() << "-R" << filePath);
#else
    QFileInfo fi(filePath);
    return QDesktopServices::openUrl(QUrl::fromLocalFile(fi.absolutePath()));
#endif
}

bool ShellIntegrationService::openFile(const QString& filePath) {
    return QDesktopServices::openUrl(QUrl::fromLocalFile(filePath));
}

bool ShellIntegrationService::openFolder(const QString& folderPath) {
    return QDesktopServices::openUrl(QUrl::fromLocalFile(folderPath));
}

// --- Trash / Delete ---

bool ShellIntegrationService::moveToTrash(const QString& filePath) {
#ifdef Q_OS_WIN
    // Use SHFileOperation with FOF_ALLOWUNDO to send to Recycle Bin
    std::wstring wpath = filePath.toStdWString();
    wpath.push_back(L'\0'); // Double null-terminated

    SHFILEOPSTRUCTW fileOp = {};
    fileOp.wFunc = FO_DELETE;
    fileOp.pFrom = wpath.c_str();
    fileOp.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SILENT;

    int result = SHFileOperationW(&fileOp);
    if (result != 0) {
        emit operationFailed(QString("Failed to move to Recycle Bin: error code %1").arg(result));
        return false;
    }
    emit operationCompleted(true, QString("Moved to Recycle Bin: %1").arg(filePath));
    return true;
#else
    // Qt 5.15+ / Qt 6 has QFile::moveToTrash
    QFile file(filePath);
    if (file.moveToTrash()) {
        emit operationCompleted(true, QString("Moved to trash: %1").arg(filePath));
        return true;
    }
    emit operationFailed(QString("Failed to move to trash: %1").arg(filePath));
    return false;
#endif
}

bool ShellIntegrationService::moveToTrash(const QStringList& filePaths) {
    int success = 0, fail = 0;
    for (const QString& path : filePaths) {
        if (moveToTrash(path)) success++;
        else fail++;
    }
    emit operationCompleted(fail == 0,
        QString("Moved %1 files to trash, %2 failed").arg(success).arg(fail));
    return fail == 0;
}

bool ShellIntegrationService::permanentDelete(const QString& filePath) {
    QFileInfo fi(filePath);
    bool ok = false;
    if (fi.isDir()) {
        QDir dir(filePath);
        ok = dir.removeRecursively();
    } else {
        ok = QFile::remove(filePath);
    }
    if (ok) {
        emit operationCompleted(true, QString("Permanently deleted: %1").arg(filePath));
    } else {
        emit operationFailed(QString("Failed to delete: %1").arg(filePath));
    }
    return ok;
}

bool ShellIntegrationService::permanentDelete(const QStringList& filePaths) {
    int success = 0, fail = 0;
    for (const QString& path : filePaths) {
        QFileInfo fi(path);
        bool ok = fi.isDir() ? QDir(path).removeRecursively() : QFile::remove(path);
        if (ok) success++; else fail++;
    }
    emit operationCompleted(fail == 0,
        QString("Deleted %1 files, %2 failed").arg(success).arg(fail));
    return fail == 0;
}

// --- Drive Classification ---

ShellIntegrationService::DriveType ShellIntegrationService::classifyDrive(const QString& rootPath) {
#ifdef Q_OS_WIN
    std::wstring wpath = QDir::toNativeSeparators(rootPath).toStdWString();
    UINT type = GetDriveTypeW(wpath.c_str());
    switch (type) {
        case DRIVE_FIXED:     return DriveType::Fixed;
        case DRIVE_REMOVABLE: return DriveType::Removable;
        case DRIVE_REMOTE:    return DriveType::Network;
        case DRIVE_CDROM:     return DriveType::Optical;
        default:              return DriveType::Unknown;
    }
#else
    // On Linux/macOS, check if it's a network mount
    if (isNetworkPath(rootPath)) return DriveType::Network;

    QStorageInfo si(rootPath);
    QString device = si.device();
    if (device.contains("usb") || device.contains("removable"))
        return DriveType::Removable;
    return DriveType::Fixed;
#endif
}

bool ShellIntegrationService::isNetworkPath(const QString& path) {
#ifdef Q_OS_WIN
    // UNC paths (\\server\share) or mapped drives pointing to network
    if (path.startsWith("\\\\") || path.startsWith("//")) return true;
    std::wstring wpath = QDir::toNativeSeparators(path.left(3)).toStdWString();
    return GetDriveTypeW(wpath.c_str()) == DRIVE_REMOTE;
#else
    // Check /etc/mtab or mount output for nfs, cifs, smb
    QStorageInfo si(path);
    QString fsType = si.fileSystemType().toLower();
    return fsType.contains("nfs") || fsType.contains("cifs") || fsType.contains("smb");
#endif
}

QString ShellIntegrationService::driveTypeToString(DriveType type) {
    switch (type) {
        case DriveType::Fixed:     return "Fixed";
        case DriveType::Removable: return "Removable";
        case DriveType::Network:   return "Network";
        case DriveType::Optical:   return "Optical";
        default:                   return "Unknown";
    }
}

// --- Clipboard ---

void ShellIntegrationService::copyPathToClipboard(const QString& path) {
    QClipboard* clipboard = QGuiApplication::clipboard();
    if (clipboard) {
        clipboard->setText(QDir::toNativeSeparators(path));
    }
}
