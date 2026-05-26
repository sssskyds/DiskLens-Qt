#ifndef FILESYSTEMNODE_H
#define FILESYSTEMNODE_H

#include <QString>
#include <QVector>
#include <QDateTime>
#include <memory>

class FileSystemNode : public std::enable_shared_from_this<FileSystemNode> {
public:
    FileSystemNode(const QString& name, const QString& path, bool isDir);
    ~FileSystemNode() = default;

    QString getName() const { return m_name; }
    QString getPath() const { return m_path; }
    bool isDirectory() const { return m_isDir; }
    
    qint64 getSize() const { return m_size; }
    void setSize(qint64 size) { m_size = size; }
    void addSize(qint64 size);

    QDateTime getLastModified() const { return m_lastModified; }
    void setLastModified(const QDateTime& dt) { m_lastModified = dt; }

    QString getExtension() const { return m_extension; }
    void setExtension(const QString& ext) { m_extension = ext; }

    QString getCategory() const { return m_category; }
    void setCategory(const QString& cat) { m_category = cat; }

    std::shared_ptr<FileSystemNode> getParent() const { return m_parent.lock(); }
    void setParent(std::shared_ptr<FileSystemNode> parent) { m_parent = parent; }

    void addChild(std::shared_ptr<FileSystemNode> child);
    const QVector<std::shared_ptr<FileSystemNode>>& getChildren() const { return m_children; }
    
    // Depth of the node in the scan tree
    int getDepth() const;

private:
    QString m_name;
    QString m_path;
    bool m_isDir;
    qint64 m_size = 0;
    QDateTime m_lastModified;
    QString m_extension;
    QString m_category;
    
    std::weak_ptr<FileSystemNode> m_parent;
    QVector<std::shared_ptr<FileSystemNode>> m_children;
};

#endif // FILESYSTEMNODE_H
