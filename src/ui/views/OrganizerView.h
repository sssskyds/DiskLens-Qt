#ifndef ORGANIZERVIEW_H
#define ORGANIZERVIEW_H

#include "core/services/OrganizerService.h"
#include <QWidget>
#include <QComboBox>
#include <QTableWidget>
#include <QPushButton>
#include <QLabel>
#include <QCheckBox>
#include <QLineEdit>

class OrganizerView : public QWidget {
    Q_OBJECT
public:
    explicit OrganizerView(QWidget* parent = nullptr);
    void setRootPath(const QString& path);
    void clear();

private slots:
    void onPreviewClicked();
    void onApplyClicked();
    void onConflictModeChanged(int index);
    void onApplyProgress(int completed, int total, const QString& currentFile);
    void onApplyFinished(int succeeded, int failed, int skipped);

private:
    void setupUI();
    OrganizerRule ruleFromIndex(int index);

    QString m_rootPath;
    OrganizerService m_organizer;

    QComboBox*    m_ruleCombo = nullptr;
    QCheckBox*    m_recursiveCheck = nullptr;
    QLineEdit*    m_customGlobEdit = nullptr;
    QLineEdit*    m_customFolderEdit = nullptr;
    QPushButton*  m_previewBtn = nullptr;
    QPushButton*  m_applyBtn = nullptr;
    QComboBox*    m_conflictCombo = nullptr;
    QTableWidget* m_previewTable = nullptr;
    QLabel*       m_statusLabel = nullptr;

    QVector<OrganizeAction> m_pendingActions;
};

#endif // ORGANIZERVIEW_H
