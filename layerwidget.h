#ifndef LAYERWIDGET_H
#define LAYERWIDGET_H

#include <QWidget>
#include <QListWidget>
#include <QSlider>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include "layer.h"
#include <QStyle>

class LayerWidget : public QWidget
{
    Q_OBJECT

public:
    explicit LayerWidget(QWidget *parent = nullptr);
    void addLayer(Layer* layer);
    void removeLayer(Layer* layer);
    void clearLayers();
    Layer* selectedLayer() const;

signals:
    void layerVisibilityChanged(Layer* layer, bool visible);
    void layerOpacityChanged(Layer* layer, qreal opacity);

private slots:
    void onLayerVisibilityChanged(int state);
    void onLayerOpacityChanged(int value);
    void onSelectionChanged();
    void onSelectedOpacitySliderChanged(int value);

private:
    QListWidget* layerList;
    QMap<Layer*, QWidget*> layerWidgets;
    QMap<Layer*, QSlider*> opacitySliders;
    QSlider* selectedOpacitySlider;
    QLabel* selectedOpacityLabel;
};

#endif // LAYERWIDGET_H 