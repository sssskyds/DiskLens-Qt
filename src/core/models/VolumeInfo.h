#ifndef VOLUMEINFO_H
#define VOLUMEINFO_H

#include <QString>

/**
 * @brief Describes a single mounted storage volume.
 */
struct VolumeInfo {
    QString path;                ///< Mount point / root path
    QString label;               ///< Human-readable display name
    QString fileSystem;          ///< e.g. NTFS, ext4, apfs
    QString type;                ///< Fixed | Removable | Network | CD-ROM | RAM Disk | Unknown
    qint64  totalBytes    = 0;
    qint64  freeBytes     = 0;
    qint64  usedBytes     = 0;
    double  saturationPercent = 0.0;
    bool    isReady       = false;
    bool    isReadOnly    = false;

    /// @returns colour token for the usage gauge per spec thresholds.
    QString statusColor() const {
        if (saturationPercent >= 90.0) return QStringLiteral("#f85149"); // Danger
        if (saturationPercent >= 75.0) return QStringLiteral("#d29922"); // Warning
        return QStringLiteral("#00d4ff");                                 // Cyan
    }
};

#endif // VOLUMEINFO_H
