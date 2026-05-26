#ifndef FILEOPERATIONSERVICE_H
#define FILEOPERATIONSERVICE_H

#include <QString>
#include <QObject>

class FileOperationService : public QObject {
    Q_OBJECT
public:
    explicit FileOperationService(QObject* parent = nullptr);
    ~FileOperationService() = default;

    bool moveToTrash(const QString& filePath);
    bool deletePermanently(const QString& filePath);
    bool renameFile(const QString& filePath, const QString& newName);
    bool showInExplorer(const QString& filePath);
    bool openFile(const QString& filePath);
};

#endif // FILEOPERATIONSERVICE_H
