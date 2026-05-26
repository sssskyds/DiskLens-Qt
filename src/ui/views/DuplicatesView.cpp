#include "DuplicatesView.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QDebug>
#include <QFileInfo>

DuplicatesView::DuplicatesView(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void DuplicatesView::setupUI() {
    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(15);

    // Header Title and Scan Controls
    auto headerLayout = new QHBoxLayout();
    auto titleLabel = new QLabel("Duplicate File Finder", this);
    titleLabel->setStyleSheet("font-size: 24px; font-weight: bold; color: #ffffff;");
    headerLayout->addWidget(titleLabel);

    m_scanBtn = new QPushButton("Scan for Duplicates", this);
    m_scanBtn->setProperty("class", "primary-btn");
    m_scanBtn->setEnabled(false);
    connect(m_scanBtn, &QPushButton::clicked, this, &DuplicatesView::onSearchStart);
    headerLayout->addWidget(m_scanBtn, 0, Qt::AlignRight);

    mainLayout->addLayout(headerLayout);

    // Progress Bar Indicator (hidden by default)
    m_progressContainer = new QFrame(this);
    m_progressContainer->setStyleSheet("background-color: rgba(124, 58, 237, 0.08); border: 1px solid rgba(124, 58, 237, 0.15); border-radius: 6px;");
    m_progressContainer->hide();
    
    auto progressLayout = new QVBoxLayout(m_progressContainer);
    progressLayout->setContentsMargins(12, 12, 12, 12);
    
    m_progressLabel = new QLabel("Initializing duplicate checking pipeline...", this);
    m_progressLabel->setStyleSheet("color: #e2e8f0; font-weight: bold;");
    progressLayout->addWidget(m_progressLabel);
    
    m_progressBar = new QProgressBar(this);
    progressLayout->addWidget(m_progressBar);
    
    mainLayout->addWidget(m_progressContainer);

    // Tree Widget representing groups
    m_treeWidget = new QTreeWidget(this);
    m_treeWidget->setColumnCount(3);
    m_treeWidget->setHeaderLabels({"File Path / Checksum Group", "Size", "Duplicate Status"});
    m_treeWidget->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_treeWidget->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_treeWidget->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_treeWidget->setStyleSheet("QTreeWidget::item { padding: 4px; }");
    
    connect(m_treeWidget, &QTreeWidget::itemChanged, this, &DuplicatesView::onTreeItemChanged);
    mainLayout->addWidget(m_treeWidget, 1);

    // Wasted Space Stats and Cleanup Actions Card
    auto controlCard = new QFrame(this);
    controlCard->setProperty("class", "card");
    controlCard->setStyleSheet("QFrame { background-color: rgba(255,255,255,0.02); border: 1px solid rgba(255,255,255,0.06); border-radius: 8px; }");

    auto cardLayout = new QHBoxLayout(controlCard);
    cardLayout->setContentsMargins(16, 12, 16, 12);

    auto statsLayout = new QVBoxLayout();
    auto titleStats = new QLabel("TOTAL RECLAIMABLE SPACE", this);
    titleStats->setProperty("class", "muted-text");
    m_wastedSpaceLabel = new QLabel("0 B (0 groups)", this);
    m_wastedSpaceLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #f85149;"); // Wasted Red
    statsLayout->addWidget(titleStats);
    statsLayout->addWidget(m_wastedSpaceLabel);

    cardLayout->addLayout(statsLayout, 1);

    m_selectAllBtn = new QPushButton("Select All Duplicates", this);
    m_selectAllBtn->setProperty("class", "secondary-btn");
    m_selectAllBtn->setEnabled(false);
    connect(m_selectAllBtn, &QPushButton::clicked, this, &DuplicatesView::onSelectAllDuplicates);

    m_deleteBtn = new QPushButton("Reclaim Checked Space", this);
    m_deleteBtn->setProperty("class", "danger-btn");
    m_deleteBtn->setEnabled(false);
    connect(m_deleteBtn, &QPushButton::clicked, this, &DuplicatesView::onDeleteSelected);

    cardLayout->addWidget(m_selectAllBtn);
    cardLayout->addWidget(m_deleteBtn);

    mainLayout->addWidget(controlCard);

    // Service connections
    connect(&m_duplicateService, &DuplicateService::progressReported, this, &DuplicatesView::onSearchProgress);
    connect(&m_duplicateService, &DuplicateService::searchCompleted, this, &DuplicatesView::onSearchCompleted);
}

void DuplicatesView::setScanResult(std::shared_ptr<FileSystemNode> root) {
    m_rootNode = root;
    m_scanBtn->setEnabled(root != nullptr);
    clear();
}

void DuplicatesView::clear() {
    m_duplicateGroups.clear();
    m_treeWidget->clear();
    m_wastedSpaceLabel->setText("0 B (0 groups)");
    m_selectAllBtn->setEnabled(false);
    m_deleteBtn->setEnabled(false);
    m_progressContainer->hide();
}

void DuplicatesView::onSearchStart() {
    if (!m_rootNode) return;

    m_scanBtn->setEnabled(false);
    m_progressContainer->show();
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressLabel->setText("Crawl indexing files...");

    m_duplicateService.startSearch(m_rootNode);
}

void DuplicatesView::onSearchProgress(const QString& currentFile, int processedCount, int totalCount) {
    m_progressBar->setMaximum(totalCount);
    m_progressBar->setValue(processedCount);

    // Shorten path for display
    QFileInfo info(currentFile);
    m_progressLabel->setText(QString("[%1/%2] Hashing: %3").arg(processedCount).arg(totalCount).arg(info.fileName()));
}

void DuplicatesView::onSearchCompleted(bool success) {
    m_progressContainer->hide();
    m_scanBtn->setEnabled(true);

    if (!success) {
        QMessageBox::warning(this, "Cancelled", "Duplicates search cancelled or failed.");
        return;
    }

    m_duplicateGroups = m_duplicateService.getDuplicates();
    populateTree();
}

void DuplicatesView::populateTree() {
    // Disable signals temporarily to avoid change loops during insert
    m_treeWidget->blockSignals(true);
    m_treeWidget->clear();

    qint64 wasted = m_duplicateService.getWastedSpace();
    m_wastedSpaceLabel->setText(QString("%1 (%2 duplicate sets)").arg(formatBytes(wasted)).arg(m_duplicateGroups.size()));

    for (const auto& group : m_duplicateGroups) {
        // Parent Group Item
        auto parentItem = new QTreeWidgetItem(m_treeWidget);
        parentItem->setText(0, QString("Group MD5 Hash [%1]").arg(group.checksum.left(12)));
        parentItem->setText(1, formatBytes(group.fileSize));
        parentItem->setText(2, QString("%1 copies").arg(group.filePaths.size()));
        
        parentItem->setFont(0, QFont("Segoe UI", 10, QFont::Bold));
        parentItem->setForeground(0, QColor("#e2e8f0"));

        bool isFirst = true;
        for (const QString& path : group.filePaths) {
            auto childItem = new QTreeWidgetItem(parentItem);
            childItem->setText(0, path);
            childItem->setText(1, formatBytes(group.fileSize));
            
            childItem->setData(0, Qt::UserRole, path); // Keep path in user data

            if (isFirst) {
                // Keep the first item as the ORIGINAL
                childItem->setText(2, "ORIGINAL");
                childItem->setForeground(2, QColor("#00d4ff")); // Cyan original
                childItem->setFont(2, QFont("Segoe UI", 9, QFont::Bold));
                childItem->setFlags(childItem->flags() & ~Qt::ItemIsUserCheckable); // Can't select original
                isFirst = false;
            } else {
                // Duplicate copies are checkable
                childItem->setText(2, "DUPLICATE COPY");
                childItem->setForeground(2, QColor("#ef4444")); // Red duplicate
                childItem->setCheckState(0, Qt::Unchecked);
                childItem->setFlags(childItem->flags() | Qt::ItemIsUserCheckable);
            }
        }
        
        parentItem->setExpanded(true); // Auto expand
    }

    m_treeWidget->blockSignals(false);

    m_selectAllBtn->setEnabled(!m_duplicateGroups.isEmpty());
    m_deleteBtn->setEnabled(false); // Enable only when checkboxes are checked
}

void DuplicatesView::onSelectAllDuplicates() {
    m_treeWidget->blockSignals(true);

    for (int i = 0; i < m_treeWidget->topLevelItemCount(); ++i) {
        QTreeWidgetItem* parent = m_treeWidget->topLevelItem(i);
        for (int c = 0; c < parent->childCount(); ++c) {
            QTreeWidgetItem* child = parent->child(c);
            if (child->text(2) == "DUPLICATE COPY") {
                child->setCheckState(0, Qt::Checked);
            }
        }
    }

    m_treeWidget->blockSignals(false);
    m_deleteBtn->setEnabled(true);
}

void DuplicatesView::onTreeItemChanged(QTreeWidgetItem* item, int column) {
    Q_UNUSED(item);
    Q_UNUSED(column);

    // Check if any duplicate checkbox is ticked
    bool anyChecked = false;
    for (int i = 0; i < m_treeWidget->topLevelItemCount(); ++i) {
        QTreeWidgetItem* parent = m_treeWidget->topLevelItem(i);
        for (int c = 0; c < parent->childCount(); ++c) {
            QTreeWidgetItem* child = parent->child(c);
            if (child->checkState(0) == Qt::Checked) {
                anyChecked = true;
                break;
            }
        }
        if (anyChecked) break;
    }

    m_deleteBtn->setEnabled(anyChecked);
}

void DuplicatesView::onDeleteSelected() {
    QVector<QString> filesToDelete;
    qint64 totalSpaceSaved = 0;

    // Gather selected duplicates paths
    for (int i = 0; i < m_treeWidget->topLevelItemCount(); ++i) {
        QTreeWidgetItem* parent = m_treeWidget->topLevelItem(i);
        qint64 size = 0;
        
        // Find group item size
        QString sizeStr = parent->text(1);
        // Better is to find corresponding duplicate struct or parse back,
        // but we saved file size or we can parse it.
        // Even simpler: match parent item in our struct array using checksum or child paths!
        // But wait! We saved the original file path and we can just read the child item size!
        for (int c = 0; c < parent->childCount(); ++c) {
            QTreeWidgetItem* child = parent->child(c);
            if (child->checkState(0) == Qt::Checked) {
                filesToDelete.push_back(child->data(0, Qt::UserRole).toString());
                // Group size is the same, so parse from struct or item text
                // Let's matching the path back to duplicate struct to get the exact size
                for (const auto& gp : m_duplicateGroups) {
                    if (gp.filePaths.contains(child->data(0, Qt::UserRole).toString())) {
                        totalSpaceSaved += gp.fileSize;
                        break;
                    }
                }
            }
        }
    }

    if (filesToDelete.isEmpty()) return;

    auto reply = QMessageBox::warning(this, "Clean up duplicates?",
                                       QString("Are you sure you want to move %1 duplicate copies to the Recycle Bin?\n\nThis will reclaim approximately %2.")
                                           .arg(filesToDelete.size())
                                           .arg(formatBytes(totalSpaceSaved)),
                                       QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        int successCount = 0;
        
        for (const QString& path : filesToDelete) {
            bool ok = m_fileOpService.moveToTrash(path);
            if (ok) {
                successCount++;
            }
        }

        QMessageBox::information(this, "Cleanup Finished", 
                                 QString("Successfully cleaned up %1 of %2 selected duplicate files.\n\nReclaimed: %3!")
                                     .arg(successCount)
                                     .arg(filesToDelete.size())
                                     .arg(formatBytes(totalSpaceSaved)));

        // Automatically run search again to refresh the duplicate groups
        onSearchStart();
    }
}

QString DuplicatesView::formatBytes(qint64 bytes) {
    double size = bytes;
    QStringList units = {"B", "KB", "MB", "GB", "TB"};
    int u = 0;
    while (size >= 1024.0 && u < units.size() - 1) {
        size /= 1024.0;
        u++;
    }
    return QString::number(size, 'f', u > 0 ? 1 : 0) + " " + units[u];
}
