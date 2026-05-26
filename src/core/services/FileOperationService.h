#ifndef FILEOPERATIONSERVICE_H
#define FILEOPERATIONSERVICE_H

#include <QString>
#include <QStringList>
#include <QObject>

/**
 * @brief Result returned by every FileOperationService method.
 *
 * Using a result type instead of a bare bool gives callers the ability
 * to distinguish success / user-cancel / error and display the right
 * message without catching exceptions.
 */
struct OperationResult {
    bool    success      = false;
    bool    userCancelled = false;
    QString errorMessage;

    static OperationResult ok()           { return {true,  false, {}}; }
    static OperationResult cancelled()    { return {false, true,  {}}; }
    static OperationResult fail(const QString& msg) { return {false, false, msg}; }
};

/**
 * @brief Conflict resolution policy for copy / move operations.
 */
enum class ConflictPolicy {
    Skip,        ///< Leave the destination file untouched
    Overwrite,   ///< Replace the destination file
    AutoRename   ///< Append " (1)", " (2)", ... to the destination name
};

/**
 * @brief Summary returned after a batch delete operation.
 */
struct BatchDeleteResult {
    int  succeeded  = 0;
    int  failed     = 0;
    QStringList failedPaths;
};

class FileOperationService : public QObject {
    Q_OBJECT
public:
    explicit FileOperationService(QObject* parent = nullptr);
    ~FileOperationService() = default;

    // ── Trash / delete ────────────────────────────────────────────
    OperationResult moveToTrash      (const QString& filePath);
    OperationResult deletePermanently(const QString& filePath);
    BatchDeleteResult batchMoveToTrash(const QStringList& paths);
    BatchDeleteResult batchDeletePermanently(const QStringList& paths);

    // ── Copy / move ───────────────────────────────────────────────
    OperationResult copyFile(const QString& sourcePath,
                             const QString& destDir,
                             ConflictPolicy policy = ConflictPolicy::AutoRename);
    OperationResult moveFile(const QString& sourcePath,
                             const QString& destDir,
                             ConflictPolicy policy = ConflictPolicy::AutoRename);

    // ── Rename ────────────────────────────────────────────────────
    OperationResult renameFile(const QString& filePath, const QString& newName);

    // ── Create ────────────────────────────────────────────────────
    OperationResult createFolder(const QString& parentPath, const QString& folderName);

    // ── Shell integration ─────────────────────────────────────────
    OperationResult showInExplorer(const QString& filePath);
    OperationResult openFile      (const QString& filePath);

private:
    static bool isNameSafe(const QString& name);
    static QString resolveConflict(const QString& destDir,
                                   const QString& fileName,
                                   ConflictPolicy policy);
};

#endif // FILEOPERATIONSERVICE_H
