#ifndef SHELLINTEGRATIONSERVICE_H
#define SHELLINTEGRATIONSERVICE_H

#include <QObject>
#include <QString>
#include <QStringList>

/**
 * @brief Platform-abstracted shell integration for file operations.
 * Handles reveal-in-explorer, open-file, trash operations, and drive classification.
 */
class ShellIntegrationService : public QObject {
    Q_OBJECT
public:
    explicit ShellIntegrationService(QObject* parent = nullptr);

    enum class DriveType {
        Fixed,
        Removable,
        Network,
        Optical,
        Unknown
    };
    Q_ENUM(DriveType)

    // --- Shell Actions ---
    bool revealInExplorer(const QString& filePath);
    bool openFile(const QString& filePath);
    bool openFolder(const QString& folderPath);

    // --- Trash / Delete ---
    bool moveToTrash(const QString& filePath);
    bool moveToTrash(const QStringList& filePaths);
    bool permanentDelete(const QString& filePath);
    bool permanentDelete(const QStringList& filePaths);

    // --- Drive Classification ---
    DriveType classifyDrive(const QString& rootPath);
    bool isNetworkPath(const QString& path);
    QString driveTypeToString(DriveType type);

    // --- Clipboard ---
    void copyPathToClipboard(const QString& path);

signals:
    void operationCompleted(bool success, const QString& message);
    void operationFailed(const QString& errorMessage);
};

#endif // SHELLINTEGRATIONSERVICE_H
