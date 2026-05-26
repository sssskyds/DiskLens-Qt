#ifndef VOLUMEINFO_H
#define VOLUMEINFO_H

#include <QString>

struct VolumeInfo {
    QString path;
    QString label;
    QString fileSystem;
    qint64 totalBytes = 0;
    qint64 freeBytes = 0;
    qint64 usedBytes = 0;
    double saturationPercent = 0.0;
    bool isReady = false;
    bool isReadOnly = false;
    QString type; // "Fixed", "Removable", "Network", etc.
};

#endif // VOLUMEINFO_H
