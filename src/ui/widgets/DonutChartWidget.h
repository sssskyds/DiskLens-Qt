#ifndef DONUTCHARTWIDGET_H
#define DONUTCHARTWIDGET_H

#include "core/services/FileTypeService.h"
#include <QWidget>
#include <QVector>

class DonutChartWidget : public QWidget {
    Q_OBJECT
public:
    explicit DonutChartWidget(QWidget* parent = nullptr);
    ~DonutChartWidget() = default;

    void setData(const QVector<CategoryStat>& data);
    void clear();

signals:
    void categoryHovered(const QString& categoryName, qint64 size, double percentage);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    struct Slice {
        QString name;
        qint64 size = 0;
        double percentage = 0.0;
        QString color;
        double startAngle = 0.0; // in degrees
        double spanAngle = 0.0;  // in degrees
    };

    QVector<CategoryStat> m_rawData;
    QVector<Slice> m_slices;
    int m_hoveredIndex = -1;
    qint64 m_totalBytes = 0;

    void updateSlices();
    int getSliceIndexAt(const QPoint& pos);
};

#endif // DONUTCHARTWIDGET_H
