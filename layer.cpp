#include "layer.h"
#include <QPainter>
#include <QCursor>
#include <QStyle>
#include <QStyleOptionGraphicsItem>

bool Layer::s_renderingForExport = false;

Layer::Layer(const QPixmap& pixmap, const QString& name, QGraphicsItem* parent)
    : QGraphicsPixmapItem(pixmap, parent)
    , layerName(name)
    , isVisible_(true)
    , opacity_(1.0)
    , currentHandle(NoHandle)
    , isResizing(false)
{
    setFlag(QGraphicsItem::ItemIsMovable);
    setFlag(QGraphicsItem::ItemIsSelectable);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges);
    setAcceptHoverEvents(true);
    QGraphicsPixmapItem::setOpacity(opacity_);
}

void Layer::setVisible(bool visible)
{
    isVisible_ = visible;
    QGraphicsPixmapItem::setVisible(visible);
}

void Layer::setOpacity(qreal opacity)
{
    opacity_ = qBound(0.0, opacity, 1.0);
    QGraphicsPixmapItem::setOpacity(opacity_);
    update();
}

void Layer::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        currentHandle = getHandleAt(event->pos());
        if (currentHandle != NoHandle) {
            isResizing = true;
            lastPos = event->pos();
            originalRect = boundingRect();
            event->accept();
            return;
        }
    }
    QGraphicsPixmapItem::mousePressEvent(event);
}

void Layer::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (isResizing) {
        QPointF delta = event->pos() - lastPos;
        QRectF newRect = originalRect;
        
        switch (currentHandle) {
            case TopLeft:
                newRect.setTopLeft(newRect.topLeft() + delta);
                break;
            case TopRight:
                newRect.setTopRight(newRect.topRight() + delta);
                break;
            case BottomLeft:
                newRect.setBottomLeft(newRect.bottomLeft() + delta);
                break;
            case BottomRight:
                newRect.setBottomRight(newRect.bottomRight() + delta);
                break;
            default:
                break;
        }
        
        // Ensure minimum size
        if (newRect.width() >= 10 && newRect.height() >= 10) {
            setTransform(QTransform::fromScale(
                newRect.width() / pixmap().width(),
                newRect.height() / pixmap().height()
            ));
            setPos(newRect.topLeft());
            update();
        }
        
        event->accept();
        return;
    }
    QGraphicsPixmapItem::mouseMoveEvent(event);
}

void Layer::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (isResizing) {
        isResizing = false;
        currentHandle = NoHandle;
        event->accept();
        return;
    }
    QGraphicsPixmapItem::mouseReleaseEvent(event);
}

void Layer::hoverMoveEvent(QGraphicsSceneHoverEvent *event)
{
    ResizeHandle handle = getHandleAt(event->pos());
    updateCursor(handle);
    QGraphicsPixmapItem::hoverMoveEvent(event);
}

void Layer::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    QStyleOptionGraphicsItem opt(*option);
    opt.state &= ~QStyle::State_Selected;
    QGraphicsPixmapItem::paint(painter, &opt, widget);

    // Only draw selection visuals if not exporting and selected
    if (!s_renderingForExport && isSelected()) {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);

        QRectF rect = pixmap().rect();
        QPen pen(Qt::blue, 1, Qt::DashLine);
        painter->setPen(pen);
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(rect);

        painter->setPen(QPen(Qt::blue, 1));
        painter->setBrush(Qt::white);

        const int HANDLE_SIZE = 12;
        const int HANDLE_MARGIN = 4;
        QPointF topLeft = rect.topLeft() + QPointF(HANDLE_MARGIN, HANDLE_MARGIN);
        QPointF topRight = rect.topRight() + QPointF(-HANDLE_MARGIN, HANDLE_MARGIN);
        QPointF bottomLeft = rect.bottomLeft() + QPointF(HANDLE_MARGIN, -HANDLE_MARGIN);
        QPointF bottomRight = rect.bottomRight() + QPointF(-HANDLE_MARGIN, -HANDLE_MARGIN);

        auto drawHandle = [&](const QPointF& center) {
            painter->drawRect(QRectF(center.x() - HANDLE_SIZE/2, center.y() - HANDLE_SIZE/2, HANDLE_SIZE, HANDLE_SIZE));
        };
        drawHandle(topLeft);
        drawHandle(topRight);
        drawHandle(bottomLeft);
        drawHandle(bottomRight);

        painter->restore();
    }
}

QRectF Layer::getHandleRect(const QPointF &center) const
{
    return QRectF(center.x() - HANDLE_SIZE/2, center.y() - HANDLE_SIZE/2,
                 HANDLE_SIZE, HANDLE_SIZE);
}

Layer::ResizeHandle Layer::getHandleAt(const QPointF &pos)
{
    QRectF rect = boundingRect();
    
    // Check each corner with margin
    QPointF topLeft = rect.topLeft() + QPointF(HANDLE_MARGIN, HANDLE_MARGIN);
    QPointF topRight = rect.topRight() + QPointF(-HANDLE_MARGIN, HANDLE_MARGIN);
    QPointF bottomLeft = rect.bottomLeft() + QPointF(HANDLE_MARGIN, -HANDLE_MARGIN);
    QPointF bottomRight = rect.bottomRight() + QPointF(-HANDLE_MARGIN, -HANDLE_MARGIN);
    
    if (getHandleRect(topLeft).contains(pos))
        return TopLeft;
    if (getHandleRect(topRight).contains(pos))
        return TopRight;
    if (getHandleRect(bottomLeft).contains(pos))
        return BottomLeft;
    if (getHandleRect(bottomRight).contains(pos))
        return BottomRight;
    
    return NoHandle;
}

void Layer::updateCursor(ResizeHandle handle)
{
    switch (handle) {
        case TopLeft:
        case BottomRight:
            setCursor(Qt::SizeFDiagCursor);
            break;
        case TopRight:
        case BottomLeft:
            setCursor(Qt::SizeBDiagCursor);
            break;
        default:
            setCursor(Qt::ArrowCursor);
            break;
    }
} 