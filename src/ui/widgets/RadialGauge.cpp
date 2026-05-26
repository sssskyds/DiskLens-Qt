#include "RadialGauge.h"
#include <QPainter>
#include <QPen>
#include <QColor>
#include <cmath>

RadialGauge::RadialGauge(QWidget* parent)
    : QWidget(parent)
{
    setMinimumSize(120, 120);
}

void RadialGauge::setValue(double val) {
    m_value = std::max(0.0, std::min(100.0, val));
    update();
}

void RadialGauge::setLabel(const QString& label) {
    m_label = label;
    update();
}

void RadialGauge::setSubLabel(const QString& subLabel) {
    m_subLabel = subLabel;
    update();
}

void RadialGauge::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    int size = std::min(width(), height()) - 10;
    int x = (width() - size) / 2;
    int y = (height() - size) / 2;

    QRectF rect(x + 10, y + 10, size - 20, size - 20);

    // Track arc angles (start at -225 degrees, length of 270 degrees)
    int startAngle = -225 * 16;
    int spanAngle = -270 * 16;

    // 1. Draw Background Track Ring
    QPen trackPen;
    trackPen.setColor(QColor(255, 255, 255, 15)); // Semi-transparent white
    trackPen.setWidth(10);
    trackPen.setCapStyle(Qt::RoundCap);
    painter.setPen(trackPen);
    painter.drawArc(rect, startAngle, spanAngle);

    // 2. Select Ring Color based on Saturation Level
    QColor ringColor;
    if (m_value < 75.0) {
        ringColor = QColor("#00d4ff"); // Cyan
    } else if (m_value < 90.0) {
        ringColor = QColor("#d29922"); // Orange
    } else {
        ringColor = QColor("#f85149"); // Red
    }

    // 3. Draw Active Filled Value Arc
    int valueSpanAngle = -static_cast<int>(m_value * 2.7 * 16.0);
    QPen valuePen;
    valuePen.setColor(ringColor);
    valuePen.setWidth(10);
    valuePen.setCapStyle(Qt::RoundCap);
    painter.setPen(valuePen);
    painter.drawArc(rect, startAngle, valueSpanAngle);

    // 4. Draw Central Saturation Text
    painter.setPen(QColor("#ffffff"));
    QFont percentFont = painter.font();
    percentFont.setPointSize(18);
    percentFont.setBold(true);
    painter.setFont(percentFont);
    
    QString pctStr = QString::number(m_value, 'f', 1) + "%";
    QRectF textRect(x, y + size/2 - 20, size, 25);
    painter.drawText(textRect, Qt::AlignCenter, pctStr);

    // 5. Draw Drive Label at bottom of ring
    QFont labelFont = painter.font();
    labelFont.setPointSize(10);
    labelFont.setBold(false);
    painter.setFont(labelFont);
    painter.setPen(QColor("#94a3b8"));
    
    QRectF labelRect(x, y + size/2 + 10, size, 20);
    painter.drawText(labelRect, Qt::AlignCenter, m_label);

    // 6. Draw SubLabel (Bytes remaining)
    if (!m_subLabel.isEmpty()) {
        QFont subFont = painter.font();
        subFont.setPointSize(8);
        painter.setFont(subFont);
        painter.setPen(QColor("#64748b"));
        
        QRectF subRect(x, y + size/2 + 28, size, 15);
        painter.drawText(subRect, Qt::AlignCenter, m_subLabel);
    }
}
