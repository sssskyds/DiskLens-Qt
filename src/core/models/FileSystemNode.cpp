#include "FileSystemNode.h"

FileSystemNode::FileSystemNode(const QString& name, const QString& path, bool isDir)
    : m_name(name), m_path(path), m_isDir(isDir)
{
    if (!m_isDir) {
        int dotIndex = name.lastIndexOf('.');
        if (dotIndex != -1) {
            m_extension = name.mid(dotIndex).toLower();
        } else {
            m_extension = "";
        }
    }
}

void FileSystemNode::addChild(std::shared_ptr<FileSystemNode> child) {
    if (child) {
        child->setParent(shared_from_this());
        m_children.push_back(child);
    }
}

void FileSystemNode::addSize(qint64 size) {
    m_size += size;
    auto parent = getParent();
    if (parent) {
        parent->addSize(size);
    }
}

int FileSystemNode::getDepth() const {
    int depth = 0;
    auto parent = getParent();
    while (parent) {
        depth++;
        parent = parent->getParent();
    }
    return depth;
}
