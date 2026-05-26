#include "TreemapWidget.h"
#include <QPainter>
#include <QMouseEvent>
#include <QToolTip>
#include <QDebug>
#include <cmath>

TreemapWidget::TreemapWidget(QWidget* parent)
    : QWidget(parent)
{
    setMinimumSize(400, 300);
    setMouseTracking(true);
}

void TreemapWidget::setRootNode(std::shared_ptr<FileSystemNode> root) {
    m_rootNode = root;
    m_hoveredIndex = -1;
    calculateLayout();
    update();
}

void TreemapWidget::setColorMode(ColorMode mode) {
    m_colorMode = mode;
    update();
}

void TreemapWidget::clear() {
    m_rootNode.reset();
    m_layoutNodes.clear();
    m_hoveredIndex = -1;
    update();
}

void TreemapWidget::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    calculateLayout();
}

void TreemapWidget::calculateLayout() {
    m_layoutNodes.clear();
    if (!m_rootNode || m_rootNode->getSize() == 0) return;

    QRectF canvasRect(5, 5, width() - 10, height() - 10);
    layoutRecursive(m_rootNode, canvasRect, 0, true);
}

void TreemapWidget::layoutRecursive(std::shared_ptr<FileSystemNode> node, const QRectF& rect, int depth, bool horizontal) {
    if (!node || rect.width() < 1.0 || rect.height() < 1.0) return;

    // Record the layout cell for this node
    RectNode rn;
    rn.node = node;
    rn.rect = rect;
    rn.depth = depth;
    m_layoutNodes.push_back(rn);

    // Limit visual nesting level to 4 to prevent visual overload & CPU bottlenecks
    if (depth >= 4) return;

    // Filter children with size > 0
    QVector<std::shared_ptr<FileSystemNode>> activeChildren;
    qint64 totalChildSize = 0;

    for (const auto& child : node->getChildren()) {
        if (child->getSize() > 0) {
            activeChildren.push_back(child);
            totalChildSize += child->getSize();
        }
    }

    if (activeChildren.isEmpty() || totalChildSize == 0) return;

    // Slice and Dice division
    double x = rect.x();
    double y = rect.y();
    double w = rect.width();
    double h = rect.height();

    for (const auto& child : activeChildren) {
        double pct = static_cast<double>(child->getSize()) / totalChildSize;
        QRectF childRect;

        if (horizontal) {
            double sliceWidth = w * pct;
            childRect = QRectF(x, y, sliceWidth, h);
            x += sliceWidth;
        } else {
            double sliceHeight = h * pct;
            childRect = QRectF(x, y, w, sliceHeight);
            y += sliceHeight;
        }

        // Subdivide child rect (alternate horizontal/vertical slicing)
        // Add padding between recursive boxes for visual depth
        QRectF paddedRect = childRect.adjusted(1, 1, -1, -1);
        layoutRecursive(child, paddedRect, depth + 1, !horizontal);
    }
}

QColor TreemapWidget::getColorForNode(const RectNode& rectNode) {
    if (m_colorMode == DepthColor) {
        // Sequenced color scale based on recursive tree nesting levels
        switch (rectNode.depth) {
            case 0: return QColor("#0f172a"); // Deep background slate
            case 1: return QColor("#1e1b4b"); // Navy / deep purple
            case 2: return QColor("#311042"); // Dark Violet
            case 3: return QColor("#4c0519"); // Deep Crimson
            default: return QColor("#831843"); // Hot pink depth
        }
    }

    // CategoryColor Mode: Leaves colored by extension group, directories colored dark slate
    auto node = rectNode.node;
    if (node->isDirectory()) {
        // Base semi-transparent directories to let nested leaf colors glow through
        return QColor(10, 15, 29, 200 - rectNode.depth * 30);
    }

    QString cat = node->getCategory();
    if (cat == "Images")      return QColor("#00d4ff"); // Cyan
    if (cat == "Videos")      return QColor("#8b5cf6"); // Purple
    if (cat == "Audio")       return QColor("#ec4899"); // Pink
    if (cat == "Documents")   return QColor("#eab308"); // Yellow
    if (cat == "Code")        return QColor("#10b981"); // Emerald
    if (cat == "Archives")    return QColor("#f97316"); // Orange
    if (cat == "Executables") return QColor("#ef4444"); // Red
    if (cat == "Fonts")       return QColor("#14b8a6"); // Teal
    if (cat == "Data")        return QColor("#3b82f6"); // Blue
    
    return QColor("#475569"); // Default other gray
}

void TreemapWidget::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    if (m_layoutNodes.isEmpty()) {
        painter.setPen(QColor("#64748b"));
        painter.setFont(QFont("Segoe UI", 12));
        painter.drawText(rect(), Qt::AlignCenter, "Select a drive and scan to visualize the space map.");
        return;
    }

    // Paint in order of depth (directories first, then leaves on top)
    for (int i = 0; i < m_layoutNodes.size(); ++i) {
        const auto& rn = m_layoutNodes[i];
        
        // Skip root node drawing if it spans the whole widget to look cleaner
        if (rn.depth == 0 && rn.node->isDirectory()) {
            continue;
        }

        QColor color = getColorForNode(rn);
        painter.setBrush(color);
        
        bool isHovered = (i == m_hoveredIndex);
        if (isHovered) {
            // Bright hover highlights
            painter.setPen(QPen(QColor("#00d4ff"), 2));
            QColor highlightColor = color.lighter(130);
            painter.setBrush(highlightColor);
        } else {
            // Thin borders between sibling items
            painter.setPen(QPen(QColor(255, 255, 255, 20), 1));
        }

        painter.drawRect(rn.rect);

        // Draw names inside larger cells
        if (rn.rect.width() > 60.0 && rn.rect.height() > 20.0) {
            painter.setPen(QColor("#ffffff"));
            QFont font("Segoe UI", rn.depth == 1 ? 9 : 8);
            font.setBold(rn.node->isDirectory());
            painter.setFont(font);

            // Shorten name if too wide
            QString nameText = rn.node->getName();
            QFontMetrics metrics(font);
            if (metrics.horizontalAdvance(nameText) > rn.rect.width() - 8) {
                nameText = metrics.elidedText(nameText, Qt::ElideRight, rn.rect.width() - 8);
            }

            QRectF textRect = rn.rect.adjusted(4, 2, -4, -2);
            painter.drawText(textRect, Qt::AlignLeft | Qt::AlignTop, nameText);
        }
    }
}

void TreemapWidget::mouseMoveEvent(QMouseEvent* event) {
    int prevHovered = m_hoveredIndex;
    m_hoveredIndex = -1;

    // Scan backwards so leaves (drawn last, higher index) take mouse priority over parent directories!
    for (int i = m_layoutNodes.size() - 1; i >= 0; --i) {
        if (m_layoutNodes[i].depth == 0) continue; // Skip root node

        if (m_layoutNodes[i].rect.contains(event->pos())) {
            m_hoveredIndex = i;
            break;
        }
    }

    if (m_hoveredIndex != prevHovered) {
        update(); // Request repaint to trigger glow border
        
        if (m_hoveredIndex != -1) {
            auto node = m_layoutNodes[m_hoveredIndex].node;
            emit nodeHovered(node);

            // Show interactive tooltip immediately
            double bytes = node->getSize();
            QStringList units = {"B", "KB", "MB", "GB", "TB"};
            int u = 0;
            while (bytes >= 1024.0 && u < units.size() - 1) {
                bytes /= 1024.0;
                u++;
            }
            QString sizeStr = QString::number(bytes, 'f', u > 0 ? 1 : 0) + " " + units[u];

            QString tip = QString("<b>Name:</b> %1<br/><b>Path:</b> %2<br/><b>Size:</b> %3<br/><b>Type:</b> %4")
                              .arg(node->getName())
                              .arg(node->getPath())
                              .arg(sizeStr)
                              .arg(node->isDirectory() ? "Directory" : "File (" + node->getExtension() + ")");
            
            QToolTip::showText(event->globalPosition().toPoint(), tip, this);
        } else {
            QToolTip::hideText();
        }
    }
}

void TreemapWidget::leaveEvent(QEvent* event) {
    Q_UNUSED(event);
    if (m_hoveredIndex != -1) {
        m_hoveredIndex = -1;
        update();
        QToolTip::hideText();
    }
}

void TreemapWidget::mousePressEvent(QMouseEvent* event) {
    if (m_hoveredIndex != -1) {
        auto node = m_layoutNodes[m_hoveredIndex].node;
        emit nodeClicked(node);
    }
}

void TreemapWidget::mouseDoubleClickEvent(QMouseEvent* event) {
    if (m_hoveredIndex != -1) {
        auto node = m_layoutNodes[m_hoveredIndex].node;
        emit nodeDoubleClicked(node);
    }
}
