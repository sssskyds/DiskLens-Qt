#ifndef VOLUMESERVICE_H
#define VOLUMESERVICE_H

#include "core/models/VolumeInfo.h"
#include <QVector>
#include <QObject>

class VolumeService : public QObject {
    Q_OBJECT
public:
    explicit VolumeService(QObject* parent = nullptr);
    ~VolumeService() = default;

    QVector<VolumeInfo> getMountedVolumes();
};

#endif // VOLUMESERVICE_H
