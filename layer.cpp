#include "layer.h"

Layer::Layer(const QPixmap& pixmap, const QString& name, QGraphicsItem* parent)
    : QGraphicsPixmapItem(pixmap, parent)
    , layerName(name)
    , isVisible_(true)
{
    setFlag(QGraphicsItem::ItemIsMovable);
    setFlag(QGraphicsItem::ItemIsSelectable);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges);
}

void Layer::setVisible(bool visible)
{
    isVisible_ = visible;
    QGraphicsPixmapItem::setVisible(visible);
} 