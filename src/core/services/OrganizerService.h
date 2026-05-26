#ifndef ORGANIZERSERVICE_H
#define ORGANIZERSERVICE_H

#include <QObject>
#include <QString>
#include <QVector>
#include <QDateTime>

/**
 * @brief Describes a single planned file move during organization.
 */
struct OrganizeAction {
    QString sourcePath;
    QString destinationPath;
    QString ruleApplied;
    qint64  sizeBytes = 0;
    bool    hasConflict = false;
    QString conflictInfo;
};

/**
 * @brief Conflict resolution strategy.
 */
enum class ConflictResolution {
    Skip,
    Overwrite,
    AutoRename
};

/**
 * @brief Organizer rule types.
 */
enum class OrganizerRule {
    ByExtensionCategory,    // Images/, Videos/, Documents/, etc.
    ByYearMonth,            // 2024/01/, 2024/02/, etc. (based on modified time)
    BySizeTier,             // Tiny/, Small/, Medium/, Large/, Huge/
    ByFirstLetter,          // A/, B/, C/, ...
    ByCustomGlob            // User-defined glob → folder mapping
};

/**
 * @brief Service for rule-based bulk file organization.
 * Supports preview (dry-run) before applying changes.
 */
class OrganizerService : public QObject {
    Q_OBJECT
public:
    explicit OrganizerService(QObject* parent = nullptr);

    // Preview generates actions without touching the filesystem
    QVector<OrganizeAction> preview(const QString& sourceDir,
                                     OrganizerRule rule,
                                     bool recursive = false,
                                     const QString& customGlob = "",
                                     const QString& customFolder = "");

    // Apply executes the planned actions
    void apply(const QVector<OrganizeAction>& actions,
               ConflictResolution conflictMode);

    // Cancel running apply operation
    void cancel();

    // Static helpers
    static QString ruleToString(OrganizerRule rule);
    static QStringList availableRules();

signals:
    void applyProgress(int completed, int total, const QString& currentFile);
    void applyFinished(int succeeded, int failed, int skipped);
    void error(const QString& message);

private:
    QString classifyByExtension(const QString& ext);
    QString classifyBySizeTier(qint64 bytes);
    QString classifyByYearMonth(const QDateTime& modified);
    QString classifyByFirstLetter(const QString& name);
    QString resolveConflict(const QString& destPath, ConflictResolution mode);

    bool m_cancelRequested = false;
};

#endif // ORGANIZERSERVICE_H
