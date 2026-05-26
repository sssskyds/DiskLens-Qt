#include "ui/dialogs/ExportDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QDesktopServices>
#include <QUrl>

ExportDialog::ExportDialog(std::shared_ptr<FileSystemNode> root, QWidget* parent)
    : QDialog(parent), m_root(root) {
    setWindowTitle("Export — DiskLens");
    setMinimumSize(500, 450);
    setupUI();

    connect(&m_exportService, &ExportService::progressUpdated, this, &ExportDialog::onProgress);
    connect(&m_exportService, &ExportService::exportFinished, this, &ExportDialog::onFinished);
}

void ExportDialog::setupUI() {
    QVBoxLayout* main = new QVBoxLayout(this);

    QLabel* title = new QLabel("Export Scan Results");
    title->setStyleSheet("font-size: 18px; font-weight: bold; color: #e5e7eb;");
    main->addWidget(title);

    // Format
    QGroupBox* formatGroup = new QGroupBox("Export Format");
    formatGroup->setStyleSheet("QGroupBox { color: #00d4ff; border: 1px solid #374151; border-radius: 8px; padding-top: 14px; }");
    QHBoxLayout* formatLayout = new QHBoxLayout(formatGroup);
    m_formatCombo = new QComboBox();
    m_formatCombo->addItems({"Tree Diagram (.txt)", "Markdown (.md)", "CSV (.csv)"});
    formatLayout->addWidget(new QLabel("Format:"));
    formatLayout->addWidget(m_formatCombo, 1);
    main->addWidget(formatGroup);

    // Options
    QGroupBox* optGroup = new QGroupBox("Include Columns");
    optGroup->setStyleSheet("QGroupBox { color: #00d4ff; border: 1px solid #374151; border-radius: 8px; padding-top: 14px; }");
    QGridLayout* optGrid = new QGridLayout(optGroup);

    m_includeSize = new QCheckBox("File Size"); m_includeSize->setChecked(true);
    m_includeModified = new QCheckBox("Modified Date"); m_includeModified->setChecked(true);
    m_includeCreated = new QCheckBox("Created Date"); m_includeCreated->setChecked(true);
    m_includeCategory = new QCheckBox("Category"); m_includeCategory->setChecked(true);
    m_includeExtension = new QCheckBox("Extension"); m_includeExtension->setChecked(true);
    m_includeOwner = new QCheckBox("Owner (may be unavailable)");
    m_includeHidden = new QCheckBox("Include Hidden Files");

    optGrid->addWidget(m_includeSize, 0, 0);
    optGrid->addWidget(m_includeModified, 0, 1);
    optGrid->addWidget(m_includeCreated, 1, 0);
    optGrid->addWidget(m_includeCategory, 1, 1);
    optGrid->addWidget(m_includeExtension, 2, 0);
    optGrid->addWidget(m_includeOwner, 2, 1);
    optGrid->addWidget(m_includeHidden, 3, 0, 1, 2);

    QHBoxLayout* depthLayout = new QHBoxLayout();
    depthLayout->addWidget(new QLabel("Max Depth:"));
    m_depthSpin = new QSpinBox(); m_depthSpin->setRange(1, 50); m_depthSpin->setValue(6);
    depthLayout->addWidget(m_depthSpin);
    depthLayout->addStretch();
    optGrid->addLayout(depthLayout, 4, 0, 1, 2);

    main->addWidget(optGroup);

    // Output path
    QHBoxLayout* pathLayout = new QHBoxLayout();
    pathLayout->addWidget(new QLabel("Save to:"));
    m_outputPathEdit = new QLineEdit();
    m_outputPathEdit->setPlaceholderText("Select output file...");
    QPushButton* browseBtn = new QPushButton("Browse...");
    browseBtn->setStyleSheet("background: #374151; color: #e5e7eb; padding: 4px 10px; border-radius: 4px;");
    pathLayout->addWidget(m_outputPathEdit, 1);
    pathLayout->addWidget(browseBtn);
    main->addLayout(pathLayout);

    connect(browseBtn, &QPushButton::clicked, this, &ExportDialog::onBrowseClicked);

    // Progress
    m_progress = new QProgressBar();
    m_progress->setRange(0, 0);
    m_progress->setVisible(false);
    m_progress->setFixedHeight(4);
    main->addWidget(m_progress);

    m_statusLabel = new QLabel("");
    m_statusLabel->setStyleSheet("color: #94a3b8;");
    main->addWidget(m_statusLabel);

    // Export button
    m_exportBtn = new QPushButton("📤 Export");
    m_exportBtn->setStyleSheet("background: #00d4ff; color: #000; padding: 10px 30px; font-weight: bold; font-size: 14px; border-radius: 6px;");
    main->addWidget(m_exportBtn);

    connect(m_exportBtn, &QPushButton::clicked, this, &ExportDialog::onExportClicked);
}

void ExportDialog::onBrowseClicked() {
    QStringList filters;
    switch (m_formatCombo->currentIndex()) {
        case 0: filters << "Text files (*.txt)"; break;
        case 1: filters << "Markdown files (*.md)"; break;
        case 2: filters << "CSV files (*.csv)"; break;
    }
    filters << "All files (*)";

    QString path = QFileDialog::getSaveFileName(this, "Export File", "", filters.join(";;"));
    if (!path.isEmpty()) m_outputPathEdit->setText(path);
}

void ExportDialog::onExportClicked() {
    if (!m_root) {
        QMessageBox::warning(this, "No Data", "No scan data available. Scan a drive first.");
        return;
    }

    QString outputPath = m_outputPathEdit->text().trimmed();
    if (outputPath.isEmpty()) {
        onBrowseClicked();
        outputPath = m_outputPathEdit->text().trimmed();
        if (outputPath.isEmpty()) return;
    }

    ExportOptions opts;
    switch (m_formatCombo->currentIndex()) {
        case 0: opts.format = ExportOptions::TreeText; break;
        case 1: opts.format = ExportOptions::Markdown; break;
        case 2: opts.format = ExportOptions::CSV; break;
    }
    opts.maxDepth = m_depthSpin->value();
    opts.includeHidden = m_includeHidden->isChecked();
    opts.includeSize = m_includeSize->isChecked();
    opts.includeCreatedDate = m_includeCreated->isChecked();
    opts.includeModifiedDate = m_includeModified->isChecked();
    opts.includeOwner = m_includeOwner->isChecked();
    opts.includeCategory = m_includeCategory->isChecked();
    opts.includeExtension = m_includeExtension->isChecked();

    m_exportBtn->setEnabled(false);
    m_progress->setVisible(true);
    m_statusLabel->setText("Exporting...");

    m_exportService.startExport(m_root, outputPath, opts);
}

void ExportDialog::onProgress(int itemsWritten, const QString& currentItem) {
    m_statusLabel->setText(QString("Exported %1 items — %2").arg(itemsWritten).arg(currentItem));
}

void ExportDialog::onFinished(bool success, const QString& outputPath) {
    m_exportBtn->setEnabled(true);
    m_progress->setVisible(false);

    if (success) {
        m_statusLabel->setText("Export complete!");
        auto reply = QMessageBox::question(this, "Export Complete",
            QString("File saved to:\n%1\n\nOpen the file?").arg(outputPath));
        if (reply == QMessageBox::Yes) {
            QDesktopServices::openUrl(QUrl::fromLocalFile(outputPath));
        }
        accept();
    } else {
        m_statusLabel->setText("Export failed.");
        QMessageBox::critical(this, "Export Failed", "Failed to write the export file. Check permissions.");
    }
}
