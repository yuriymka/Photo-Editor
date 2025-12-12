#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMenuBar>
#include <QGraphicsRectItem>
#include <QFileDialog>
#include <QMessageBox>
#include <QListWidget>
#include <QFileInfo>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    
    // Initialize canvas components
    canvasView = new QGraphicsView(this);
    canvasScene = new QGraphicsScene(this);
    canvasView->setScene(canvasScene);
    setCentralWidget(canvasView);
    
    setupMenu();
    setupLayerDock();
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

void MainWindow::updateLayerList()
{
    layerList->clear();
    
    // Add background layer first
    QListWidgetItem *backgroundItem = new QListWidgetItem("Background");
    backgroundItem->setFlags(backgroundItem->flags() | Qt::ItemIsUserCheckable);
    backgroundItem->setCheckState(Qt::Checked);
    layerList->addItem(backgroundItem);
    
    // Add all other layers
    QList<QGraphicsItem*> items = canvasScene->items();
    for (int i = items.size() - 1; i >= 0; --i) {
        Layer *layer = qgraphicsitem_cast<Layer*>(items[i]);
        if (layer) {
            QListWidgetItem *item = new QListWidgetItem(layer->getName());
            item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
            item->setCheckState(layer->isVisible() ? Qt::Checked : Qt::Unchecked);
            layerList->addItem(item);
        }
    }
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

void MainWindow::addLayer(const QPixmap& pixmap, const QString& name)
{
    Layer *layer = new Layer(pixmap, name);
    layer->setPos(0, 0);
    canvasScene->addItem(layer);
    
    // Add layer to layer list
    QListWidgetItem *item = new QListWidgetItem(name);
    item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
    item->setCheckState(Qt::Checked);
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
                canvasScene->update(); // Force scene update
            }
        }
    }
}

void MainWindow::layerSelectionChanged()
{
    QListWidgetItem *currentItem = layerList->currentItem();
    if (currentItem) {
        int index = layerList->row(currentItem);
        if (index > 0) { // Skip background layer
            QList<QGraphicsItem*> items = canvasScene->items();
            if (index < items.size()) {
                Layer *layer = qgraphicsitem_cast<Layer*>(items[index - 1]);
                if (layer) {
                    layer->setSelected(true);
                }
            }
        }
    }
}

bool MainWindow::hasCanvas() const
{
    return !canvasScene->items().isEmpty();
}
