#ifndef TREEMAPWIDGET_H
#define TREEMAPWIDGET_H

#include "core/models/FileSystemNode.h"
#include <QWidget>
#include <QVector>
#include <QRectF>
#include <memory>

class TreemapWidget : public QWidget {
    Q_OBJECT
public:
    enum ColorMode {
        CategoryColor,
        DepthColor
    };

    explicit TreemapWidget(QWidget* parent = nullptr);
    ~TreemapWidget() = default;

    void setRootNode(std::shared_ptr<FileSystemNode> root);
    void setColorMode(ColorMode mode);
    void clear();

signals:
    void nodeHovered(std::shared_ptr<FileSystemNode> node);
    void nodeClicked(std::shared_ptr<FileSystemNode> node);
    void nodeDoubleClicked(std::shared_ptr<FileSystemNode> node);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    struct RectNode {
        std::shared_ptr<FileSystemNode> node;
        QRectF rect;
        int depth = 0;
    };

    std::shared_ptr<FileSystemNode> m_rootNode;
    ColorMode m_colorMode = CategoryColor;
    QVector<RectNode> m_layoutNodes;
    int m_hoveredIndex = -1;

    void calculateLayout();
    void layoutRecursive(std::shared_ptr<FileSystemNode> node, const QRectF& rect, int depth, bool horizontal);
    QColor getColorForNode(const RectNode& rectNode);
};

#endif // TREEMAPWIDGET_H
