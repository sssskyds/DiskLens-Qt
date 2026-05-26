#ifndef EXPORTDIALOG_H
#define EXPORTDIALOG_H

#include "core/services/ExportService.h"
#include "core/models/FileSystemNode.h"
#include <QDialog>
#include <QComboBox>
#include <QCheckBox>
#include <QSpinBox>
#include <QLineEdit>
#include <QPushButton>
#include <QProgressBar>
#include <QLabel>
#include <memory>

class ExportDialog : public QDialog {
    Q_OBJECT
public:
    explicit ExportDialog(std::shared_ptr<FileSystemNode> root, QWidget* parent = nullptr);

private slots:
    void onBrowseClicked();
    void onExportClicked();
    void onProgress(int itemsWritten, const QString& currentItem);
    void onFinished(bool success, const QString& outputPath);

private:
    void setupUI();

    std::shared_ptr<FileSystemNode> m_root;
    ExportService m_exportService;

    QComboBox*    m_formatCombo = nullptr;
    QSpinBox*     m_depthSpin = nullptr;
    QCheckBox*    m_includeHidden = nullptr;
    QCheckBox*    m_includeSize = nullptr;
    QCheckBox*    m_includeCreated = nullptr;
    QCheckBox*    m_includeModified = nullptr;
    QCheckBox*    m_includeOwner = nullptr;
    QCheckBox*    m_includeCategory = nullptr;
    QCheckBox*    m_includeExtension = nullptr;
    QLineEdit*    m_outputPathEdit = nullptr;
    QPushButton*  m_exportBtn = nullptr;
    QProgressBar* m_progress = nullptr;
    QLabel*       m_statusLabel = nullptr;
};

#endif // EXPORTDIALOG_H
