#include "layer.h"
#include <QPainter>
#include <QCursor>
#include <QStyle>
#include <QStyleOptionGraphicsItem>
#include <QGraphicsSceneHoverEvent>

bool Layer::s_renderingForExport = false;

cv::Mat Layer::pixmapToMat(const QPixmap& pixmap)
{
    QImage image = pixmap.toImage().convertToFormat(QImage::Format_ARGB32);
    cv::Mat mat(image.height(), image.width(), CV_8UC4, (void*)image.bits(), image.bytesPerLine());
    return mat.clone(); // clone to detach from QImage memory
}

QPixmap Layer::matToPixmap(const cv::Mat& mat)
{
    if (mat.channels() == 4) {
        // No color conversion needed, just wrap the data
        QImage image(mat.data, mat.cols, mat.rows, mat.step, QImage::Format_ARGB32);
        return QPixmap::fromImage(image.copy());
    } else {
        cv::Mat rgb;
        cv::cvtColor(mat, rgb, cv::COLOR_BGR2RGB);
        QImage image(rgb.data, rgb.cols, rgb.rows, rgb.step, QImage::Format_RGB888);
        return QPixmap::fromImage(image.copy());
    }
}

Layer::Layer(const QPixmap& pixmap, const QString& name, QGraphicsItem* parent)
    : QGraphicsPixmapItem(pixmap, parent)
    , layerName(name)
    , isVisible_(true)
    , opacity_(1.0)
    , resizingEnabled_(false)
    , originalPixmapSize_(pixmap.size())
    , targetSize_(pixmap.size())
    , currentHandle(NoHandle)
    , isResizing(false)
    , originalPixmap_(pixmap)
{
    setFlag(QGraphicsItem::ItemIsMovable);
    setFlag(QGraphicsItem::ItemIsSelectable);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges);
    setAcceptHoverEvents(true);
    QGraphicsPixmapItem::setOpacity(opacity_);
    
    // Convert to ARGB32 format to ensure transparency support
    QImage image = pixmap.toImage().convertToFormat(QImage::Format_ARGB32);
    
    // Create a new image with transparency
    QImage transparentImage(image.size(), QImage::Format_ARGB32);
    transparentImage.fill(Qt::transparent);
    
    // Copy the original image onto the transparent image
    QPainter painter(&transparentImage);
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    painter.drawImage(0, 0, image);
    painter.end();
    
    QPixmap transparentPixmap = QPixmap::fromImage(transparentImage);
    QGraphicsPixmapItem::setPixmap(transparentPixmap);
    originalPixmap_ = transparentPixmap;
}

Layer::~Layer()
{
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

void Layer::setResizingEnabled(bool enabled)
{
    resizingEnabled_ = enabled;
    if (!enabled) {
        currentHandle = NoHandle;
        updateCursor(NoHandle);
    }
    update();
}

void Layer::setSize(qreal width, qreal height)
{
    if (width <= 0 || height <= 0) return;
    
    targetSize_ = QSizeF(width, height);
    QPixmap scaled = pixmap().scaled(targetSize_.toSize(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    setPixmap(scaled);
    update();
}

void Layer::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        if (resizingEnabled_) {
            currentHandle = getHandleAt(event->pos());
            if (currentHandle != NoHandle) {
                isResizing = true;
                resizeStartPos = event->pos();
                resizeStartSize = targetSize_;
                originalRect = boundingRect();
                event->accept();
                return;
            }
        }
    }
    QGraphicsPixmapItem::mousePressEvent(event);
}

void Layer::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    if (isResizing && currentHandle != NoHandle) {
        QPointF delta = event->pos() - resizeStartPos;
        QSizeF newSize = resizeStartSize;
        
        switch (currentHandle) {
            case TopLeft:
                newSize.setWidth(resizeStartSize.width() - delta.x());
                newSize.setHeight(resizeStartSize.height() - delta.y());
                break;
            case TopRight:
                newSize.setWidth(resizeStartSize.width() + delta.x());
                newSize.setHeight(resizeStartSize.height() - delta.y());
                break;
            case BottomLeft:
                newSize.setWidth(resizeStartSize.width() - delta.x());
                newSize.setHeight(resizeStartSize.height() + delta.y());
                break;
            case BottomRight:
                newSize.setWidth(resizeStartSize.width() + delta.x());
                newSize.setHeight(resizeStartSize.height() + delta.y());
                break;
            default:
                break;
        }
        
        // Maintain aspect ratio if shift is pressed
        if (event->modifiers() & Qt::ShiftModifier) {
            qreal aspectRatio = resizeStartSize.width() / resizeStartSize.height();
            if (newSize.width() / newSize.height() > aspectRatio) {
                newSize.setWidth(newSize.height() * aspectRatio);
            } else {
                newSize.setHeight(newSize.width() / aspectRatio);
            }
        }
        
        setSize(newSize.width(), newSize.height());
        event->accept();
        return;
    }
    QGraphicsPixmapItem::mouseMoveEvent(event);
}

void Layer::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && isResizing) {
        isResizing = false;
        currentHandle = NoHandle;
        event->accept();
        return;
    }
    QGraphicsPixmapItem::mouseReleaseEvent(event);
}

void Layer::hoverMoveEvent(QGraphicsSceneHoverEvent* event)
{
    if (resizingEnabled_) {
        ResizeHandle handle = getHandleAt(event->pos());
        updateCursor(handle);
    }
    QGraphicsPixmapItem::hoverMoveEvent(event);
}

void Layer::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    if (!s_renderingForExport) {
        // Enable alpha blending
        painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
        QGraphicsPixmapItem::paint(painter, option, widget);
        
        if (isSelected() && resizingEnabled_) {
            painter->setPen(QPen(Qt::blue, 1, Qt::DashLine));
            painter->drawRect(boundingRect());
            
            // Draw resize handles
            painter->setPen(QPen(Qt::blue, 1));
            painter->setBrush(Qt::white);
            
            QRectF rect = boundingRect();
            QPointF corners[] = {
                rect.topLeft(),
                rect.topRight(),
                rect.bottomLeft(),
                rect.bottomRight()
            };
            
            for (const QPointF& corner : corners) {
                painter->drawRect(getHandleRect(corner));
            }
        }
    } else {
        QGraphicsPixmapItem::paint(painter, option, widget);
    }
}

QRectF Layer::getHandleRect(const QPointF& center) const
{
    return QRectF(center.x() - HANDLE_SIZE/2,
                 center.y() - HANDLE_SIZE/2,
                 HANDLE_SIZE,
                 HANDLE_SIZE);
}

Layer::ResizeHandle Layer::getHandleAt(const QPointF& pos)
{
    if (!resizingEnabled_) return NoHandle;
    
    QRectF rect = boundingRect();
    QPointF corners[] = {
        rect.topLeft(),
        rect.topRight(),
        rect.bottomLeft(),
        rect.bottomRight()
    };
    
    for (int i = 0; i < 4; ++i) {
        if (getHandleRect(corners[i]).contains(pos)) {
            return static_cast<ResizeHandle>(i + 1);
        }
    }
    
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

void Layer::applyBlur(int ksize)
{
    QPixmap px = pixmap();
    cv::Mat mat = pixmapToMat(px);
    if (mat.empty()) return;
    cv::Mat blurred;
    cv::blur(mat, blurred, cv::Size(ksize, ksize));
    setPixmap(matToPixmap(blurred));
    originalPixmapSize_ = QSizeF(pixmap().width(), pixmap().height());
    targetSize_ = originalPixmapSize_;
}

void Layer::applySaturation(double factor)
{
    QPixmap px = pixmap();
    cv::Mat mat = pixmapToMat(px);
    if (mat.empty()) return;

    // Split color and alpha
    std::vector<cv::Mat> bgraChannels;
    cv::split(mat, bgraChannels);
    cv::Mat bgr, alpha;
    cv::merge(std::vector<cv::Mat>{bgraChannels[0], bgraChannels[1], bgraChannels[2]}, bgr);
    alpha = bgraChannels[3];

    // Convert to HSV, adjust saturation
    cv::Mat hsv;
    cv::cvtColor(bgr, hsv, cv::COLOR_BGR2HSV);
    std::vector<cv::Mat> hsvChannels;
    cv::split(hsv, hsvChannels);
    hsvChannels[1].convertTo(hsvChannels[1], -1, factor, 0); // scale saturation
    cv::merge(hsvChannels, hsv);
    cv::Mat bgrResult;
    cv::cvtColor(hsv, bgrResult, cv::COLOR_HSV2BGR);

    // Merge color and alpha back
    std::vector<cv::Mat> bgraResultChannels;
    cv::split(bgrResult, bgraResultChannels);
    bgraResultChannels.push_back(alpha);
    cv::Mat bgraResult;
    cv::merge(bgraResultChannels, bgraResult);

    setPixmap(matToPixmap(bgraResult));
    originalPixmapSize_ = QSizeF(pixmap().width(), pixmap().height());
    targetSize_ = originalPixmapSize_;
}

void Layer::applyBrightness(int value)
{
    // Always start from the original image
    cv::Mat mat = pixmapToMat(originalPixmap_);
    if (mat.empty()) return;

    // Split color and alpha
    std::vector<cv::Mat> bgraChannels;
    cv::split(mat, bgraChannels);
    cv::Mat bgr, alpha;
    cv::merge(std::vector<cv::Mat>{bgraChannels[0], bgraChannels[1], bgraChannels[2]}, bgr);
    alpha = bgraChannels[3];

    // Convert to HSV
    cv::Mat hsv;
    cv::cvtColor(bgr, hsv, cv::COLOR_BGR2HSV);

    // Split HSV channels
    std::vector<cv::Mat> hsvChannels;
    cv::split(hsv, hsvChannels);

    // Convert to float for better precision
    cv::Mat floatV;
    hsvChannels[2].convertTo(floatV, CV_32F);

    // Apply brightness to Value channel
    floatV += value;

    // Clamp values to valid range
    cv::threshold(floatV, floatV, 255, 255, cv::THRESH_TRUNC);
    cv::threshold(floatV, floatV, 0, 0, cv::THRESH_TOZERO);

    // Convert back to 8-bit
    cv::Mat resultV;
    floatV.convertTo(resultV, CV_8U);

    // Replace Value channel
    hsvChannels[2] = resultV;

    // Merge HSV channels
    cv::Mat resultHsv;
    cv::merge(hsvChannels, resultHsv);

    // Convert back to BGR
    cv::Mat resultBgr;
    cv::cvtColor(resultHsv, resultBgr, cv::COLOR_HSV2BGR);

    // Merge color and alpha back
    std::vector<cv::Mat> bgraResultChannels;
    cv::split(resultBgr, bgraResultChannels);
    bgraResultChannels.push_back(alpha);
    cv::Mat bgraResult;
    cv::merge(bgraResultChannels, bgraResult);

    setPixmap(matToPixmap(bgraResult));
    originalPixmapSize_ = QSizeF(pixmap().width(), pixmap().height());
    targetSize_ = originalPixmapSize_;
}

void Layer::applyContrast(double factor)
{
    // Always start from the original image
    cv::Mat mat = pixmapToMat(originalPixmap_);
    if (mat.empty()) return;

    // Split color and alpha
    std::vector<cv::Mat> bgraChannels;
    cv::split(mat, bgraChannels);
    cv::Mat bgr, alpha;
    cv::merge(std::vector<cv::Mat>{bgraChannels[0], bgraChannels[1], bgraChannels[2]}, bgr);
    alpha = bgraChannels[3];

    // Convert to float for better precision
    cv::Mat floatBgr;
    bgr.convertTo(floatBgr, CV_32F);

    // Apply contrast
    floatBgr = (floatBgr - 128) * factor + 128;

    // Clamp values to valid range
    cv::threshold(floatBgr, floatBgr, 255, 255, cv::THRESH_TRUNC);
    cv::threshold(floatBgr, floatBgr, 0, 0, cv::THRESH_TOZERO);

    // Convert back to 8-bit
    cv::Mat resultBgr;
    floatBgr.convertTo(resultBgr, CV_8U);

    // Merge color and alpha back
    std::vector<cv::Mat> bgraResultChannels;
    cv::split(resultBgr, bgraResultChannels);
    bgraResultChannels.push_back(alpha);
    cv::Mat bgraResult;
    cv::merge(bgraResultChannels, bgraResult);

    setPixmap(matToPixmap(bgraResult));
    originalPixmapSize_ = QSizeF(pixmap().width(), pixmap().height());
    targetSize_ = originalPixmapSize_;
}

void Layer::applyGrayscale()
{
    QPixmap px = pixmap();
    cv::Mat mat = pixmapToMat(px);
    if (mat.empty()) return;

    // Split color and alpha
    std::vector<cv::Mat> bgraChannels;
    cv::split(mat, bgraChannels);
    cv::Mat bgr, alpha;
    cv::merge(std::vector<cv::Mat>{bgraChannels[0], bgraChannels[1], bgraChannels[2]}, bgr);
    alpha = bgraChannels[3];

    // Convert to grayscale
    cv::Mat gray;
    cv::cvtColor(bgr, gray, cv::COLOR_BGR2GRAY);
    cv::cvtColor(gray, bgr, cv::COLOR_GRAY2BGR);

    // Merge color and alpha back
    std::vector<cv::Mat> bgraResultChannels;
    cv::split(bgr, bgraResultChannels);
    bgraResultChannels.push_back(alpha);
    cv::Mat bgraResult;
    cv::merge(bgraResultChannels, bgraResult);

    setPixmap(matToPixmap(bgraResult));
    originalPixmapSize_ = QSizeF(pixmap().width(), pixmap().height());
    targetSize_ = originalPixmapSize_;
}

void Layer::applyInvert()
{
    QPixmap px = pixmap();
    cv::Mat mat = pixmapToMat(px);
    if (mat.empty()) return;

    // Split color and alpha
    std::vector<cv::Mat> bgraChannels;
    cv::split(mat, bgraChannels);
    cv::Mat bgr, alpha;
    cv::merge(std::vector<cv::Mat>{bgraChannels[0], bgraChannels[1], bgraChannels[2]}, bgr);
    alpha = bgraChannels[3];

    // Invert colors
    cv::bitwise_not(bgr, bgr);

    // Merge color and alpha back
    std::vector<cv::Mat> bgraResultChannels;
    cv::split(bgr, bgraResultChannels);
    bgraResultChannels.push_back(alpha);
    cv::Mat bgraResult;
    cv::merge(bgraResultChannels, bgraResult);

    setPixmap(matToPixmap(bgraResult));
    originalPixmapSize_ = QSizeF(pixmap().width(), pixmap().height());
    targetSize_ = originalPixmapSize_;
}

void Layer::setPixmap(const QPixmap& pixmap)
{
    // Convert to ARGB32 format to ensure transparency support
    QImage image = pixmap.toImage().convertToFormat(QImage::Format_ARGB32);
    QPixmap transparentPixmap = QPixmap::fromImage(image);
    QGraphicsPixmapItem::setPixmap(transparentPixmap);
    originalPixmap_ = transparentPixmap;
    update();
} 