#include "DonutChartWidget.h"
#include <QPainter>
#include <QMouseEvent>
#include <QPainterPath>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

DonutChartWidget::DonutChartWidget(QWidget* parent)
    : QWidget(parent)
{
    setMinimumSize(250, 250);
    setMouseTracking(true); // Crucial for hover tracking!
}

void DonutChartWidget::setData(const QVector<CategoryStat>& data) {
    m_rawData = data;
    m_hoveredIndex = -1;
    updateSlices();
    update();
}

void DonutChartWidget::clear() {
    m_rawData.clear();
    m_slices.clear();
    m_hoveredIndex = -1;
    m_totalBytes = 0;
    update();
}

void DonutChartWidget::updateSlices() {
    m_slices.clear();
    m_totalBytes = 0;

    for (const auto& cat : m_rawData) {
        m_totalBytes += cat.totalSize;
    }

    if (m_totalBytes == 0) return;

    double currentAngle = 90.0; // Start at 12 o'clock (90 degrees in standard math)

    for (const auto& cat : m_rawData) {
        Slice slice;
        slice.name = cat.categoryName;
        slice.size = cat.totalSize;
        slice.percentage = cat.percentage;
        slice.color = cat.color;

        // Angle span proportional to size
        slice.spanAngle = -(cat.percentage * 3.6); // Negative because Qt drawPie goes clockwise
        slice.startAngle = currentAngle;

        m_slices.push_back(slice);
        currentAngle += slice.spanAngle; // Subtract span angle to go clockwise
    }
}

int DonutChartWidget::getSliceIndexAt(const QPoint& pos) {
    if (m_slices.isEmpty() || m_totalBytes == 0) return -1;

    double cx = width() / 2.0;
    double cy = height() / 2.0;
    double dx = pos.x() - cx;
    double dy = pos.y() - cy; // Qt y coordinates grow down, so standard math is inverted

    double dist = std::sqrt(dx*dx + dy*dy);
    int outerRadius = std::min(width(), height()) / 2 - 15;
    int innerRadius = outerRadius * 0.65;

    // Check if mouse is inside the donut band
    if (dist < innerRadius || dist > outerRadius + 8) {
        return -1;
    }

    // Calculate angle in degrees [0, 360) starting at 3 o'clock (0 rad) going counter-clockwise
    // Let's convert to match Qt's startAngle coordinate system
    double angleRad = std::atan2(-dy, dx); // Negative dy because y goes down
    double angleDeg = angleRad * 180.0 / M_PI;
    if (angleDeg < 0.0) {
        angleDeg += 360.0;
    }

    // Now convert standard math degrees to the Qt angle system
    // In Qt: 90 is 12 o'clock, 0 is 3 o'clock, 270 is 6 o'clock, 180 is 9 o'clock.
    // We want to see which slice this angle falls into.
    for (int i = 0; i < m_slices.size(); ++i) {
        double start = m_slices[i].startAngle;
        double span = m_slices[i].spanAngle;

        // Since span is negative, the slice occupies [start + span, start]
        double lowerBound = start + span;
        double upperBound = start;

        // Normalize bounds to [0, 360)
        double normAngle = angleDeg;

        // We need to handle slices crossing the 0-degree boundary
        double normLower = lowerBound;
        double normUpper = upperBound;

        while (normLower < 0) { normLower += 360.0; normUpper += 360.0; }
        while (normAngle < normLower) { normAngle += 360.0; }

        if (normAngle >= normLower && normAngle <= normUpper) {
            return i;
        }
    }

    return -1;
}

void DonutChartWidget::mouseMoveEvent(QMouseEvent* event) {
    int prevHovered = m_hoveredIndex;
    m_hoveredIndex = getSliceIndexAt(event->pos());

    if (m_hoveredIndex != prevHovered) {
        update(); // Force repaint to animate
        if (m_hoveredIndex != -1) {
            const auto& slice = m_slices[m_hoveredIndex];
            emit categoryHovered(slice.name, slice.size, slice.percentage);
        }
    }
}

void DonutChartWidget::leaveEvent(QEvent* event) {
    Q_UNUSED(event);
    if (m_hoveredIndex != -1) {
        m_hoveredIndex = -1;
        update();
    }
}

void DonutChartWidget::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    int size = std::min(width(), height()) - 30;
    int cx = width() / 2;
    int cy = height() / 2;

    int defaultRadius = size / 2;
    int innerRadius = defaultRadius * 0.65;

    // Draw slices
    for (int i = 0; i < m_slices.size(); ++i) {
        const auto& slice = m_slices[i];
        
        bool isHovered = (i == m_hoveredIndex);
        int radius = isHovered ? (defaultRadius + 8) : defaultRadius; // Grows +8px when hovered!

        QRectF rect(cx - radius, cy - radius, radius * 2, radius * 2);

        QPainterPath path;
        path.moveTo(cx, cy);
        // Qt angles are specified in 1/16th of a degree
        path.arcTo(rect, slice.startAngle, slice.spanAngle);
        path.closeSubpath();

        painter.fillPath(path, QColor(slice.color));
    }

    // Cover center with background color circle to form the "donut hole"
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor("#060a13")); // Deep midnight blue matching QMainWindow
    painter.drawEllipse(QPointF(cx, cy), innerRadius, innerRadius);

    // Draw central stats text
    painter.setPen(QColor("#ffffff"));
    
    // Label "STORAGE CONSUMED"
    QFont titleFont = painter.font();
    titleFont.setPointSize(8);
    titleFont.setBold(true);
    painter.setFont(titleFont);
    painter.setPen(QColor("#64748b"));
    QRectF titleRect(cx - innerRadius, cy - 25, innerRadius * 2, 15);
    painter.drawText(titleRect, Qt::AlignCenter, "TOTAL FOOTPRINT");

    // Bytes value
    QFont valFont = painter.font();
    valFont.setPointSize(14);
    valFont.setBold(true);
    painter.setFont(valFont);
    painter.setPen(QColor("#ffffff"));
    
    // Formatted byte string helper
    double bytes = m_totalBytes;
    QStringList units = {"B", "KB", "MB", "GB", "TB"};
    int u = 0;
    while (bytes >= 1024.0 && u < units.size() - 1) {
        bytes /= 1024.0;
        u++;
    }
    QString byteStr = QString::number(bytes, 'f', u > 0 ? 1 : 0) + " " + units[u];

    QRectF valRect(cx - innerRadius, cy - 5, innerRadius * 2, 25);
    painter.drawText(valRect, Qt::AlignCenter, byteStr);

    // Show hovered category name under the footprint
    if (m_hoveredIndex != -1 && m_hoveredIndex < m_slices.size()) {
        QFont hoverFont = painter.font();
        hoverFont.setPointSize(8);
        hoverFont.setBold(false);
        painter.setFont(hoverFont);
        painter.setPen(QColor(m_slices[m_hoveredIndex].color));

        QRectF hoverRect(cx - innerRadius, cy + 20, innerRadius * 2, 15);
        painter.drawText(hoverRect, Qt::AlignCenter, m_slices[m_hoveredIndex].name);
    }
}
