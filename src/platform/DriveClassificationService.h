#pragma once
#ifndef DRIVECLASSIFICATIONSERVICE_H
#define DRIVECLASSIFICATIONSERVICE_H

#include <QString>
#include <QStorageInfo>

#ifdef Q_OS_WIN
#  include <windows.h>
#endif

/**
 * @brief Classifies a path as local / removable / network / cdrom / unknown.
 *
 * Used by VolumeService, SearchService (to gate content-search on network
 * paths) and any other module that needs to adapt behaviour based on
 * storage medium.
 */
class DriveClassificationService {
public:
    enum class DriveType {
        Fixed,
        Removable,
        Network,
        CdRom,
        RamDisk,
        Unknown
    };

    static DriveType classify(const QString& path) {
#ifdef Q_OS_WIN
        // Use the Windows API for definitive drive-type information.
        const QString root = QStorageInfo(path).rootPath();
        const std::wstring wpath = root.toStdWString();
        switch (GetDriveTypeW(wpath.c_str())) {
            case DRIVE_FIXED:     return DriveType::Fixed;
            case DRIVE_REMOVABLE: return DriveType::Removable;
            case DRIVE_REMOTE:    return DriveType::Network;
            case DRIVE_CDROM:     return DriveType::CdRom;
            case DRIVE_RAMDISK:   return DriveType::RamDisk;
            default:              return DriveType::Unknown;
        }
#elif defined(Q_OS_LINUX)
        return classifyLinux(path);
#elif defined(Q_OS_MACOS)
        return classifyMacOs(path);
#else
        Q_UNUSED(path)
        return DriveType::Unknown;
#endif
    }

    static QString typeName(DriveType t) {
        switch (t) {
            case DriveType::Fixed:     return "Fixed";
            case DriveType::Removable: return "Removable";
            case DriveType::Network:   return "Network";
            case DriveType::CdRom:     return "CD-ROM";
            case DriveType::RamDisk:   return "RAM Disk";
            default:                   return "Unknown";
        }
    }

    static bool isNetworkPath(const QString& path) {
        return classify(path) == DriveType::Network;
    }

private:
#ifdef Q_OS_LINUX
    static DriveType classifyLinux(const QString& path) {
        // Parse /proc/mounts to find the filesystem type for this path's mount
        // point.  Network FSes: nfs, nfs4, cifs, smb, smbfs, fuse.sshfs, etc.
        QStorageInfo si(path);
        const QString fsType = QString::fromLatin1(si.fileSystemType()).toLower();

        static const QStringList networkFs = {
            "nfs","nfs4","cifs","smb","smbfs","samba",
            "fuse.sshfs","fuse.s3fs","ncpfs","afs","gfs","glusterfs",
            "davfs","davfs2","ncpfs","coda"
        };
        for (const QString& fs : networkFs) {
            if (fsType.startsWith(fs)) return DriveType::Network;
        }

        static const QStringList cdFs = {"iso9660","udf","cdfs"};
        for (const QString& fs : cdFs) {
            if (fsType == fs) return DriveType::CdRom;
        }

        // Check /sys/block to distinguish removable from fixed.
        // Extract the block device name from the mount device string.
        const QString device = QString::fromLatin1(si.device());
        // e.g. /dev/sdb1 → sdb
        QString devName;
        if (device.startsWith("/dev/")) {
            devName = device.mid(5);
            // Strip partition number: sdb1 → sdb, nvme0n1p1 → nvme0n1
            while (!devName.isEmpty() && devName.back().isDigit()) {
                devName.chop(1);
            }
        }

        if (!devName.isEmpty()) {
            QFile removableFile("/sys/block/" + devName + "/removable");
            if (removableFile.open(QIODevice::ReadOnly)) {
                const QByteArray val = removableFile.readAll().trimmed();
                removableFile.close();
                if (val == "1") return DriveType::Removable;
            }
        }

        return DriveType::Fixed;
    }
#endif

#ifdef Q_OS_MACOS
    static DriveType classifyMacOs(const QString& path) {
        QStorageInfo si(path);
        const QString fsType = QString::fromLatin1(si.fileSystemType()).toLower();

        static const QStringList networkFs = {
            "nfs","smbfs","afpfs","webdav","ftpfs","cifs","osxfuse"
        };
        for (const QString& fs : networkFs) {
            if (fsType.startsWith(fs)) return DriveType::Network;
        }

        if (fsType == "cd9660" || fsType == "udf") return DriveType::CdRom;

        // /Volumes paths that are not the main boot volume are removable or
        // external (good enough heuristic for macOS).
        const QString root = si.rootPath();
        if (root.startsWith("/Volumes/")) return DriveType::Removable;

        return DriveType::Fixed;
    }
#endif
};

#endif // DRIVECLASSIFICATIONSERVICE_H
