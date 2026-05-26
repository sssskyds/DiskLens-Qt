#include "FileTypesView.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QHeaderView>
#include <QDebug>

FileTypesView::FileTypesView(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void FileTypesView::setupUI() {
    auto mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(20);

    // Left Panel: Chart & Hover Card
    auto leftLayout = new QVBoxLayout();
    leftLayout->setSpacing(15);

    auto titleLabel = new QLabel("File Type Distribution", this);
    titleLabel->setStyleSheet("font-size: 24px; font-weight: bold; color: #ffffff;");
    leftLayout->addWidget(titleLabel);

    m_donutChart = new DonutChartWidget(this);
    connect(m_donutChart, &DonutChartWidget::categoryHovered, this, &FileTypesView::onCategoryHovered);
    leftLayout->addWidget(m_donutChart, 1);

    m_hoverDetailsLabel = new QLabel("Hover over a chart slice to view details...", this);
    m_hoverDetailsLabel->setStyleSheet("background-color: rgba(255,255,255,0.02); padding: 10px 14px; border-radius: 6px; color: #94a3b8; border: 1px solid rgba(255,255,255,0.04); text-align: center;");
    m_hoverDetailsLabel->setAlignment(Qt::AlignCenter);
    leftLayout->addWidget(m_hoverDetailsLabel);

    mainLayout->addLayout(leftLayout, 2);

    // Right Panel: Split lists
    auto rightLayout = new QVBoxLayout();
    rightLayout->setSpacing(15);

    auto listHeading = new QLabel("Category Footprint Details", this);
    listHeading->setStyleSheet("font-size: 16px; font-weight: bold; color: #ffffff;");
    rightLayout->addWidget(listHeading);

    // Categories Table
    m_categoryTable = new QTableWidget(this);
    m_categoryTable->setColumnCount(4);
    m_categoryTable->setHorizontalHeaderLabels({"Category", "File Count", "Total Size", "%"});
    m_categoryTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_categoryTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_categoryTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_categoryTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    
    connect(m_categoryTable, &QTableWidget::itemSelectionChanged, this, &FileTypesView::onCategorySelected);
    rightLayout->addWidget(m_categoryTable, 3);

    // Sub-extension Details Card
    m_detailsTitleLabel = new QLabel("Extension Breakdown", this);
    m_detailsTitleLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: #f1f5f9;");
    rightLayout->addWidget(m_detailsTitleLabel);

    m_extensionDetailsTable = new QTableWidget(this);
    m_extensionDetailsTable->setColumnCount(3);
    m_extensionDetailsTable->setHorizontalHeaderLabels({"Extension", "File Count", "Cumulative Size"});
    m_extensionDetailsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_extensionDetailsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_extensionDetailsTable->setSelectionMode(QAbstractItemView::NoSelection);
    m_extensionDetailsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    rightLayout->addWidget(m_extensionDetailsTable, 2);

    mainLayout->addLayout(rightLayout, 3);
}

void FileTypesView::setScanResult(std::shared_ptr<FileSystemNode> root) {
    m_rootNode = root;
    populateStats();
}

void FileTypesView::clear() {
    m_rootNode.reset();
    m_currentStats.clear();
    m_donutChart->clear();
    m_categoryTable->setRowCount(0);
    m_extensionDetailsTable->setRowCount(0);
    m_hoverDetailsLabel->setText("Hover over a chart slice to view details...");
    m_detailsTitleLabel->setText("Extension Breakdown");
}

void FileTypesView::populateStats() {
    m_categoryTable->setRowCount(0);
    m_extensionDetailsTable->setRowCount(0);
    if (!m_rootNode) return;

    m_currentStats = m_fileTypeService.analyzeFileTypes(m_rootNode);
    m_donutChart->setData(m_currentStats);

    m_categoryTable->setRowCount(m_currentStats.size());

    for (int i = 0; i < m_currentStats.size(); ++i) {
        const auto& cat = m_currentStats[i];

        // Format name with color marker indicator
        auto nameItem = new QTableWidgetItem(cat.categoryName);
        nameItem->setForeground(QColor(cat.color));
        nameItem->setFont(QFont("Segoe UI", 10, QFont::Bold));

        auto countItem = new QTableWidgetItem(QString::number(cat.fileCount));
        auto sizeItem = new QTableWidgetItem(formatBytes(cat.totalSize));
        sizeItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);

        auto pctItem = new QTableWidgetItem(QString::number(cat.percentage, 'f', 1) + "%");
        pctItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);

        m_categoryTable->setItem(i, 0, nameItem);
        m_categoryTable->setItem(i, 1, countItem);
        m_categoryTable->setItem(i, 2, sizeItem);
        m_categoryTable->setItem(i, 3, pctItem);
    }

    if (m_categoryTable->rowCount() > 0) {
        m_categoryTable->selectRow(0); // Trigger default selection
    }
}

void FileTypesView::onCategoryHovered(const QString& name, qint64 size, double percentage) {
    m_hoverDetailsLabel->setText(QString("<b>%1</b>: %2 (%3% of footprint)")
                                     .arg(name, formatBytes(size), QString::number(percentage, 'f', 1)));
}

void FileTypesView::onCategorySelected() {
    int row = m_categoryTable->currentRow();
    populateExtensionsForCategory(row);
}

void FileTypesView::populateExtensionsForCategory(int row) {
    m_extensionDetailsTable->setRowCount(0);
    if (row < 0 || row >= m_currentStats.size()) return;

    const auto& cat = m_currentStats[row];
    m_detailsTitleLabel->setText(QString("Extension Breakdown for %1").arg(cat.categoryName));

    const auto& exts = cat.extensionDetails;
    m_extensionDetailsTable->setRowCount(exts.size());

    for (int i = 0; i < exts.size(); ++i) {
        const auto& ext = exts[i];

        auto extItem = new QTableWidgetItem(ext.ext);
        extItem->setFont(QFont("Segoe UI", 10, QFont::DemiBold));

        auto countItem = new QTableWidgetItem(QString::number(ext.count));
        auto sizeItem = new QTableWidgetItem(formatBytes(ext.size));
        sizeItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);

        m_extensionDetailsTable->setItem(i, 0, extItem);
        m_extensionDetailsTable->setItem(i, 1, countItem);
        m_extensionDetailsTable->setItem(i, 2, sizeItem);
    }
}

QString FileTypesView::formatBytes(qint64 bytes) {
    double size = bytes;
    QStringList units = {"B", "KB", "MB", "GB", "TB"};
    int u = 0;
    while (size >= 1024.0 && u < units.size() - 1) {
        size /= 1024.0;
        u++;
    }
    return QString::number(size, 'f', u > 0 ? 1 : 0) + " " + units[u];
}
