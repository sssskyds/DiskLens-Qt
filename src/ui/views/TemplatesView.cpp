#include "ui/views/TemplatesView.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QInputDialog>

TemplatesView::TemplatesView(QWidget* parent) : QWidget(parent) {
    setupUI();
    refreshList();
}

void TemplatesView::setupUI() {
    QVBoxLayout* main = new QVBoxLayout(this);
    main->setContentsMargins(16, 16, 16, 16);

    QLabel* title = new QLabel("Project Templates");
    title->setStyleSheet("font-size: 20px; font-weight: bold; color: #e5e7eb;");
    main->addWidget(title);

    QLabel* desc = new QLabel("Create project folder structures instantly from built-in or custom templates.");
    desc->setStyleSheet("color: #94a3b8; margin-bottom: 8px;");
    main->addWidget(desc);

    QSplitter* splitter = new QSplitter(Qt::Horizontal);

    // Left: template list
    QWidget* leftPanel = new QWidget();
    QVBoxLayout* leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0, 0, 0, 0);

    QLabel* listLabel = new QLabel("Available Templates");
    listLabel->setStyleSheet("color: #00d4ff; font-weight: bold;");
    leftLayout->addWidget(listLabel);

    m_templateList = new QListWidget();
    m_templateList->setStyleSheet("QListWidget { background: #1e1e2e; border: 1px solid #374151; border-radius: 6px; }"
                                   "QListWidget::item { padding: 8px; color: #e5e7eb; }"
                                   "QListWidget::item:selected { background: #7c3aed; }");
    leftLayout->addWidget(m_templateList);

    QHBoxLayout* listBtns = new QHBoxLayout();
    m_captureBtn = new QPushButton("📂 Capture from Folder");
    m_captureBtn->setStyleSheet("background: #374151; color: #e5e7eb; padding: 6px 12px; border-radius: 4px;");
    m_deleteBtn = new QPushButton("🗑 Delete Custom");
    m_deleteBtn->setStyleSheet("background: #374151; color: #f85149; padding: 6px 12px; border-radius: 4px;");
    m_deleteBtn->setEnabled(false);
    listBtns->addWidget(m_captureBtn);
    listBtns->addWidget(m_deleteBtn);
    leftLayout->addLayout(listBtns);

    splitter->addWidget(leftPanel);

    // Right: preview + create
    QWidget* rightPanel = new QWidget();
    QVBoxLayout* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0);

    m_descLabel = new QLabel("Select a template to see its preview.");
    m_descLabel->setStyleSheet("color: #94a3b8; font-style: italic;");
    rightLayout->addWidget(m_descLabel);

    QLabel* previewLabel = new QLabel("Template Preview");
    previewLabel->setStyleSheet("color: #00d4ff; font-weight: bold;");
    rightLayout->addWidget(previewLabel);

    m_previewText = new QTextEdit();
    m_previewText->setReadOnly(true);
    m_previewText->setStyleSheet("background: #0d1117; color: #e5e7eb; border: 1px solid #374151; border-radius: 6px; font-family: Consolas, monospace;");
    rightLayout->addWidget(m_previewText, 1);

    // Create controls
    QGroupBox* createGroup = new QGroupBox("Create Project");
    createGroup->setStyleSheet("QGroupBox { color: #3fb950; font-weight: bold; border: 1px solid #374151; border-radius: 8px; padding-top: 14px; }");
    QGridLayout* createGrid = new QGridLayout(createGroup);

    createGrid->addWidget(new QLabel("Project Name:"), 0, 0);
    m_projectNameEdit = new QLineEdit();
    m_projectNameEdit->setPlaceholderText("my-project");
    createGrid->addWidget(m_projectNameEdit, 0, 1);

    createGrid->addWidget(new QLabel("Destination:"), 1, 0);
    m_destDirEdit = new QLineEdit();
    m_destDirEdit->setPlaceholderText("Select destination directory...");
    QPushButton* browseBtn = new QPushButton("Browse...");
    browseBtn->setStyleSheet("background: #374151; color: #e5e7eb; padding: 4px 10px; border-radius: 4px;");
    QHBoxLayout* destLayout = new QHBoxLayout();
    destLayout->addWidget(m_destDirEdit, 1);
    destLayout->addWidget(browseBtn);
    createGrid->addLayout(destLayout, 1, 1);

    m_createBtn = new QPushButton("🚀 Create Project");
    m_createBtn->setStyleSheet("background: #3fb950; color: #000; padding: 10px 24px; font-weight: bold; font-size: 14px; border-radius: 6px;");
    m_createBtn->setEnabled(false);
    createGrid->addWidget(m_createBtn, 2, 0, 1, 2);

    rightLayout->addWidget(createGroup);
    splitter->addWidget(rightPanel);

    splitter->setSizes({300, 500});
    main->addWidget(splitter, 1);

    // Connections
    connect(m_templateList, &QListWidget::currentRowChanged, this, &TemplatesView::onTemplateSelected);
    connect(m_createBtn, &QPushButton::clicked, this, &TemplatesView::onCreateClicked);
    connect(m_captureBtn, &QPushButton::clicked, this, &TemplatesView::onCaptureClicked);
    connect(m_deleteBtn, &QPushButton::clicked, this, &TemplatesView::onDeleteCustomClicked);
    connect(browseBtn, &QPushButton::clicked, this, [this]() {
        QString dir = QFileDialog::getExistingDirectory(this, "Select Destination Directory");
        if (!dir.isEmpty()) m_destDirEdit->setText(dir);
    });
}

void TemplatesView::refreshList() {
    m_templateList->clear();
    auto templates = m_templateService.allTemplates();
    for (const auto& t : templates) {
        QString label = t.name;
        if (!t.isBuiltIn) label += " (Custom)";
        m_templateList->addItem(label);
    }
}

void TemplatesView::onTemplateSelected(int row) {
    auto templates = m_templateService.allTemplates();
    if (row < 0 || row >= templates.size()) {
        m_createBtn->setEnabled(false);
        m_deleteBtn->setEnabled(false);
        return;
    }

    const auto& t = templates[row];
    m_selectedTemplateId = t.id;
    m_descLabel->setText(t.description);
    m_createBtn->setEnabled(true);
    m_deleteBtn->setEnabled(!t.isBuiltIn);

    // Show preview
    QStringList preview = m_templateService.previewTemplate(t.id, m_projectNameEdit->text().isEmpty() ? "my-project" : m_projectNameEdit->text());
    QString previewText;
    for (const auto& path : preview) {
        previewText += path + "\n";
    }
    m_previewText->setPlainText(previewText);
}

void TemplatesView::onCreateClicked() {
    if (m_selectedTemplateId.isEmpty()) return;

    QString projectName = m_projectNameEdit->text().trimmed();
    if (projectName.isEmpty()) {
        QMessageBox::warning(this, "Missing Name", "Please enter a project name.");
        return;
    }

    QString destDir = m_destDirEdit->text().trimmed();
    if (destDir.isEmpty()) {
        destDir = QFileDialog::getExistingDirectory(this, "Select Destination Directory");
        if (destDir.isEmpty()) return;
        m_destDirEdit->setText(destDir);
    }

    if (m_templateService.createFromTemplate(m_selectedTemplateId, destDir, projectName)) {
        QMessageBox::information(this, "Success",
            QString("Project '%1' created successfully at:\n%2/%1").arg(projectName, destDir));
    } else {
        QMessageBox::critical(this, "Error", "Failed to create project. Check permissions.");
    }
}

void TemplatesView::onCaptureClicked() {
    QString folderPath = QFileDialog::getExistingDirectory(this, "Select Folder to Capture as Template");
    if (folderPath.isEmpty()) return;

    bool ok;
    QString name = QInputDialog::getText(this, "Template Name",
        "Enter a name for the new template:", QLineEdit::Normal, "My Template", &ok);
    if (!ok || name.isEmpty()) return;

    auto reply = QMessageBox::question(this, "Include File Content?",
        "Include file content in the template?\n(Only text files under 1 MB will be captured)");
    bool includeContent = (reply == QMessageBox::Yes);

    ProjectTemplate t = m_templateService.captureFromFolder(folderPath, name, includeContent);
    if (m_templateService.saveCustomTemplate(t)) {
        refreshList();
        QMessageBox::information(this, "Template Saved",
            QString("Template '%1' saved with %2 entries.").arg(name).arg(t.entries.size()));
    }
}

void TemplatesView::onDeleteCustomClicked() {
    if (m_selectedTemplateId.isEmpty()) return;

    auto reply = QMessageBox::question(this, "Delete Template",
        "Delete this custom template? This cannot be undone.");
    if (reply == QMessageBox::Yes) {
        m_templateService.deleteCustomTemplate(m_selectedTemplateId);
        m_selectedTemplateId.clear();
        refreshList();
    }
}
