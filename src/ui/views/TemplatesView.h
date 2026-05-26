#ifndef TEMPLATESVIEW_H
#define TEMPLATESVIEW_H

#include "core/services/TemplateService.h"
#include <QWidget>
#include <QListWidget>
#include <QTextEdit>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>

class TemplatesView : public QWidget {
    Q_OBJECT
public:
    explicit TemplatesView(QWidget* parent = nullptr);

private slots:
    void onTemplateSelected(int row);
    void onCreateClicked();
    void onCaptureClicked();
    void onDeleteCustomClicked();

private:
    void setupUI();
    void refreshList();

    TemplateService m_templateService;
    QListWidget* m_templateList = nullptr;
    QTextEdit*   m_previewText = nullptr;
    QLineEdit*   m_projectNameEdit = nullptr;
    QLineEdit*   m_destDirEdit = nullptr;
    QPushButton* m_createBtn = nullptr;
    QPushButton* m_captureBtn = nullptr;
    QPushButton* m_deleteBtn = nullptr;
    QLabel*      m_descLabel = nullptr;
    QString      m_selectedTemplateId;
};

#endif // TEMPLATESVIEW_H
