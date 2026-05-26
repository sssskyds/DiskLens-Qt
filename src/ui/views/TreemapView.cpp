#include "TreemapView.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QMessageBox>
#include <QDebug>

TreemapView::TreemapView(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void TreemapView::setupUI() {
    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(15);

    // Header Controls
    auto headerLayout = new QHBoxLayout();
    auto titleLabel = new QLabel("Space Map Visualizer", this);
    titleLabel->setStyleSheet("font-size: 24px; font-weight: bold; color: #ffffff;");
    headerLayout->addWidget(titleLabel);

    auto modeLayout = new QHBoxLayout();
    auto modeLabel = new QLabel("Color Mode:", this);
    modeLabel->setStyleSheet("color: #94a3b8;");
    auto modeCombo = new QComboBox(this);
    modeCombo->addItem("File Category");
    modeCombo->addItem("Folder Depth");
    connect(modeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &TreemapView::onColorModeChanged);
    modeLayout->addWidget(modeLabel);
    modeLayout->addWidget(modeCombo);
    headerLayout->addLayout(modeLayout);

    mainLayout->addLayout(headerLayout);

    // Hovered Path Indicator Bar
    m_hoveredPathLabel = new QLabel("Hover over a block to view its path...", this);
    m_hoveredPathLabel->setStyleSheet("background-color: rgba(255, 255, 255, 0.02); padding: 8px 12px; border-radius: 6px; color: #94a3b8; border: 1px solid rgba(255,255,255,0.04);");
    mainLayout->addWidget(m_hoveredPathLabel);

    // Main Treemap Widget
    m_treemapWidget = new TreemapWidget(this);
    connect(m_treemapWidget, &TreemapWidget::nodeHovered,       this, &TreemapView::onNodeHovered);
    connect(m_treemapWidget, &TreemapWidget::nodeClicked,       this, &TreemapView::onNodeClicked);
    connect(m_treemapWidget, &TreemapWidget::nodeDoubleClicked, this, &TreemapView::onNodeDoubleClicked);
    mainLayout->addWidget(m_treemapWidget, 1);

    // Bottom Selected Details Panel
    auto detailsCard = new QFrame(this);
    detailsCard->setProperty("class", "card");
    detailsCard->setStyleSheet(
        "QFrame {"
        "  background-color: rgba(255, 255, 255, 0.02);"
        "  border: 1px solid rgba(255, 255, 255, 0.06);"
        "  border-radius: 8px;"
        "}"
    );

    auto cardLayout = new QHBoxLayout(detailsCard);
    cardLayout->setContentsMargins(16, 12, 16, 12);
    cardLayout->setSpacing(20);

    auto textLayout = new QVBoxLayout();
    m_selectedNameLabel = new QLabel("Select a block to explore details", this);
    m_selectedNameLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: #ffffff;");

    auto subDetailsLayout = new QHBoxLayout();
    m_selectedSizeLabel = new QLabel("Size: --", this);
    m_selectedSizeLabel->setStyleSheet("color: #94a3b8;");
    m_selectedTypeLabel = new QLabel("Type: --", this);
    m_selectedTypeLabel->setStyleSheet("color: #94a3b8;");
    subDetailsLayout->addWidget(m_selectedSizeLabel);
    subDetailsLayout->addWidget(m_selectedTypeLabel);
    subDetailsLayout->addStretch();

    textLayout->addWidget(m_selectedNameLabel);
    textLayout->addLayout(subDetailsLayout);
    cardLayout->addLayout(textLayout, 1);

    m_revealBtn = new QPushButton("Reveal in Explorer", this);
    m_revealBtn->setProperty("class", "secondary-btn");
    m_revealBtn->setEnabled(false);
    connect(m_revealBtn, &QPushButton::clicked, this, &TreemapView::onRevealClicked);

    m_deleteBtn = new QPushButton("Move to Trash", this);
    m_deleteBtn->setProperty("class", "danger-btn");
    m_deleteBtn->setEnabled(false);
    connect(m_deleteBtn, &QPushButton::clicked, this, &TreemapView::onDeleteClicked);

    cardLayout->addWidget(m_revealBtn);
    cardLayout->addWidget(m_deleteBtn);

    mainLayout->addWidget(detailsCard);
}

void TreemapView::setScanResult(std::shared_ptr<FileSystemNode> root) {
    m_treemapWidget->setRootNode(root);
    clear();
}

void TreemapView::clear() {
    m_selectedNode.reset();
    m_selectedNameLabel->setText("Select a block to explore details");
    m_selectedSizeLabel->setText("Size: --");
    m_selectedTypeLabel->setText("Type: --");
    m_revealBtn->setEnabled(false);
    m_deleteBtn->setEnabled(false);
    m_hoveredPathLabel->setText("Hover over a block to view its path...");
}

void TreemapView::onNodeHovered(std::shared_ptr<FileSystemNode> node) {
    if (node) {
        m_hoveredPathLabel->setText(node->getPath());
    } else {
        m_hoveredPathLabel->setText("Hover over a block to view its path...");
    }
}

void TreemapView::onNodeClicked(std::shared_ptr<FileSystemNode> node) {
    if (!node) return;

    m_selectedNode = node;
    m_selectedNameLabel->setText(node->getName());
    m_selectedSizeLabel->setText("Size: " + formatBytes(node->getSize()));
    m_selectedTypeLabel->setText(QString("Type: %1").arg(node->isDirectory() ? "Directory" : "File (" + node->getExtension() + ")"));

    m_revealBtn->setEnabled(true);
    m_deleteBtn->setEnabled(node->getParent() != nullptr);
}

void TreemapView::onNodeDoubleClicked(std::shared_ptr<FileSystemNode> node) {
    if (node && !node->isDirectory()) {
        m_fileOpService.openFile(node->getPath());
    }
}

void TreemapView::onRevealClicked() {
    if (m_selectedNode) {
        m_fileOpService.showInExplorer(m_selectedNode->getPath());
    }
}

void TreemapView::onDeleteClicked() {
    if (!m_selectedNode) return;

    QString msg = QString("Are you sure you want to move this %1 to the Recycle Bin?\n\n%2")
                      .arg(m_selectedNode->isDirectory() ? "folder" : "file")
                      .arg(m_selectedNode->getName());

    auto reply = QMessageBox::question(this, "Move to Recycle Bin?", msg,
                                       QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        OperationResult res = m_fileOpService.moveToTrash(m_selectedNode->getPath());
        if (res.success) {
            QMessageBox::information(this, "Success", "Moved to Recycle Bin successfully.");
            clear();
            m_treemapWidget->update();
        } else {
            QMessageBox::critical(this, "Error", "Failed to move the item to the Recycle Bin. It might be in use or you lack permissions.");
        }
    }
}

void TreemapView::onColorModeChanged(int index) {
    auto mode = static_cast<TreemapWidget::ColorMode>(index);
    m_treemapWidget->setColorMode(mode);
}

QString TreemapView::formatBytes(qint64 bytes) {
    double size = bytes;
    QStringList units = {"B", "KB", "MB", "GB", "TB"};
    int u = 0;
    while (size >= 1024.0 && u < units.size() - 1) {
        size /= 1024.0;
        u++;
    }
    return QString::number(size, 'f', u > 0 ? 1 : 0) + " " + units[u];
}
