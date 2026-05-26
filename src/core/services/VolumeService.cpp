#include "VolumeService.h"
#include <QStorageInfo>
#include <QFileInfo>
#include <QDir>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

VolumeService::VolumeService(QObject* parent)
    : QObject(parent)
{
}

QVector<VolumeInfo> VolumeService::getMountedVolumes() {
    QVector<VolumeInfo> volumes;
    auto mounted = QStorageInfo::mountedVolumes();

    for (const auto& storage : mounted) {
        if (!storage.isValid() || !storage.isReady()) {
            continue;
        }

        VolumeInfo vol;
        vol.path = storage.rootPath();
        vol.label = storage.displayName();
        if (vol.label.isEmpty()) {
            vol.label = storage.name();
        }
        if (vol.label.isEmpty()) {
            vol.label = "Local Disk";
        }
        
        vol.fileSystem = storage.fileSystemType();
        vol.totalBytes = storage.bytesTotal();
        vol.freeBytes = storage.bytesAvailable();
        vol.usedBytes = vol.totalBytes - vol.freeBytes;
        vol.isReady = storage.isReady();
        vol.isReadOnly = storage.isReadOnly();

        if (vol.totalBytes > 0) {
            vol.saturationPercent = (static_cast<double>(vol.usedBytes) / vol.totalBytes) * 100.0;
        } else {
            vol.saturationPercent = 0.0;
        }

        // Determine Drive Type
        vol.type = "Fixed"; // Default
#ifdef Q_OS_WIN
        std::wstring wpath = vol.path.toStdWString();
        UINT winType = GetDriveTypeW(wpath.c_str());
        switch (winType) {
            case DRIVE_REMOVABLE:
                vol.type = "Removable";
                break;
            case DRIVE_FIXED:
                vol.type = "Fixed";
                break;
            case DRIVE_REMOTE:
                vol.type = "Network";
                break;
            case DRIVE_CDROM:
                vol.type = "CD-ROM";
                break;
            case DRIVE_RAMDISK:
                vol.type = "RAM Disk";
                break;
            default:
                vol.type = "Unknown";
                break;
        }
#else
        // Fallback for non-Windows systems
        if (vol.path.startsWith("/Volumes") || vol.path.startsWith("/media") || vol.path.startsWith("/mnt")) {
            vol.type = "Removable";
        }
#endif
        volumes.push_back(vol);
    }

    return volumes;
}
