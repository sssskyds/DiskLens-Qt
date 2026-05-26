#include "LargeFilesView.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMenu>
#include <QMessageBox>
#include <QDateTime>
#include <QDebug>

LargeFilesView::LargeFilesView(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void LargeFilesView::setupUI() {
    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(15);

    // Header Panel
    auto headerLayout = new QHBoxLayout();
    auto titleLabel = new QLabel("Large File Finder", this);
    titleLabel->setStyleSheet("font-size: 24px; font-weight: bold; color: #ffffff;");
    headerLayout->addWidget(titleLabel);

    auto limitLayout = new QHBoxLayout();
    auto limitLabel = new QLabel("Display Limit:", this);
    limitLabel->setStyleSheet("color: #94a3b8;");
    m_limitCombo = new QComboBox(this);
    m_limitCombo->addItem("Top 50", 50);
    m_limitCombo->addItem("Top 100", 100);
    m_limitCombo->addItem("Top 250", 250);
    m_limitCombo->addItem("Top 500", 500);
    m_limitCombo->setCurrentIndex(1); // Top 100 default
    connect(m_limitCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &LargeFilesView::onLimitChanged);

    limitLayout->addWidget(limitLabel);
    limitLayout->addWidget(m_limitCombo);
    headerLayout->addLayout(limitLayout);

    mainLayout->addLayout(headerLayout);

    // Files Table Widget
    m_tableWidget = new QTableWidget(this);
    m_tableWidget->setColumnCount(5);
    m_tableWidget->setHorizontalHeaderLabels({"File Name", "Category", "Size", "Date Modified", "Path"});
    m_tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    m_tableWidget->horizontalHeader()->setStretchLastSection(true);
    m_tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tableWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    
    // Set custom stylesheet just in case QSS didn't load headers correctly
    m_tableWidget->setStyleSheet("QTableWidget { gridline-color: rgba(255,255,255,0.03); }");

    connect(m_tableWidget, &QTableWidget::customContextMenuRequested, this, &LargeFilesView::onCustomContextMenu);
    connect(m_tableWidget, &QTableWidget::itemSelectionChanged, this, &LargeFilesView::onTableSelectionChanged);

    mainLayout->addWidget(m_tableWidget, 1);

    // Selected Stats Action Row
    auto controlCard = new QFrame(this);
    controlCard->setProperty("class", "card");
    controlCard->setStyleSheet("QFrame { background-color: rgba(255,255,255,0.02); border: 1px solid rgba(255,255,255,0.06); border-radius: 8px; }");

    auto cardLayout = new QHBoxLayout(controlCard);
    cardLayout->setContentsMargins(16, 12, 16, 12);

    m_selectedDetailsLabel = new QLabel("Select a file from the table above to perform operations...", this);
    m_selectedDetailsLabel->setStyleSheet("color: #94a3b8; font-size: 13px;");
    cardLayout->addWidget(m_selectedDetailsLabel, 1);

    m_revealBtn = new QPushButton("Reveal", this);
    m_revealBtn->setProperty("class", "secondary-btn");
    m_revealBtn->setEnabled(false);
    connect(m_revealBtn, &QPushButton::clicked, this, &LargeFilesView::onRevealClicked);

    m_deleteBtn = new QPushButton("Move to Trash", this);
    m_deleteBtn->setProperty("class", "danger-btn");
    m_deleteBtn->setEnabled(false);
    connect(m_deleteBtn, &QPushButton::clicked, this, &LargeFilesView::onDeleteClicked);

    cardLayout->addWidget(m_revealBtn);
    cardLayout->addWidget(m_deleteBtn);

    mainLayout->addWidget(controlCard);
}

void LargeFilesView::setScanResult(std::shared_ptr<FileSystemNode> root) {
    m_rootNode = root;
    populateTable();
}

void LargeFilesView::clear() {
    m_rootNode.reset();
    m_currentFiles.clear();
    m_tableWidget->setRowCount(0);
    m_revealBtn->setEnabled(false);
    m_deleteBtn->setEnabled(false);
    m_selectedDetailsLabel->setText("Select a file from the table above to perform operations...");
}

void LargeFilesView::onLimitChanged(int index) {
    Q_UNUSED(index);
    populateTable();
}

void LargeFilesView::populateTable() {
    m_tableWidget->setRowCount(0);
    if (!m_rootNode) return;

    int limit = m_limitCombo->currentData().toInt();
    m_currentFiles = m_largeFileService.findLargeFiles(m_rootNode, limit);

    m_tableWidget->setRowCount(m_currentFiles.size());

    for (int i = 0; i < m_currentFiles.size(); ++i) {
        const auto& file = m_currentFiles[i];

        auto nameItem = new QTableWidgetItem(file.name);
        auto catItem = new QTableWidgetItem(file.category);
        auto sizeItem = new QTableWidgetItem(formatBytes(file.size));
        
        // Right align sizes
        sizeItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);

        auto dateItem = new QTableWidgetItem(file.lastModified.toString("yyyy-MM-dd hh:mm:ss"));
        auto pathItem = new QTableWidgetItem(file.path);

        // Muted grey for full paths
        pathItem->setForeground(QColor("#64748b"));

        m_tableWidget->setItem(i, 0, nameItem);
        m_tableWidget->setItem(i, 1, catItem);
        m_tableWidget->setItem(i, 2, sizeItem);
        m_tableWidget->setItem(i, 3, dateItem);
        m_tableWidget->setItem(i, 4, pathItem);
    }

    // Set proportional header sizing
    m_tableWidget->horizontalHeader()->resizeSection(0, 200);
    m_tableWidget->horizontalHeader()->resizeSection(1, 100);
    m_tableWidget->horizontalHeader()->resizeSection(2, 100);
    m_tableWidget->horizontalHeader()->resizeSection(3, 140);
}

void LargeFilesView::onTableSelectionChanged() {
    int row = m_tableWidget->currentRow();
    if (row >= 0 && row < m_currentFiles.size()) {
        const auto& file = m_currentFiles[row];
        m_selectedDetailsLabel->setText(QString("Selected: %1 (%2)").arg(file.name, formatBytes(file.size)));
        m_revealBtn->setEnabled(true);
        m_deleteBtn->setEnabled(true);
    } else {
        m_revealBtn->setEnabled(false);
        m_deleteBtn->setEnabled(false);
        m_selectedDetailsLabel->setText("Select a file from the table above to perform operations...");
    }
}

void LargeFilesView::onRevealClicked() {
    int row = m_tableWidget->currentRow();
    if (row >= 0 && row < m_currentFiles.size()) {
        m_fileOpService.showInExplorer(m_currentFiles[row].path);
    }
}

void LargeFilesView::onDeleteClicked() {
    int row = m_tableWidget->currentRow();
    if (row < 0 || row >= m_currentFiles.size()) return;

    const auto& file = m_currentFiles[row];

    auto reply = QMessageBox::question(this, "Move to Recycle Bin?", 
                                       QString("Are you sure you want to move this file to the Recycle Bin?\n\n%1").arg(file.name),
                                       QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        bool ok = m_fileOpService.moveToTrash(file.path);
        if (ok) {
            QMessageBox::information(this, "Success", "File moved to Recycle Bin successfully.");
            populateTable(); // Reload table
            onTableSelectionChanged(); // Reset details
        } else {
            QMessageBox::critical(this, "Error", "Failed to delete file. It might be in use or write protected.");
        }
    }
}

void LargeFilesView::onCustomContextMenu(const QPoint& pos) {
    int row = m_tableWidget->currentRow();
    if (row < 0 || row >= m_currentFiles.size()) return;

    const auto& file = m_currentFiles[row];

    QMenu menu(this);
    auto openAct = menu.addAction("Open File");
    auto revealAct = menu.addAction("Reveal in File Explorer");
    menu.addSeparator();
    auto trashAct = menu.addAction("Move to Recycle Bin");
    auto deleteAct = menu.addAction("Delete Permanently");

    QAction* selected = menu.exec(m_tableWidget->viewport()->mapToGlobal(pos));
    if (!selected) return;

    if (selected == openAct) {
        m_fileOpService.openFile(file.path);
    } else if (selected == revealAct) {
        m_fileOpService.showInExplorer(file.path);
    } else if (selected == trashAct) {
        onDeleteClicked();
    } else if (selected == deleteAct) {
        auto reply = QMessageBox::warning(this, "PERMANENT DELETE?", 
                                           QString("WARNING: This will permanently delete the file from your computer! This action CANNOT be undone!\n\nDelete %1?").arg(file.name),
                                           QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::Yes) {
            bool ok = m_fileOpService.deletePermanently(file.path);
            if (ok) {
                QMessageBox::information(this, "Success", "File permanently deleted.");
                populateTable();
                onTableSelectionChanged();
            } else {
                QMessageBox::critical(this, "Error", "Failed to delete file.");
            }
        }
    }
}

QString LargeFilesView::formatBytes(qint64 bytes) {
    double size = bytes;
    QStringList units = {"B", "KB", "MB", "GB", "TB"};
    int u = 0;
    while (size >= 1024.0 && u < units.size() - 1) {
        size /= 1024.0;
        u++;
    }
    return QString::number(size, 'f', u > 0 ? 1 : 0) + " " + units[u];
}
