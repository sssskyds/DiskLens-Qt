#include "DashboardView.h"
#include "ui/widgets/RadialGauge.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QScrollArea>

DashboardView::DashboardView(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
    refreshVolumes();
}

void DashboardView::setupUI() {
    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(20);

    // Title Section
    auto headerLayout = new QHBoxLayout();
    auto titleLabel = new QLabel("System Overview Dashboard", this);
    titleLabel->setObjectName("viewTitle");
    titleLabel->setProperty("class", "heading-1");
    titleLabel->setStyleSheet("font-size: 24px; font-weight: bold; color: #ffffff;");
    headerLayout->addWidget(titleLabel);

    auto refreshBtn = new QPushButton("Refresh Drives", this);
    refreshBtn->setProperty("class", "secondary-btn");
    connect(refreshBtn, &QPushButton::clicked, this, &DashboardView::refreshVolumes);
    headerLayout->addWidget(refreshBtn, 0, Qt::AlignRight);

    mainLayout->addLayout(headerLayout);

    // Aggregate Summary Panel (Glass-Card)
    auto summaryPanel = new QFrame(this);
    summaryPanel->setObjectName("summaryPanel");
    summaryPanel->setFrameShape(QFrame::StyledPanel);
    summaryPanel->setMinimumHeight(100);

    auto summaryLayout = new QHBoxLayout(summaryPanel);
    summaryLayout->setContentsMargins(24, 16, 24, 16);

    auto totalCapCol = new QVBoxLayout();
    auto capTitle = new QLabel("TOTAL INSTALLED FOOTPRINT", this);
    capTitle->setProperty("class", "muted-text");
    m_totalCapacityLabel = new QLabel("0.0 GB", this);
    m_totalCapacityLabel->setStyleSheet("font-size: 20px; font-weight: bold; color: #ffffff;");
    totalCapCol->addWidget(capTitle);
    totalCapCol->addWidget(m_totalCapacityLabel);
    summaryLayout->addLayout(totalCapCol);

    // Divider line
    auto divider1 = new QFrame(this);
    divider1->setFrameShape(QFrame::VLine);
    divider1->setFrameShadow(QFrame::Sunken);
    divider1->setStyleSheet("background-color: rgba(255,255,255,0.06);");
    summaryLayout->addWidget(divider1);

    auto totalUsedCol = new QVBoxLayout();
    auto usedTitle = new QLabel("AGGREGATE SPACE CONSUMED", this);
    usedTitle->setProperty("class", "muted-text");
    m_totalUsedLabel = new QLabel("0.0 GB", this);
    m_totalUsedLabel->setStyleSheet("font-size: 20px; font-weight: bold; color: #7c3aed;"); // Purple brand accent
    totalUsedCol->addWidget(usedTitle);
    totalUsedCol->addWidget(m_totalUsedLabel);
    summaryLayout->addLayout(totalUsedCol);

    // Divider line
    auto divider2 = new QFrame(this);
    divider2->setFrameShape(QFrame::VLine);
    divider2->setFrameShadow(QFrame::Sunken);
    divider2->setStyleSheet("background-color: rgba(255,255,255,0.06);");
    summaryLayout->addWidget(divider2);

    auto totalFreeCol = new QVBoxLayout();
    auto freeTitle = new QLabel("SYSTEM FREE POOL", this);
    freeTitle->setProperty("class", "muted-text");
    m_totalFreeLabel = new QLabel("0.0 GB", this);
    m_totalFreeLabel->setStyleSheet("font-size: 20px; font-weight: bold; color: #3fb950;"); // Success Green
    totalFreeCol->addWidget(freeTitle);
    totalFreeCol->addWidget(m_totalFreeLabel);
    summaryLayout->addLayout(totalFreeCol);

    mainLayout->addWidget(summaryPanel);

    // Scroll area for cards
    auto scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet("background-color: transparent;");

    auto scrollContainer = new QWidget(scrollArea);
    scrollContainer->setStyleSheet("background-color: transparent;");
    m_cardsLayout = new QGridLayout(scrollContainer);
    m_cardsLayout->setSpacing(20);
    m_cardsLayout->setContentsMargins(0, 0, 0, 0);

    scrollArea->setWidget(scrollContainer);
    mainLayout->addWidget(scrollArea);
}

void DashboardView::refreshVolumes() {
    // Clear old cards layout
    QLayoutItem* child;
    while ((child = m_cardsLayout->takeAt(0)) != nullptr) {
        delete child->widget();
        delete child;
    }

    auto volumes = m_volumeService.getMountedVolumes();

    qint64 totalCapacity = 0;
    qint64 totalFree = 0;
    qint64 totalUsed = 0;

    int row = 0;
    int col = 0;
    const int maxCols = 3;

    for (const auto& vol : volumes) {
        totalCapacity += vol.totalBytes;
        totalFree += vol.freeBytes;
        totalUsed += vol.usedBytes;

        // Create Drive Card Frame
        auto card = new QFrame(this);
        card->setProperty("class", "card");
        card->setStyleSheet(
            "QFrame.card {"
            "  background-color: rgba(255, 255, 255, 0.02);"
            "  border: 1px solid rgba(255, 255, 255, 0.06);"
            "  border-radius: 12px;"
            "}"
            "QFrame.card:hover {"
            "  border-color: rgba(0, 212, 255, 0.25);"
            "  background-color: rgba(255, 255, 255, 0.03);"
            "}"
        );

        auto cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(16, 16, 16, 16);
        cardLayout->setSpacing(12);

        // Drive Metadata Header
        auto labelLayout = new QHBoxLayout();
        auto label = new QLabel(QString("%1 (%2)").arg(vol.label, vol.path), this);
        label->setStyleSheet("font-size: 15px; font-weight: bold; color: #ffffff;");
        labelLayout->addWidget(label);

        auto badge = new QLabel(vol.fileSystem + " | " + vol.type, this);
        badge->setStyleSheet("background-color: rgba(255,255,255,0.05); padding: 2px 6px; border-radius: 4px; color: #94a3b8; font-size: 10px;");
        labelLayout->addWidget(badge, 0, Qt::AlignRight);

        cardLayout->addLayout(labelLayout);

        // Radial Gauge
        auto gauge = new RadialGauge(this);
        gauge->setValue(vol.saturationPercent);
        gauge->setLabel(vol.path);
        
        QString usedStr = formatBytes(vol.usedBytes);
        QString totalStr = formatBytes(vol.totalBytes);
        gauge->setSubLabel(QString("%1 / %2").arg(usedStr, totalStr));
        
        cardLayout->addWidget(gauge, 0, Qt::AlignCenter);

        // Scan Action button
        auto scanBtn = new QPushButton("Scan Storage Map", this);
        scanBtn->setProperty("class", "primary-btn");
        connect(scanBtn, &QPushButton::clicked, this, [this, vol]() {
            emit driveSelectedForScan(vol.path);
        });
        cardLayout->addWidget(scanBtn);

        m_cardsLayout->addWidget(card, row, col);

        col++;
        if (col >= maxCols) {
            col = 0;
            row++;
        }
    }

    m_totalCapacityLabel->setText(formatBytes(totalCapacity));
    m_totalUsedLabel->setText(formatBytes(totalUsed));
    m_totalFreeLabel->setText(formatBytes(totalFree));
}

QString DashboardView::formatBytes(qint64 bytes) {
    double size = bytes;
    QStringList units = {"B", "KB", "MB", "GB", "TB"};
    int u = 0;
    while (size >= 1024.0 && u < units.size() - 1) {
        size /= 1024.0;
        u++;
    }
    return QString::number(size, 'f', u > 0 ? 1 : 0) + " " + units[u];
}
