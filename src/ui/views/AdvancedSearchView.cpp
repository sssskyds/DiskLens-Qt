#include "ui/views/AdvancedSearchView.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QMenu>
#include <QAction>
#include <QMessageBox>

AdvancedSearchView::AdvancedSearchView(QWidget* parent) : QWidget(parent) {
    setupUI();

    connect(&m_searchService, &SearchService::resultsBatch,
            this, &AdvancedSearchView::onResultsBatch);
    connect(&m_searchService, &SearchService::progressUpdated,
            this, &AdvancedSearchView::onProgressUpdated);
    connect(&m_searchService, &SearchService::searchFinished,
            this, &AdvancedSearchView::onSearchFinished);
}

void AdvancedSearchView::setRootPath(const QString& path) {
    m_rootPath = path;
}

void AdvancedSearchView::clear() {
    m_resultsTable->setRowCount(0);
    m_allResults.clear();
    m_statusLabel->setText("Enter search criteria and click Search.");
    m_progressBar->setVisible(false);
}

void AdvancedSearchView::setupUI() {
    QVBoxLayout* main = new QVBoxLayout(this);
    main->setContentsMargins(16, 16, 16, 16);

    // Title
    QLabel* title = new QLabel("Advanced Search");
    title->setStyleSheet("font-size: 20px; font-weight: bold; color: #e5e7eb;");
    main->addWidget(title);

    setupFilterPanel();
    main->addWidget(new QLabel()); // spacer

    // Action bar
    QHBoxLayout* actionBar = new QHBoxLayout();
    m_searchBtn = new QPushButton("🔍 Search");
    m_searchBtn->setStyleSheet("background: #00d4ff; color: #000; padding: 8px 20px; font-weight: bold; border-radius: 6px;");
    m_cancelBtn = new QPushButton("Cancel");
    m_cancelBtn->setStyleSheet("background: #f85149; color: #fff; padding: 8px 16px; border-radius: 6px;");
    m_cancelBtn->setEnabled(false);

    m_statusLabel = new QLabel("Enter search criteria and click Search.");
    m_statusLabel->setStyleSheet("color: #94a3b8;");

    m_progressBar = new QProgressBar();
    m_progressBar->setRange(0, 0); // indeterminate
    m_progressBar->setVisible(false);
    m_progressBar->setFixedHeight(4);

    actionBar->addWidget(m_searchBtn);
    actionBar->addWidget(m_cancelBtn);
    actionBar->addStretch();
    actionBar->addWidget(m_statusLabel);
    main->addLayout(actionBar);
    main->addWidget(m_progressBar);

    connect(m_searchBtn, &QPushButton::clicked, this, &AdvancedSearchView::onSearchClicked);
    connect(m_cancelBtn, &QPushButton::clicked, this, &AdvancedSearchView::onCancelClicked);

    setupResultsTable();
    main->addWidget(m_resultsTable, 1);
}

void AdvancedSearchView::setupFilterPanel() {
    QGroupBox* filterGroup = new QGroupBox("Search Filters");
    filterGroup->setStyleSheet("QGroupBox { color: #00d4ff; font-weight: bold; border: 1px solid #374151; border-radius: 8px; padding-top: 14px; }");

    QGridLayout* grid = new QGridLayout(filterGroup);
    grid->setSpacing(8);

    int row = 0;
    auto addRow = [&](const QString& label, QWidget* widget) {
        QLabel* lbl = new QLabel(label);
        lbl->setStyleSheet("color: #94a3b8; font-weight: normal;");
        grid->addWidget(lbl, row, 0);
        grid->addWidget(widget, row, 1);
        row++;
    };

    m_nameEdit = new QLineEdit();
    m_nameEdit->setPlaceholderText("Filename contains...");
    addRow("Name:", m_nameEdit);

    m_globEdit = new QLineEdit();
    m_globEdit->setPlaceholderText("e.g., *.log, report_*");
    addRow("Glob Pattern:", m_globEdit);

    m_extensionsEdit = new QLineEdit();
    m_extensionsEdit->setPlaceholderText("e.g., jpg,png,gif (comma-separated)");
    addRow("Extensions:", m_extensionsEdit);

    // Size range (horizontal)
    QHBoxLayout* sizeLayout = new QHBoxLayout();
    m_minSizeMB = new QSpinBox(); m_minSizeMB->setRange(0, 999999); m_minSizeMB->setSuffix(" MB"); m_minSizeMB->setSpecialValueText("No min");
    m_maxSizeMB = new QSpinBox(); m_maxSizeMB->setRange(0, 999999); m_maxSizeMB->setSuffix(" MB"); m_maxSizeMB->setSpecialValueText("No max");
    sizeLayout->addWidget(m_minSizeMB);
    sizeLayout->addWidget(new QLabel("to"));
    sizeLayout->addWidget(m_maxSizeMB);
    QWidget* sizeWidget = new QWidget();
    sizeWidget->setLayout(sizeLayout);
    addRow("Size Range:", sizeWidget);

    // Date range
    m_modifiedAfter = new QDateTimeEdit(QDateTime::currentDateTime().addYears(-1));
    m_modifiedAfter->setCalendarPopup(true);
    m_modifiedAfter->setDisplayFormat("yyyy-MM-dd");
    m_modifiedBefore = new QDateTimeEdit(QDateTime::currentDateTime());
    m_modifiedBefore->setCalendarPopup(true);
    m_modifiedBefore->setDisplayFormat("yyyy-MM-dd");
    QHBoxLayout* dateLayout = new QHBoxLayout();
    dateLayout->addWidget(m_modifiedAfter);
    dateLayout->addWidget(new QLabel("to"));
    dateLayout->addWidget(m_modifiedBefore);
    QWidget* dateWidget = new QWidget();
    dateWidget->setLayout(dateLayout);
    addRow("Modified Date:", dateWidget);

    m_categoryCombo = new QComboBox();
    m_categoryCombo->addItems({"(Any)", "Images", "Videos", "Audio", "Documents", "Code", "Archives", "Executables", "Fonts", "Data", "Other"});
    addRow("Category:", m_categoryCombo);

    m_depthSpin = new QSpinBox();
    m_depthSpin->setRange(1, 50);
    m_depthSpin->setValue(10);
    addRow("Max Depth:", m_depthSpin);

    // Options row
    QHBoxLayout* optionsLayout = new QHBoxLayout();
    m_includeHidden = new QCheckBox("Include Hidden");
    m_caseSensitive = new QCheckBox("Case Sensitive");
    m_regexCheck = new QCheckBox("Regex");
    optionsLayout->addWidget(m_includeHidden);
    optionsLayout->addWidget(m_caseSensitive);
    optionsLayout->addWidget(m_regexCheck);
    optionsLayout->addStretch();
    QWidget* optWidget = new QWidget();
    optWidget->setLayout(optionsLayout);
    addRow("Options:", optWidget);

    m_contentEdit = new QLineEdit();
    m_contentEdit->setPlaceholderText("Search inside text files (expensive on large trees)");
    addRow("Content Contains:", m_contentEdit);

    m_excludeEdit = new QLineEdit();
    m_excludeEdit->setPlaceholderText("e.g., *.tmp, node_modules (comma-separated)");
    addRow("Exclude Patterns:", m_excludeEdit);

    layout()->addWidget(filterGroup);
}

void AdvancedSearchView::setupResultsTable() {
    m_resultsTable = new QTableWidget(0, 5);
    m_resultsTable->setHorizontalHeaderLabels({"Name", "Path", "Size", "Modified", "Category"});
    m_resultsTable->horizontalHeader()->setStretchLastSection(true);
    m_resultsTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_resultsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_resultsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_resultsTable->setContextMenuPolicy(Qt::CustomContextMenu);
    m_resultsTable->setAlternatingRowColors(true);

    connect(m_resultsTable, &QTableWidget::cellDoubleClicked,
            this, &AdvancedSearchView::onResultDoubleClicked);
    connect(m_resultsTable, &QTableWidget::customContextMenuRequested,
            this, &AdvancedSearchView::onContextMenu);
}

SearchCriteria AdvancedSearchView::buildCriteria() {
    SearchCriteria c;
    c.nameText = m_nameEdit->text().trimmed();
    c.globPattern = m_globEdit->text().trimmed();

    QString extText = m_extensionsEdit->text().trimmed();
    if (!extText.isEmpty()) {
        c.extensions = extText.split(",", Qt::SkipEmptyParts);
        for (auto& e : c.extensions) e = e.trimmed().toLower();
    }

    if (m_minSizeMB->value() > 0)
        c.minSize = (quint64)m_minSizeMB->value() * 1024ULL * 1024ULL;
    if (m_maxSizeMB->value() > 0)
        c.maxSize = (quint64)m_maxSizeMB->value() * 1024ULL * 1024ULL;

    c.modifiedAfter = m_modifiedAfter->dateTime();
    c.modifiedBefore = m_modifiedBefore->dateTime();

    if (m_categoryCombo->currentIndex() > 0)
        c.categories = {m_categoryCombo->currentText()};

    c.maxDepth = m_depthSpin->value();
    c.includeHidden = m_includeHidden->isChecked();
    c.caseSensitive = m_caseSensitive->isChecked();
    c.regex = m_regexCheck->isChecked();
    c.containsText = m_contentEdit->text().trimmed();

    QString excText = m_excludeEdit->text().trimmed();
    if (!excText.isEmpty()) {
        c.excludePatterns = excText.split(",", Qt::SkipEmptyParts);
        for (auto& e : c.excludePatterns) e = e.trimmed();
    }

    return c;
}

void AdvancedSearchView::onSearchClicked() {
    if (m_rootPath.isEmpty()) {
        QMessageBox::warning(this, "No Path", "Scan a drive first to set the search root path.");
        return;
    }

    clear();
    m_searchBtn->setEnabled(false);
    m_cancelBtn->setEnabled(true);
    m_progressBar->setVisible(true);
    m_statusLabel->setText("Searching...");

    SearchCriteria criteria = buildCriteria();
    m_searchService.startSearch(m_rootPath, criteria);
}

void AdvancedSearchView::onCancelClicked() {
    m_searchService.cancelSearch();
    m_cancelBtn->setEnabled(false);
}

void AdvancedSearchView::onResultsBatch(QVector<std::shared_ptr<FileSystemNode>> results) {
    for (const auto& node : results) {
        int row = m_resultsTable->rowCount();
        m_resultsTable->insertRow(row);
        m_resultsTable->setItem(row, 0, new QTableWidgetItem(node->getName()));
        m_resultsTable->setItem(row, 1, new QTableWidgetItem(node->getPath()));
        m_resultsTable->setItem(row, 2, new QTableWidgetItem(formatBytes(node->getSize())));
        m_resultsTable->setItem(row, 3, new QTableWidgetItem(node->getLastModified().toString("yyyy-MM-dd HH:mm")));
        m_resultsTable->setItem(row, 4, new QTableWidgetItem(node->getCategory()));
        m_allResults.append(node);
    }
}

void AdvancedSearchView::onProgressUpdated(qint64 filesChecked, qint64 matchesFound, const QString& currentPath) {
    m_statusLabel->setText(QString("Checked %1 files, %2 matches — %3")
        .arg(filesChecked).arg(matchesFound).arg(currentPath));
}

void AdvancedSearchView::onSearchFinished(qint64 totalMatches, bool wasCancelled) {
    m_searchBtn->setEnabled(true);
    m_cancelBtn->setEnabled(false);
    m_progressBar->setVisible(false);

    QString status = wasCancelled
        ? QString("Search cancelled. %1 matches found.").arg(totalMatches)
        : QString("Search complete. %1 matches found.").arg(totalMatches);
    m_statusLabel->setText(status);
}

void AdvancedSearchView::onResultDoubleClicked(int row, int col) {
    Q_UNUSED(col);
    if (row >= 0 && row < m_allResults.size()) {
        m_shell.revealInExplorer(m_allResults[row]->getPath());
    }
}

void AdvancedSearchView::onContextMenu(const QPoint& pos) {
    int row = m_resultsTable->rowAt(pos.y());
    if (row < 0 || row >= m_allResults.size()) return;

    QMenu menu;
    QAction* openAction = menu.addAction("Open File");
    QAction* revealAction = menu.addAction("Reveal in Explorer");
    QAction* copyPathAction = menu.addAction("Copy Path");
    menu.addSeparator();
    QAction* deleteAction = menu.addAction("Move to Recycle Bin");
    deleteAction->setIcon(QIcon()); // Could add icon

    QAction* selected = menu.exec(m_resultsTable->viewport()->mapToGlobal(pos));
    if (!selected) return;

    auto node = m_allResults[row];
    if (selected == openAction) {
        m_shell.openFile(node->getPath());
    } else if (selected == revealAction) {
        m_shell.revealInExplorer(node->getPath());
    } else if (selected == copyPathAction) {
        m_shell.copyPathToClipboard(node->getPath());
    } else if (selected == deleteAction) {
        auto reply = QMessageBox::question(this, "Confirm Delete",
            QString("Move to Recycle Bin?\n%1").arg(node->getPath()));
        if (reply == QMessageBox::Yes) {
            m_shell.moveToTrash(node->getPath());
            m_resultsTable->removeRow(row);
            m_allResults.removeAt(row);
        }
    }
}

QString AdvancedSearchView::formatBytes(qint64 bytes) {
    if (bytes >= (qint64)1073741824) return QString::number(bytes / 1073741824.0, 'f', 2) + " GB";
    if (bytes >= 1048576) return QString::number(bytes / 1048576.0, 'f', 2) + " MB";
    if (bytes >= 1024) return QString::number(bytes / 1024.0, 'f', 1) + " KB";
    return QString::number(bytes) + " B";
}
