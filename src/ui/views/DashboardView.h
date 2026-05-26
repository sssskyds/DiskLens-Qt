#ifndef DASHBOARDVIEW_H
#define DASHBOARDVIEW_H

#include "core/services/VolumeService.h"
#include <QWidget>
#include <QGridLayout>
#include <QLabel>
#include <QFrame>

class DashboardView : public QWidget {
    Q_OBJECT
public:
    explicit DashboardView(QWidget* parent = nullptr);
    ~DashboardView() = default;

    void refreshVolumes();

signals:
    void driveSelectedForScan(const QString& drivePath);

private:
    VolumeService m_volumeService;
    QGridLayout* m_cardsLayout = nullptr;
    QLabel* m_totalCapacityLabel = nullptr;
    QLabel* m_totalUsedLabel = nullptr;
    QLabel* m_totalFreeLabel = nullptr;

    void setupUI();
    QString formatBytes(qint64 bytes);
};

#endif // DASHBOARDVIEW_H
