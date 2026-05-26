#include "ui/views/OrganizerView.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QMessageBox>

OrganizerView::OrganizerView(QWidget* parent) : QWidget(parent) {
    setupUI();
    connect(&m_organizer, &OrganizerService::applyProgress,
            this, &OrganizerView::onApplyProgress);
    connect(&m_organizer, &OrganizerService::applyFinished,
            this, &OrganizerView::onApplyFinished);
}

void OrganizerView::setRootPath(const QString& path) { m_rootPath = path; }

void OrganizerView::clear() {
    m_previewTable->setRowCount(0);
    m_pendingActions.clear();
    m_applyBtn->setEnabled(false);
    m_statusLabel->setText("Select a rule and click Preview to see planned moves.");
}

void OrganizerView::setupUI() {
    QVBoxLayout* main = new QVBoxLayout(this);
    main->setContentsMargins(16, 16, 16, 16);

    QLabel* title = new QLabel("Folder Organizer");
    title->setStyleSheet("font-size: 20px; font-weight: bold; color: #e5e7eb;");
    main->addWidget(title);

    QLabel* desc = new QLabel("Organize files into folders based on rules. Preview before applying.");
    desc->setStyleSheet("color: #94a3b8; margin-bottom: 8px;");
    main->addWidget(desc);

    // Rule selection
    QGroupBox* ruleGroup = new QGroupBox("Organization Rule");
    ruleGroup->setStyleSheet("QGroupBox { color: #00d4ff; border: 1px solid #374151; border-radius: 8px; padding-top: 14px; }");
    QGridLayout* ruleGrid = new QGridLayout(ruleGroup);

    ruleGrid->addWidget(new QLabel("Rule:"), 0, 0);
    m_ruleCombo = new QComboBox();
    m_ruleCombo->addItems(OrganizerService::availableRules());
    ruleGrid->addWidget(m_ruleCombo, 0, 1);

    m_recursiveCheck = new QCheckBox("Include subdirectories (recursive)");
    ruleGrid->addWidget(m_recursiveCheck, 1, 0, 1, 2);

    ruleGrid->addWidget(new QLabel("Custom Glob:"), 2, 0);
    m_customGlobEdit = new QLineEdit();
    m_customGlobEdit->setPlaceholderText("e.g., *.log (only for Custom Pattern rule)");
    ruleGrid->addWidget(m_customGlobEdit, 2, 1);

    ruleGrid->addWidget(new QLabel("Target Folder:"), 3, 0);
    m_customFolderEdit = new QLineEdit();
    m_customFolderEdit->setPlaceholderText("Destination folder name");
    ruleGrid->addWidget(m_customFolderEdit, 3, 1);

    ruleGrid->addWidget(new QLabel("Conflict:"), 4, 0);
    m_conflictCombo = new QComboBox();
    m_conflictCombo->addItems({"Skip", "Overwrite", "Auto-rename"});
    ruleGrid->addWidget(m_conflictCombo, 4, 1);

    main->addWidget(ruleGroup);

    // Action buttons
    QHBoxLayout* actionBar = new QHBoxLayout();
    m_previewBtn = new QPushButton("📋 Preview");
    m_previewBtn->setStyleSheet("background: #7c3aed; color: #fff; padding: 8px 20px; font-weight: bold; border-radius: 6px;");
    m_applyBtn = new QPushButton("✅ Apply");
    m_applyBtn->setStyleSheet("background: #3fb950; color: #000; padding: 8px 20px; font-weight: bold; border-radius: 6px;");
    m_applyBtn->setEnabled(false);
    m_statusLabel = new QLabel("Select a rule and click Preview.");
    m_statusLabel->setStyleSheet("color: #94a3b8;");

    actionBar->addWidget(m_previewBtn);
    actionBar->addWidget(m_applyBtn);
    actionBar->addStretch();
    actionBar->addWidget(m_statusLabel);
    main->addLayout(actionBar);

    connect(m_previewBtn, &QPushButton::clicked, this, &OrganizerView::onPreviewClicked);
    connect(m_applyBtn, &QPushButton::clicked, this, &OrganizerView::onApplyClicked);

    // Preview table
    m_previewTable = new QTableWidget(0, 4);
    m_previewTable->setHorizontalHeaderLabels({"Source", "Destination", "Rule", "Conflict"});
    m_previewTable->horizontalHeader()->setStretchLastSection(true);
    m_previewTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_previewTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_previewTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_previewTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_previewTable->setAlternatingRowColors(true);
    main->addWidget(m_previewTable, 1);
}

OrganizerRule OrganizerView::ruleFromIndex(int index) {
    switch (index) {
        case 0: return OrganizerRule::ByExtensionCategory;
        case 1: return OrganizerRule::ByYearMonth;
        case 2: return OrganizerRule::BySizeTier;
        case 3: return OrganizerRule::ByFirstLetter;
        case 4: return OrganizerRule::ByCustomGlob;
        default: return OrganizerRule::ByExtensionCategory;
    }
}

void OrganizerView::onPreviewClicked() {
    if (m_rootPath.isEmpty()) {
        QMessageBox::warning(this, "No Path", "Scan a drive first to set the organizer root path.");
        return;
    }

    OrganizerRule rule = ruleFromIndex(m_ruleCombo->currentIndex());
    bool recursive = m_recursiveCheck->isChecked();

    m_pendingActions = m_organizer.preview(m_rootPath, rule, recursive,
        m_customGlobEdit->text(), m_customFolderEdit->text());

    m_previewTable->setRowCount(0);
    int conflicts = 0;
    for (const auto& action : m_pendingActions) {
        int row = m_previewTable->rowCount();
        m_previewTable->insertRow(row);
        m_previewTable->setItem(row, 0, new QTableWidgetItem(action.sourcePath));
        m_previewTable->setItem(row, 1, new QTableWidgetItem(action.destinationPath));
        m_previewTable->setItem(row, 2, new QTableWidgetItem(action.ruleApplied));

        QTableWidgetItem* conflictItem = new QTableWidgetItem(action.hasConflict ? "⚠ " + action.conflictInfo : "—");
        if (action.hasConflict) {
            conflictItem->setForeground(QColor("#d29922"));
            conflicts++;
        }
        m_previewTable->setItem(row, 3, conflictItem);
    }

    m_applyBtn->setEnabled(!m_pendingActions.isEmpty());
    m_statusLabel->setText(QString("Preview: %1 files to organize, %2 conflicts.")
        .arg(m_pendingActions.size()).arg(conflicts));
}

void OrganizerView::onApplyClicked() {
    if (m_pendingActions.isEmpty()) return;

    auto reply = QMessageBox::question(this, "Confirm Organize",
        QString("Move %1 files according to the preview?\nThis action modifies the file system.")
        .arg(m_pendingActions.size()));
    if (reply != QMessageBox::Yes) return;

    ConflictResolution mode;
    switch (m_conflictCombo->currentIndex()) {
        case 0: mode = ConflictResolution::Skip; break;
        case 1: mode = ConflictResolution::Overwrite; break;
        case 2: mode = ConflictResolution::AutoRename; break;
        default: mode = ConflictResolution::Skip;
    }

    m_applyBtn->setEnabled(false);
    m_previewBtn->setEnabled(false);
    m_organizer.apply(m_pendingActions, mode);
}

void OrganizerView::onConflictModeChanged(int index) { Q_UNUSED(index); }

void OrganizerView::onApplyProgress(int completed, int total, const QString& currentFile) {
    m_statusLabel->setText(QString("Organizing %1/%2 — %3").arg(completed).arg(total).arg(currentFile));
}

void OrganizerView::onApplyFinished(int succeeded, int failed, int skipped) {
    m_previewBtn->setEnabled(true);
    m_statusLabel->setText(QString("Done! %1 moved, %2 failed, %3 skipped.")
        .arg(succeeded).arg(failed).arg(skipped));
    m_pendingActions.clear();
    m_previewTable->setRowCount(0);

    if (failed > 0) {
        QMessageBox::warning(this, "Partial Failure",
            QString("%1 files could not be moved. Check permissions and paths.").arg(failed));
    }
}
