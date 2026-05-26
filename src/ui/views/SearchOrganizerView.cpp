#include "SearchOrganizerView.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QHeaderView>
#include <QFileDialog>
#include <QTextStream>
#include <QMessageBox>
#include <QFileInfo>
#include <QDir>
#include <QDebug>

SearchOrganizerView::SearchOrganizerView(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void SearchOrganizerView::setupUI() {
    auto mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(20);

    // Left Column: Advanced Search
    auto leftCol = new QVBoxLayout();
    leftCol->setSpacing(12);

    auto searchTitle = new QLabel("Advanced Storage Search", this);
    searchTitle->setStyleSheet("font-size: 18px; font-weight: bold; color: #ffffff;");
    leftCol->addWidget(searchTitle);

    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText("Enter file name or extension (e.g. invoice, .pdf, *.zip)...");
    m_searchEdit->setEnabled(false);
    connect(m_searchEdit, &QLineEdit::textChanged, this, &SearchOrganizerView::onSearchQueryChanged);
    leftCol->addWidget(m_searchEdit);

    m_searchCountLabel = new QLabel("Results: 0 files matched", this);
    m_searchCountLabel->setStyleSheet("color: #94a3b8; font-size: 11px;");
    leftCol->addWidget(m_searchCountLabel);

    m_searchResultsTable = new QTableWidget(this);
    m_searchResultsTable->setColumnCount(3);
    m_searchResultsTable->setHorizontalHeaderLabels({"File Name", "Size", "Parent Path"});
    m_searchResultsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    m_searchResultsTable->horizontalHeader()->setStretchLastSection(true);
    m_searchResultsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_searchResultsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_searchResultsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    
    connect(m_searchResultsTable, &QTableWidget::cellDoubleClicked, this, &SearchOrganizerView::onSearchTableDoubleClicked);
    leftCol->addWidget(m_searchResultsTable, 1);

    mainLayout->addLayout(leftCol, 3);

    // Right Column: Split into Organizer and Exporter
    auto rightCol = new QVBoxLayout();
    rightCol->setSpacing(20);

    // Section 1: Folder Organizer Card
    auto organizerCard = new QFrame(this);
    organizerCard->setProperty("class", "card");
    organizerCard->setStyleSheet("QFrame { background-color: rgba(255,255,255,0.02); border: 1px solid rgba(255,255,255,0.06); border-radius: 10px; }");
    
    auto orgLayout = new QVBoxLayout(organizerCard);
    orgLayout->setContentsMargins(16, 16, 16, 16);
    orgLayout->setSpacing(10);

    auto orgTitle = new QLabel("Smart Folder Organizer", this);
    orgTitle->setStyleSheet("font-size: 16px; font-weight: bold; color: #ffffff; border: none; background: transparent;");
    orgLayout->addWidget(orgTitle);

    auto orgControls = new QHBoxLayout();
    m_organizerRuleCombo = new QComboBox(this);
    m_organizerRuleCombo->addItem("Sort by Type (Images, Videos, Docs)");
    m_organizerRuleCombo->addItem("Clean Trash Files (*.tmp, *.log)");
    m_organizerRuleCombo->setEnabled(false);
    orgControls->addWidget(m_organizerRuleCombo, 1);

    auto previewBtn = new QPushButton("Preview Actions", this);
    previewBtn->setProperty("class", "secondary-btn");
    connect(previewBtn, &QPushButton::clicked, this, &SearchOrganizerView::onPreviewOrganizer);
    orgControls->addWidget(previewBtn);
    orgLayout->addLayout(orgControls);

    m_organizerPreviewTable = new QTableWidget(this);
    m_organizerPreviewTable->setColumnCount(3);
    m_organizerPreviewTable->setHorizontalHeaderLabels({"Action Type", "Target Path", "Destination / Status"});
    m_organizerPreviewTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_organizerPreviewTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_organizerPreviewTable->setFixedHeight(150);
    orgLayout->addWidget(m_organizerPreviewTable);

    m_organizeApplyBtn = new QPushButton("Apply Clean Operations", this);
    m_organizeApplyBtn->setProperty("class", "primary-btn");
    m_organizeApplyBtn->setEnabled(false);
    connect(m_organizeApplyBtn, &QPushButton::clicked, this, &SearchOrganizerView::onApplyOrganizer);
    orgLayout->addWidget(m_organizeApplyBtn);

    rightCol->addWidget(organizerCard, 3);

    // Section 2: Report Exporter Card
    auto exporterCard = new QFrame(this);
    exporterCard->setProperty("class", "card");
    exporterCard->setStyleSheet("QFrame { background-color: rgba(255,255,255,0.02); border: 1px solid rgba(255,255,255,0.06); border-radius: 10px; }");

    auto expLayout = new QVBoxLayout(exporterCard);
    expLayout->setContentsMargins(16, 16, 16, 16);
    expLayout->setSpacing(12);

    auto expTitle = new QLabel("Report Exporter Panel", this);
    expTitle->setStyleSheet("font-size: 16px; font-weight: bold; color: #ffffff; border: none; background: transparent;");
    expLayout->addWidget(expTitle);

    auto expDesc = new QLabel("Export a fully detailed database report of your scanned directories.", this);
    expDesc->setStyleSheet("color: #94a3b8; font-size: 11px; border: none; background: transparent;");
    expLayout->addWidget(expDesc);

    auto expButtons = new QHBoxLayout();
    auto csvBtn = new QPushButton("Export Flat CSV List", this);
    csvBtn->setProperty("class", "secondary-btn");
    connect(csvBtn, &QPushButton::clicked, this, &SearchOrganizerView::onExportCsv);
    expButtons->addWidget(csvBtn);

    auto mdBtn = new QPushButton("Export Bullet MD Tree", this);
    mdBtn->setProperty("class", "secondary-btn");
    connect(mdBtn, &QPushButton::clicked, this, &SearchOrganizerView::onExportMarkdown);
    expButtons->addWidget(mdBtn);

    expLayout->addLayout(expButtons);
    rightCol->addWidget(exporterCard, 1);

    mainLayout->addLayout(rightCol, 2);
}

void SearchOrganizerView::setScanResult(std::shared_ptr<FileSystemNode> root) {
    m_rootNode = root;
    m_searchEdit->setEnabled(root != nullptr);
    m_organizerRuleCombo->setEnabled(root != nullptr);
    clear();
}

void SearchOrganizerView::clear() {
    m_matchingNodes.clear();
    m_pendingActions.clear();
    m_searchResultsTable->setRowCount(0);
    m_organizerPreviewTable->setRowCount(0);
    m_searchEdit->clear();
    m_searchCountLabel->setText("Results: 0 files matched");
    m_organizeApplyBtn->setEnabled(false);
}

void SearchOrganizerView::onSearchQueryChanged(const QString& query) {
    m_searchResultsTable->setRowCount(0);
    m_matchingNodes.clear();

    if (!m_rootNode || query.trimmed().isEmpty()) {
        m_searchCountLabel->setText("Results: 0 files matched");
        return;
    }

    // Standard recursive search
    recursiveSearch(m_rootNode, query.trimmed());

    int count = std::min(m_matchingNodes.size(), static_cast<qsizetype>(500)); // Caps rendering at 500 entries
    m_searchResultsTable->setRowCount(count);

    for (int i = 0; i < count; ++i) {
        const auto& node = m_matchingNodes[i];
        
        auto nameItem = new QTableWidgetItem(node->getName());
        auto sizeItem = new QTableWidgetItem(formatBytes(node->getSize()));
        sizeItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);

        QFileInfo info(node->getPath());
        auto pathItem = new QTableWidgetItem(info.absolutePath());
        pathItem->setForeground(QColor("#64748b"));

        m_searchResultsTable->setItem(i, 0, nameItem);
        m_searchResultsTable->setItem(i, 1, sizeItem);
        m_searchResultsTable->setItem(i, 2, pathItem);
    }

    m_searchResultsTable->horizontalHeader()->resizeSection(0, 180);
    m_searchResultsTable->horizontalHeader()->resizeSection(1, 80);

    m_searchCountLabel->setText(QString("Results: %1 files matched%2")
                                    .arg(m_matchingNodes.size())
                                    .arg(m_matchingNodes.size() > 500 ? " (showing top 500)" : ""));
}

void SearchOrganizerView::recursiveSearch(std::shared_ptr<FileSystemNode> node, const QString& query) {
    if (!node) return;

    if (!node->isDirectory()) {
        // Simple case-insensitive search
        if (node->getName().contains(query, Qt::CaseInsensitive)) {
            m_matchingNodes.push_back(node);
        }
    } else {
        for (const auto& child : node->getChildren()) {
            recursiveSearch(child, query);
        }
    }
}

void SearchOrganizerView::onSearchTableDoubleClicked(int row, int col) {
    Q_UNUSED(col);
    if (row >= 0 && row < m_matchingNodes.size()) {
        m_fileOpService.openFile(m_matchingNodes[row]->getPath());
    }
}

void SearchOrganizerView::onPreviewOrganizer() {
    m_organizerPreviewTable->setRowCount(0);
    m_pendingActions.clear();

    if (!m_rootNode) return;

    QString ruleType = m_organizerRuleCombo->currentText();
    collectOrganizerActions(m_rootNode, ruleType, m_pendingActions);

    m_organizerPreviewTable->setRowCount(m_pendingActions.size());

    for (int i = 0; i < m_pendingActions.size(); ++i) {
        const auto& act = m_pendingActions[i];
        
        QFileInfo original(act.sourcePath);

        auto actionItem = new QTableWidgetItem(act.ruleApplied);
        actionItem->setForeground(QColor(act.ruleApplied.startsWith("Clean") ? "#ef4444" : "#00d4ff"));

        auto targetItem = new QTableWidgetItem(original.fileName());
        
        auto destItem = new QTableWidgetItem(act.destinationPath);
        destItem->setForeground(QColor("#64748b"));

        m_organizerPreviewTable->setItem(i, 0, actionItem);
        m_organizerPreviewTable->setItem(i, 1, targetItem);
        m_organizerPreviewTable->setItem(i, 2, destItem);
    }

    m_organizeApplyBtn->setEnabled(!m_pendingActions.isEmpty());

    if (m_pendingActions.isEmpty()) {
        QMessageBox::information(this, "Organizer Preview", "No files found matching the organizer rules under this directory structure.");
    }
}

void SearchOrganizerView::collectOrganizerActions(std::shared_ptr<FileSystemNode> node, const QString& ruleType, QVector<OrganizeAction>& actions) {
    if (!node) return;

    if (!node->isDirectory()) {
        QFileInfo info(node->getPath());
        QString ext = node->getExtension();

        if (ruleType.startsWith("Sort by Type")) {
            // Check if file is inside a categorization folder already (e.g. Images inside /Images)
            // to avoid sorting infinitely
            auto parent = node->getParent();
            QString parentName = parent ? parent->getName() : "";
            QString cat = node->getCategory();

            if (cat != "Other" && parentName != cat) {
                OrganizeAction act;
                act.sourcePath = node->getPath();
                // Dest path under base directory + category folder + original file name
                QDir baseDir(m_rootNode->getPath());
                act.destinationPath = baseDir.filePath(cat + "/" + node->getName());
                act.ruleApplied = QString("Sort into %1").arg(cat);
                act.sizeBytes = node->getSize();
                actions.push_back(act);
            }
        } else if (ruleType.startsWith("Clean Trash")) {
            if (ext == ".tmp" || ext == ".log" || ext == ".bak" || node->getName().contains("temp", Qt::CaseInsensitive)) {
                OrganizeAction act;
                act.sourcePath = node->getPath();
                act.destinationPath = "MOVE TO TRASH BIN";
                act.ruleApplied = "Clean Temp / Log file";
                act.sizeBytes = node->getSize();
                actions.push_back(act);
            }
        }
    } else {
        for (const auto& child : node->getChildren()) {
            collectOrganizerActions(child, ruleType, actions);
        }
    }
}

void SearchOrganizerView::onApplyOrganizer() {
    if (m_pendingActions.isEmpty()) return;

    auto reply = QMessageBox::warning(this, "Apply organizing rules?",
                                       QString("Are you sure you want to perform these %1 storage actions?\n\nThis will physically rename folders or move files!").arg(m_pendingActions.size()),
                                       QMessageBox::Yes | QMessageBox::No);

    if (reply != QMessageBox::Yes) return;

    int success = 0;
    qint64 totalBytes = 0;

    for (const auto& act : m_pendingActions) {
        if (act.destinationPath == "MOVE TO TRASH BIN") {
            bool ok = m_fileOpService.moveToTrash(act.sourcePath);
            if (ok) {
                success++;
                totalBytes += act.sizeBytes;
            }
        } else {
            // Separate by type: create folders and move
            QFileInfo destInfo(act.destinationPath);
            QDir destDir = destInfo.absoluteDir();
            if (!destDir.exists()) {
                destDir.mkpath(".");
            }

            bool ok = QFile::rename(act.sourcePath, act.destinationPath);
            if (ok) {
                success++;
                totalBytes += act.sizeBytes;
            }
        }
    }

    QMessageBox::information(this, "Clean Complete", 
                             QString("Successfully executed %1 of %2 storage organizing operations!\n\nOrganized size footprint: %3.")
                                 .arg(success)
                                 .arg(m_pendingActions.size())
                                 .arg(formatBytes(totalBytes)));

    clear();
}

void SearchOrganizerView::onExportCsv() {
    if (!m_rootNode) return;

    QString savePath = QFileDialog::getSaveFileName(this, "Save Flat CSV Report", 
                                                    QDir::homePath() + "/DiskLens_Report.csv", 
                                                    "CSV Files (*.csv)");
    if (!savePath.isEmpty()) {
        exportToCsvFile(savePath);
    }
}

void SearchOrganizerView::onExportMarkdown() {
    if (!m_rootNode) return;

    QString savePath = QFileDialog::getSaveFileName(this, "Save Structured Markdown Report", 
                                                    QDir::homePath() + "/DiskLens_Report.md", 
                                                    "Markdown Files (*.md)");
    if (!savePath.isEmpty()) {
        exportToMarkdownFile(savePath);
    }
}

void SearchOrganizerView::exportToCsvFile(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Export Error", "Could not open file for writing reports.");
        return;
    }

    QTextStream out(&file);
    out << "File Name,File Path,Size (Bytes),Category,Is Directory\n";

    collectAllFilesCsv(m_rootNode, out);

    file.close();
    QMessageBox::information(this, "Export Success", "Successfully exported full database CSV file.");
}

void SearchOrganizerView::collectAllFilesCsv(std::shared_ptr<FileSystemNode> node, QTextStream& stream) {
    if (!node) return;

    // Standard escape CSV commas
    QString escapedName = node->getName();
    if (escapedName.contains(",") || escapedName.contains("\"")) {
        escapedName = "\"" + escapedName.replace("\"", "\"\"") + "\"";
    }
    QString escapedPath = node->getPath();
    if (escapedPath.contains(",") || escapedPath.contains("\"")) {
        escapedPath = "\"" + escapedPath.replace("\"", "\"\"") + "\"";
    }

    stream << escapedName << ","
           << escapedPath << ","
           << node->getSize() << ","
           << (node->isDirectory() ? "Directory" : node->getCategory()) << ","
           << (node->isDirectory() ? "TRUE" : "FALSE") << "\n";

    for (const auto& child : node->getChildren()) {
        collectAllFilesCsv(child, stream);
    }
}

void SearchOrganizerView::exportToMarkdownFile(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Export Error", "Could not open file for writing.");
        return;
    }

    QTextStream out(&file);
    out << "# DiskLens Storage Analysis Report\n";
    out << "Generated on: " << QDateTime::currentDateTime().toString() << "\n\n";
    out << "## Directory Tree Structure Map\n\n";

    collectAllFilesMarkdown(m_rootNode, out, 0);

    file.close();
    QMessageBox::information(this, "Export Success", "Successfully saved full folder Markdown report.");
}

void SearchOrganizerView::collectAllFilesMarkdown(std::shared_ptr<FileSystemNode> node, QTextStream& stream, int indent) {
    if (!node) return;

    QString indentStr = QString(indent * 2, ' ');
    QString typeMarker = node->isDirectory() ? "[D]" : "[F]";
    
    stream << indentStr << "- " << typeMarker << " " << node->getName() 
           << " (" << formatBytes(node->getSize()) << ")\n";

    if (node->isDirectory()) {
        for (const auto& child : node->getChildren()) {
            collectAllFilesMarkdown(child, stream, indent + 1);
        }
    }
}

QString SearchOrganizerView::formatBytes(qint64 bytes) {
    double size = bytes;
    QStringList units = {"B", "KB", "MB", "GB", "TB"};
    int u = 0;
    while (size >= 1024.0 && u < units.size() - 1) {
        size /= 1024.0;
        u++;
    }
    return QString::number(size, 'f', u > 0 ? 1 : 0) + " " + units[u];
}
