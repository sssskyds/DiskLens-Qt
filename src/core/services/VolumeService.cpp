#include "VolumeService.h"
#include "platform/DriveClassificationService.h"
#include <QStorageInfo>

VolumeService::VolumeService(QObject* parent)
    : QObject(parent)
{
}

void VolumeService::refresh() {
    emit volumesChanged();
}

QVector<VolumeInfo> VolumeService::getMountedVolumes() {
    QVector<VolumeInfo> volumes;

    for (const QStorageInfo& storage : QStorageInfo::mountedVolumes()) {
        if (!storage.isValid() || !storage.isReady()) continue;

        VolumeInfo vol;
        vol.path       = storage.rootPath();
        vol.fileSystem = QString::fromLatin1(storage.fileSystemType());
        vol.totalBytes = storage.bytesTotal();
        vol.freeBytes  = storage.bytesAvailable();
        vol.usedBytes  = vol.totalBytes - vol.freeBytes;
        vol.isReady    = storage.isReady();
        vol.isReadOnly = storage.isReadOnly();

        // Prefer the display name; fall back to name(), then root path
        vol.label = storage.displayName();
        if (vol.label.isEmpty()) vol.label = storage.name();
        if (vol.label.isEmpty()) vol.label = vol.path;

        if (vol.totalBytes > 0) {
            vol.saturationPercent =
                (static_cast<double>(vol.usedBytes) / vol.totalBytes) * 100.0;
        }

        // Delegate drive-type classification to the platform service so
        // the detection logic lives in one place and is properly cross-platform.
        const auto dt = DriveClassificationService::classify(vol.path);
        vol.type = DriveClassificationService::typeName(dt);

        volumes.push_back(vol);
    }

    return volumes;
}
