#ifndef LAYER_H
#define LAYER_H

#include <QGraphicsItem>
#include <QGraphicsPixmapItem>
#include <QString>
#include <QGraphicsSceneMouseEvent>
#include <opencv2/opencv.hpp>

class Layer : public QGraphicsPixmapItem
{
public:
    enum { Type = UserType + 1 };
    int type() const override { return Type; }

    Layer(const QPixmap& pixmap, const QString& name = "Layer", QGraphicsItem* parent = nullptr);
    ~Layer();
    QString getName() const { return layerName; }
    void setName(const QString& name) { layerName = name; }
    bool isVisible() const { return isVisible_; }
    void setVisible(bool visible);
    qreal opacity() const { return opacity_; }
    void setOpacity(qreal opacity);
    void setResizingEnabled(bool enabled);
    bool isResizingEnabled() const { return resizingEnabled_; }
    QSizeF getOriginalPixmapSize() const { return originalPixmapSize_; }
    void setSize(qreal width, qreal height);
    QSizeF targetSize() const { return targetSize_; }
    static bool s_renderingForExport;
    QString name() const { return layerName; }
    void setPixmap(const QPixmap& pixmap);

    // OpenCV conversion methods
    static cv::Mat pixmapToMat(const QPixmap& pixmap);
    static QPixmap matToPixmap(const cv::Mat& mat);
    void applyBlur(int ksize);
    void applySaturation(double factor);
    void applyBrightness(int value);
    void applyContrast(double factor);
    void applyGrayscale();
    void applyInvert();

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
    qreal opacity_;
    bool resizingEnabled_ = false;
    QSizeF originalPixmapSize_;
    QSizeF targetSize_;
    ResizeHandle currentHandle;
    QPointF lastPos;
    QRectF originalRect;
    bool isResizing;
    QPointF resizeStartPos;
    QSizeF resizeStartSize;
    static const int HANDLE_SIZE = 12;
    static const int HANDLE_MARGIN = 4;
    QPixmap originalPixmap_;  // Store the original pixmap
    
    ResizeHandle getHandleAt(const QPointF &pos);
    void updateCursor(ResizeHandle handle);
    QRectF getHandleRect(const QPointF &center) const;
};

#endif // LAYER_H 