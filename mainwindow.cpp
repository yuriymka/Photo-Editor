#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMenuBar>
#include <QGraphicsRectItem>
#include <QFileDialog>
#include <QMessageBox>
#include <QListWidget>
#include <QFileInfo>
#include <QToolBar>
#include <QSlider>
#include <QLabel>
#include <QHBoxLayout>
#include <QWidget>
#include <QVariant>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Initialize canvas components
    canvasView = new QGraphicsView(this);
    canvasScene = new QGraphicsScene(this);
    canvasView->setScene(canvasScene);

    // Opacity slider and label in a compact widget above the canvas
    globalOpacitySlider = new QSlider(Qt::Horizontal, this);
    globalOpacitySlider->setRange(0, 100);
    globalOpacitySlider->setValue(100);
    globalOpacitySlider->setFixedWidth(200);
    globalOpacityLabel = new QLabel("Opacity:", this);
    QHBoxLayout* opacityLayout = new QHBoxLayout;
    opacityLayout->addWidget(globalOpacityLabel);
    opacityLayout->addWidget(globalOpacitySlider);
    opacityLayout->addStretch();
    QWidget* opacityWidget = new QWidget(this);
    opacityWidget->setLayout(opacityLayout);

    // Main layout for central widget
    QVBoxLayout* centralLayout = new QVBoxLayout;
    centralLayout->addWidget(opacityWidget);
    centralLayout->addWidget(canvasView);
    QWidget* centralWidget = new QWidget(this);
    centralWidget->setLayout(centralLayout);
    setCentralWidget(centralWidget);

    setupMenu();
    setupLayerDock();

    connect(globalOpacitySlider, &QSlider::valueChanged, this, &MainWindow::onGlobalOpacitySliderChanged);
    connect(canvasScene, &QGraphicsScene::selectionChanged, this, &MainWindow::onSceneSelectionChanged);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setupMenu()
{
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));

    QAction *newCanvasAction = new QAction(tr("&New Canvas"), this);
    newCanvasAction->setShortcut(QKeySequence::New);
    connect(newCanvasAction, &QAction::triggered, this, &MainWindow::createNewCanvas);
    fileMenu->addAction(newCanvasAction);

    QAction *openImageAction = new QAction(tr("&Open Image"), this);
    openImageAction->setShortcut(QKeySequence::Open);
    connect(openImageAction, &QAction::triggered, this, &MainWindow::openImage);
    fileMenu->addAction(openImageAction);

    QAction *saveImageAction = new QAction(tr("&Save Image"), this);
    saveImageAction->setShortcut(QKeySequence::Save);
    connect(saveImageAction, &QAction::triggered, this, &MainWindow::saveImage);
    fileMenu->addAction(saveImageAction);
}

void MainWindow::setupLayerDock()
{
    layerDock = new QDockWidget(tr("Layers"), this);
    layerDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

    layerList = new QListWidget(layerDock);
    layerList->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(layerList, &QListWidget::itemChanged, this, &MainWindow::toggleLayerVisibility);
    connect(layerList, &QListWidget::itemSelectionChanged, this, &MainWindow::layerSelectionChanged);

    layerDock->setWidget(layerList);
    addDockWidget(Qt::RightDockWidgetArea, layerDock);
}

void MainWindow::createNewCanvas()
{
    CanvasDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        int width = dialog.getWidth();
        int height = dialog.getHeight();

        // Clear existing scene and layer list
        canvasScene->clear();
        layerList->clear();

        // Create new canvas with white background
        QGraphicsRectItem *canvas = new QGraphicsRectItem(0, 0, width, height);
        canvas->setBrush(Qt::white);
        canvas->setPen(QPen(Qt::black));
        canvasScene->addItem(canvas);

        // Add background layer to layer list
        QListWidgetItem *backgroundItem = new QListWidgetItem("Background");
        backgroundItem->setFlags(backgroundItem->flags() | Qt::ItemIsUserCheckable);
        backgroundItem->setCheckState(Qt::Checked);
        layerList->addItem(backgroundItem);

        // Adjust view to fit the canvas
        canvasView->fitInView(canvas, Qt::KeepAspectRatio);
    }
}

void MainWindow::openImage()
{
    if (!hasCanvas()) {
        QMessageBox::warning(this, tr("Warning"),
            tr("Please create a canvas first before adding images."));
        return;
    }

    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Open Image"), "",
        tr("Image Files (*.png *.jpg *.jpeg *.bmp *.gif)"));

    if (!fileName.isEmpty()) {
        QPixmap pixmap(fileName);
        if (pixmap.isNull()) {
            QMessageBox::warning(this, tr("Warning"),
                tr("Failed to load the image."));
            return;
        }

        // Create a layer name from the file name
        QString layerName = QFileInfo(fileName).baseName();
        addLayer(pixmap, layerName);
    }
}

void MainWindow::saveImage()
{
    if (!hasCanvas()) {
        QMessageBox::warning(this, tr("Warning"),
            tr("Please create a canvas first before saving."));
        return;
    }

    QString fileName = QFileDialog::getSaveFileName(this,
        tr("Save Image"), "",
        tr("PNG Files (*.png);;JPEG Files (*.jpg);;BMP Files (*.bmp)"));

    if (!fileName.isEmpty()) {
        // Find the background/canvas item (assumed to be the first QGraphicsRectItem)
        QGraphicsRectItem* canvas = nullptr;
        for (QGraphicsItem* item : canvasScene->items(Qt::AscendingOrder)) {
            canvas = qgraphicsitem_cast<QGraphicsRectItem*>(item);
            if (canvas) break;
        }
        if (!canvas) {
            QMessageBox::warning(this, tr("Warning"), tr("No canvas found."));
            return;
        }

        QRectF canvasRect = canvas->rect().translated(canvas->pos());
        QImage image(canvasRect.size().toSize(), QImage::Format_ARGB32);
        image.fill(Qt::white);

        // Deselect all items
        QList<QGraphicsItem*> selectedItems = canvasScene->selectedItems();
        for (QGraphicsItem* item : selectedItems)
            item->setSelected(false);
        canvasScene->clearSelection(); // Extra safety

        // Force a scene update to ensure selection visuals are gone
        canvasScene->update();
        qApp->processEvents();

        // Suppress selection visuals during export
        Layer::s_renderingForExport = true;
        QPainter painter(&image);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);
        // Render only the canvas area
        canvasScene->render(&painter, QRectF(QPointF(0, 0), canvasRect.size()), canvasRect);
        Layer::s_renderingForExport = false;

        // Restore selections
        for (QGraphicsItem* item : selectedItems)
            item->setSelected(true);

        if (!image.save(fileName)) {
            QMessageBox::warning(this, tr("Warning"),
                tr("Failed to save the image."));
        }
    }
}

void MainWindow::addLayer(const QPixmap& pixmap, const QString& name)
{
    Layer *layer = new Layer(pixmap, name);
    layer->setPos(0, 0);
    canvasScene->addItem(layer);
    
    // Add layer to layer list
    QListWidgetItem *item = new QListWidgetItem(name);
    item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
    item->setCheckState(Qt::Checked);
    // Store Layer* in item for reliable selection sync
    item->setData(Qt::UserRole, QVariant::fromValue(static_cast<void*>(layer)));
    layerList->addItem(item);
    
    // Select the new layer
    layerList->setCurrentItem(item);
}

void MainWindow::toggleLayerVisibility(QListWidgetItem* item)
{
    int index = layerList->row(item);
    if (index > 0) { // Skip background layer
        QList<QGraphicsItem*> items = canvasScene->items();
        if (index < items.size()) {
            Layer *layer = qgraphicsitem_cast<Layer*>(items[index - 1]);
            if (layer) {
                layer->setVisible(item->checkState() == Qt::Checked);
            }
        }
    }
}

void MainWindow::layerSelectionChanged()
{
    QListWidgetItem *currentItem = layerList->currentItem();
    if (currentItem) {
        Layer* layer = static_cast<Layer*>(currentItem->data(Qt::UserRole).value<void*>());
        if (layer) {
            layer->setSelected(true);
            globalOpacitySlider->blockSignals(true);
            globalOpacitySlider->setValue(layer->opacity() * 100);
            globalOpacitySlider->blockSignals(false);
        }
    }
}

bool MainWindow::hasCanvas() const
{
    return !canvasScene->items().isEmpty();
}

void MainWindow::onGlobalOpacitySliderChanged(int value)
{
    QListWidgetItem *currentItem = layerList->currentItem();
    if (currentItem) {
        Layer* layer = static_cast<Layer*>(currentItem->data(Qt::UserRole).value<void*>());
        if (layer) {
            layer->setOpacity(value / 100.0);
        }
    }
}

void MainWindow::onSceneSelectionChanged()
{
    // Get the selected Layer in the scene
    QList<QGraphicsItem*> selectedItems = canvasScene->selectedItems();
    if (selectedItems.isEmpty()) {
        layerList->clearSelection();
        return;
    }
    Layer* selectedLayer = qgraphicsitem_cast<Layer*>(selectedItems.first());
    if (!selectedLayer)
        return;

    // Find the corresponding item in the layerList by pointer and select it
    for (int i = 0; i < layerList->count(); ++i) {
        QListWidgetItem* item = layerList->item(i);
        Layer* itemLayer = static_cast<Layer*>(item->data(Qt::UserRole).value<void*>());
        if (itemLayer == selectedLayer) {
            layerList->setCurrentItem(item);
            break;
        }
    }
}
