#ifndef LAYER_H
#define LAYER_H

#include <QGraphicsItem>
#include <QGraphicsPixmapItem>
#include <QString>
#include <QGraphicsSceneMouseEvent>

class Layer : public QGraphicsPixmapItem
{
public:
    enum { Type = UserType + 1 };
    int type() const override { return Type; }

    Layer(const QPixmap& pixmap, const QString& name = "Layer", QGraphicsItem* parent = nullptr);
    QString getName() const { return layerName; }
    void setName(const QString& name) { layerName = name; }
    bool isVisible() const { return isVisible_; }
    void setVisible(bool visible);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    void hoverMoveEvent(QGraphicsSceneHoverEvent *event) override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

private:
    enum ResizeHandle {
        NoHandle,
        TopLeft,
        TopRight,
        BottomLeft,
        BottomRight
    };

    QString layerName;
    bool isVisible_;
    ResizeHandle currentHandle;
    QPointF lastPos;
    QRectF originalRect;
    bool isResizing;
    static const int HANDLE_SIZE = 8;
    
    ResizeHandle getHandleAt(const QPointF &pos);
    void updateCursor(ResizeHandle handle);
};

#endif // LAYER_H 