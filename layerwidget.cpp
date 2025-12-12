#include "layerwidget.h"
#include <QCheckBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QSlider>

LayerWidget::LayerWidget(QWidget *parent)
    : QWidget(parent)
    , layerList(new QListWidget(this))
    , selectedOpacitySlider(new QSlider(Qt::Horizontal, this))
    , selectedOpacityLabel(new QLabel("Opacity:", this))
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(layerList, 1);

    // Opacity slider for selected layer
    QHBoxLayout* opacityLayout = new QHBoxLayout;
    opacityLayout->addWidget(selectedOpacityLabel);
    selectedOpacitySlider->setRange(0, 100);
    selectedOpacitySlider->setValue(100);
    opacityLayout->addWidget(selectedOpacitySlider);
    mainLayout->addLayout(opacityLayout);

    setLayout(mainLayout);

    connect(layerList, &QListWidget::itemSelectionChanged, this, &LayerWidget::onSelectionChanged);
    connect(selectedOpacitySlider, &QSlider::valueChanged, this, &LayerWidget::onSelectedOpacitySliderChanged);
}

void LayerWidget::addLayer(Layer* layer)
{
    QWidget* layerWidget = new QWidget;
    QHBoxLayout* layout = new QHBoxLayout(layerWidget);
    
    QCheckBox* visibilityCheck = new QCheckBox;
    visibilityCheck->setChecked(layer->isVisible());
    visibilityCheck->setProperty("layer", QVariant::fromValue(layer));
    connect(visibilityCheck, &QCheckBox::stateChanged, this, &LayerWidget::onLayerVisibilityChanged);
    
    QLabel* nameLabel = new QLabel(layer->getName());
    
    layout->addWidget(visibilityCheck);
    layout->addWidget(nameLabel);
    layout->setContentsMargins(2, 2, 2, 2);
    
    QListWidgetItem* item = new QListWidgetItem;
    item->setSizeHint(layerWidget->sizeHint());
    
    layerList->insertItem(0, item);
    layerList->setItemWidget(item, layerWidget);
    
    layerWidgets[layer] = layerWidget;
}

void LayerWidget::removeLayer(Layer* layer)
{
    if (layerWidgets.contains(layer)) {
        QWidget* widget = layerWidgets[layer];
        QListWidgetItem* item = layerList->itemAt(widget->pos());
        if (item) {
            layerList->takeItem(layerList->row(item));
        }
        delete widget;
        layerWidgets.remove(layer);
    }
}

void LayerWidget::clearLayers()
{
    layerList->clear();
    layerWidgets.clear();
}

Layer* LayerWidget::selectedLayer() const
{
    QListWidgetItem* item = layerList->currentItem();
    if (!item) return nullptr;
    int row = layerList->row(item);
    // The layers are inserted at the top, so reverse order
    QList<Layer*> layers = layerWidgets.keys();
    if (row >= 0 && row < layers.size())
        return layers[layers.size() - 1 - row];
    return nullptr;
}

void LayerWidget::onLayerVisibilityChanged(int state)
{
    QCheckBox* checkBox = qobject_cast<QCheckBox*>(sender());
    if (checkBox) {
        Layer* layer = checkBox->property("layer").value<Layer*>();
        if (layer) {
            emit layerVisibilityChanged(layer, state == Qt::Checked);
        }
    }
}

void LayerWidget::onLayerOpacityChanged(int value)
{
    QSlider* slider = qobject_cast<QSlider*>(sender());
    if (slider) {
        Layer* layer = slider->property("layer").value<Layer*>();
        if (layer) {
            emit layerOpacityChanged(layer, value / 100.0);
        }
    }
}

void LayerWidget::onSelectionChanged()
{
    Layer* layer = selectedLayer();
    if (layer) {
        selectedOpacitySlider->blockSignals(true);
        selectedOpacitySlider->setValue(layer->opacity() * 100);
        selectedOpacitySlider->blockSignals(false);
        selectedOpacitySlider->setEnabled(true);
    } else {
        selectedOpacitySlider->setEnabled(false);
    }
}

void LayerWidget::onSelectedOpacitySliderChanged(int value)
{
    Layer* layer = selectedLayer();
    if (layer) {
        emit layerOpacityChanged(layer, value / 100.0);
    }
} 