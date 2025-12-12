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
#include <QToolButton>
#include <QButtonGroup>
#include <QIcon>
#include <QSpinBox>
#include <QTimer>
#include <QPushButton>
#include <QCheckBox>
#include <QInputDialog>
#include <QColorDialog>
#include <QMouseEvent>
#include <opencv2/opencv.hpp>
#include <QDateTime>
#include <QTextStream>
#include <functional>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , currentTool(None)
    , brushSize(5)
    , brushColor(Qt::black)
    , isDrawing(false)
    , currentPixmap(nullptr)
    , currentPainter(nullptr)
{
    ui->setupUi(this);

    // Initialize canvas components
    canvasView = new QGraphicsView(this);
    canvasScene = new QGraphicsScene(this);
    canvasView->setScene(canvasScene);
    
    // Connect to scene's item change events to track movement
    connect(canvasScene, &QGraphicsScene::changed, this, [this](const QList<QRectF>&) {
        if (currentTool == Move) {
            QList<QGraphicsItem*> selectedItems = canvasScene->selectedItems();
            for (QGraphicsItem* item : selectedItems) {
                Layer* layer = qgraphicsitem_cast<Layer*>(item);
                if (layer) {
                    QPointF newPos = item->pos();
                    static QPointF lastPos = newPos;
                    if (newPos != lastPos) {
                        logMovement(layer->getName(), lastPos, newPos);
                        lastPos = newPos;
                    }
                }
            }
        }
    });
    
    // Install event filter for brush tool
    canvasView->viewport()->installEventFilter(this);

    // Opacity slider and label in a compact widget above the canvas
    globalOpacitySlider = new QSlider(Qt::Horizontal, this);
    globalOpacitySlider->setRange(0, 100);
    globalOpacitySlider->setValue(100);
    globalOpacitySlider->setFixedWidth(200);
    globalOpacityLabel = new QLabel("Opacity:", this);

    // Tools section
    moveToolButton = new QToolButton(this);
    moveToolButton->setText("Move");
    moveToolButton->setCheckable(true);
    moveToolButton->setIcon(QIcon(":/icons/move.png"));
    resizeToolButton = new QToolButton(this);
    resizeToolButton->setText("Resize");
    resizeToolButton->setCheckable(true);
    resizeToolButton->setIcon(QIcon(":/icons/resize.png"));
    rotateToolButton = new QToolButton(this);
    rotateToolButton->setText("Rotate");
    rotateToolButton->setCheckable(true);
    rotateToolButton->setIcon(QIcon(":/icons/rotate.png"));
    toolButtonGroup = new QButtonGroup(this);
    toolButtonGroup->setExclusive(true);
    toolButtonGroup->addButton(moveToolButton, Move);
    toolButtonGroup->addButton(resizeToolButton, Resize);
    toolButtonGroup->addButton(rotateToolButton, Rotate);
    connect(moveToolButton, &QToolButton::clicked, this, &MainWindow::onMoveToolClicked);
    connect(resizeToolButton, &QToolButton::clicked, this, &MainWindow::onResizeToolClicked);
    connect(rotateToolButton, &QToolButton::clicked, this, &MainWindow::onRotateToolClicked);

    // Width and Height spin boxes (hidden by default)
    widthLabel = new QLabel("W:", this);
    widthSpinBox = new QSpinBox(this);
    widthSpinBox->setRange(1, 10000);
    widthSpinBox->setVisible(false);
    widthLabel->setVisible(false);
    connect(widthSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::onWidthSpinChanged);
    connect(widthSpinBox, &QSpinBox::editingFinished, this, &MainWindow::onWidthSpinEditingFinished);
    heightLabel = new QLabel("H:", this);
    heightSpinBox = new QSpinBox(this);
    heightSpinBox->setRange(1, 10000);
    heightSpinBox->setVisible(false);
    heightLabel->setVisible(false);
    connect(heightSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::onHeightSpinChanged);
    connect(heightSpinBox, &QSpinBox::editingFinished, this, &MainWindow::onHeightSpinEditingFinished);

    // Rotation slider (hidden by default)
    rotationSlider = new QSlider(Qt::Horizontal, this);
    rotationSlider->setRange(-180, 180);
    rotationSlider->setValue(0);
    rotationSlider->setFixedWidth(120);
    rotationSlider->setVisible(false);
    rotationLabel = new QLabel("Angle:", this);
    rotationLabel->setVisible(false);
    connect(rotationSlider, &QSlider::valueChanged, this, &MainWindow::onRotationSliderChanged);

    // Add brush tool button
    brushToolButton = new QToolButton(this);
    brushToolButton->setText("Brush");
    brushToolButton->setCheckable(true);
    brushToolButton->setIcon(QIcon(":/icons/brush.png"));
    toolButtonGroup->addButton(brushToolButton, Brush);
    connect(brushToolButton, &QToolButton::clicked, this, &MainWindow::onBrushToolClicked);

    // Add eraser tool button
    eraserToolButton = new QToolButton(this);
    eraserToolButton->setText("Eraser");
    eraserToolButton->setCheckable(true);
    eraserToolButton->setIcon(QIcon(":/icons/eraser.png"));
    toolButtonGroup->addButton(eraserToolButton, Eraser);
    connect(eraserToolButton, &QToolButton::clicked, this, &MainWindow::onEraserToolClicked);

    // Layout for tools and opacity
    topLayout = new QHBoxLayout;
    topLayout->addWidget(globalOpacityLabel);
    topLayout->addWidget(globalOpacitySlider);
    topLayout->addSpacing(20);
    topLayout->addWidget(moveToolButton);
    topLayout->addWidget(resizeToolButton);
    topLayout->addWidget(rotateToolButton);
    topLayout->addWidget(widthLabel);
    topLayout->addWidget(widthSpinBox);
    topLayout->addWidget(heightLabel);
    topLayout->addWidget(heightSpinBox);
    topLayout->addWidget(rotationLabel);
    topLayout->addWidget(rotationSlider);
    topLayout->addWidget(brushToolButton);
    topLayout->addWidget(eraserToolButton);
    topLayout->addStretch();
    QWidget* topWidget = new QWidget(this);
    topWidget->setLayout(topLayout);

    // Main layout for central widget
    QVBoxLayout* centralLayout = new QVBoxLayout;
    centralLayout->addWidget(topWidget);
    centralLayout->addWidget(canvasView);
    QWidget* centralWidget = new QWidget(this);
    centralWidget->setLayout(centralLayout);
    setCentralWidget(centralWidget);

    setupMenu();
    setupLayerDock();

    connect(globalOpacitySlider, &QSlider::valueChanged, this, &MainWindow::onGlobalOpacitySliderChanged);
    connect(canvasScene, &QGraphicsScene::selectionChanged, this, &MainWindow::onSceneSelectionChanged);

    // Set Move as the default tool
    moveToolButton->setChecked(true);
    currentTool = Move;
    updateLayerInteraction();

    // Setup brush tool controls
    setupBrushTool();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setupMenu()
{
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
    QMenu *editMenu = menuBar()->addMenu(tr("&Edit"));

    // Add undo action to Edit menu
    undoAction = new QAction(tr("&Undo"), this);
    undoAction->setShortcut(QKeySequence::Undo);
    undoAction->setEnabled(false);  // Initially disabled
    connect(undoAction, &QAction::triggered, this, &MainWindow::undoLastOperation);
    editMenu->addAction(undoAction);

    QAction *newCanvasAction = new QAction(tr("&New Canvas"), this);
    newCanvasAction->setShortcut(QKeySequence::New);
    connect(newCanvasAction, &QAction::triggered, this, &MainWindow::createNewCanvas);
    fileMenu->addAction(newCanvasAction);

    QAction *newCanvasFromImageAction = new QAction(tr("New Canvas from &Image"), this);
    connect(newCanvasFromImageAction, &QAction::triggered, this, &MainWindow::createNewCanvasFromImage);
    fileMenu->addAction(newCanvasFromImageAction);

    QAction *openImageAction = new QAction(tr("&Open Image"), this);
    openImageAction->setShortcut(QKeySequence::Open);
    connect(openImageAction, &QAction::triggered, this, &MainWindow::openImage);
    fileMenu->addAction(openImageAction);

    QAction *saveImageAction = new QAction(tr("&Save Image"), this);
    saveImageAction->setShortcut(QKeySequence::Save);
    connect(saveImageAction, &QAction::triggered, this, &MainWindow::saveImage);
    fileMenu->addAction(saveImageAction);

    // Add Filters menu
    QMenu *filtersMenu = menuBar()->addMenu(tr("&Filters"));
    QAction *blurAction = new QAction(tr("Blur"), this);
    connect(blurAction, &QAction::triggered, this, &MainWindow::onBlurFilter);
    filtersMenu->addAction(blurAction);
    
    QAction *saturationAction = new QAction(tr("Saturation"), this);
    connect(saturationAction, &QAction::triggered, this, &MainWindow::onSaturationFilter);
    filtersMenu->addAction(saturationAction);
    
    QAction *brightnessAction = new QAction(tr("Brightness"), this);
    connect(brightnessAction, &QAction::triggered, this, &MainWindow::onBrightnessFilter);
    filtersMenu->addAction(brightnessAction);
    
    QAction *contrastAction = new QAction(tr("Contrast"), this);
    connect(contrastAction, &QAction::triggered, this, &MainWindow::onContrastFilter);
    filtersMenu->addAction(contrastAction);
    
    QAction *grayscaleAction = new QAction(tr("Grayscale"), this);
    connect(grayscaleAction, &QAction::triggered, this, &MainWindow::onGrayscaleFilter);
    filtersMenu->addAction(grayscaleAction);
    
    QAction *cartoonAction = new QAction(tr("Cartoon"), this);
    connect(cartoonAction, &QAction::triggered, this, &MainWindow::onCartoonFilter);
    filtersMenu->addAction(cartoonAction);
    
    QAction *vintageAction = new QAction(tr("Vintage"), this);
    connect(vintageAction, &QAction::triggered, this, &MainWindow::onVintageFilter);
    filtersMenu->addAction(vintageAction);
    
    QAction *edgeAction = new QAction(tr("Edge Detection"), this);
    connect(edgeAction, &QAction::triggered, this, &MainWindow::onEdgeFilter);
    filtersMenu->addAction(edgeAction);
    
    QAction *invertAction = new QAction(tr("Invert Colors"), this);
    connect(invertAction, &QAction::triggered, this, &MainWindow::onInvertFilter);
    filtersMenu->addAction(invertAction);
    
    QAction *hueAction = new QAction(tr("Hue"), this);
    connect(hueAction, &QAction::triggered, this, &MainWindow::onHueFilter);
    filtersMenu->addAction(hueAction);

    QAction *selectiveColorAction = new QAction(tr("Selective Color"), this);
    connect(selectiveColorAction, &QAction::triggered, this, &MainWindow::onSelectiveColorFilter);
    filtersMenu->addAction(selectiveColorAction);
}

void MainWindow::setupLayerDock()
{
    layerDock = new QDockWidget(tr("Layers"), this);
    layerDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

    // Create a widget to hold the layer list and buttons
    QWidget* layerWidget = new QWidget(layerDock);
    QVBoxLayout* layout = new QVBoxLayout(layerWidget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(2);

    // Create buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    QPushButton* addLayerButton = new QPushButton(tr("+"), this);
    addLayerButton->setToolTip(tr("Add New Layer"));
    addLayerButton->setFixedWidth(30);
    QPushButton* deleteLayerButton = new QPushButton(tr("-"), this);
    deleteLayerButton->setToolTip(tr("Delete Selected Layer"));
    deleteLayerButton->setFixedWidth(30);
    buttonLayout->addWidget(addLayerButton);
    buttonLayout->addWidget(deleteLayerButton);
    buttonLayout->addStretch();

    // Connect button signals
    connect(addLayerButton, &QPushButton::clicked, this, &MainWindow::addEmptyLayer);
    connect(deleteLayerButton, &QPushButton::clicked, this, &MainWindow::deleteLayer);

    // Add buttons to layout
    layout->addLayout(buttonLayout);

    // Create and setup layer list
    layerList = new QListWidget(layerWidget);
    layerList->setSelectionMode(QAbstractItemView::SingleSelection);
    layerList->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(layerList, &QListWidget::itemChanged, this, &MainWindow::toggleLayerVisibility);
    connect(layerList, &QListWidget::itemSelectionChanged, this, &MainWindow::layerSelectionChanged);
    connect(layerList, &QListWidget::customContextMenuRequested, this, &MainWindow::showLayerContextMenu);

    // Add layer list to layout
    layout->addWidget(layerList);

    // Set the widget as the dock's widget
    layerDock->setWidget(layerWidget);
    addDockWidget(Qt::RightDockWidgetArea, layerDock);
}

void MainWindow::logFunctionCall(const QString& functionName)
{
    QFile logFile("log.txt");
    if (logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        QTextStream out(&logFile);
        QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
        out << timestamp << " - Function called: " << functionName << "\n";
        logFile.close();
    }
}

void MainWindow::createNewCanvas()
{
    logFunctionCall("createNewCanvas");
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
        canvas->setPen(QPen(Qt::black, 1, Qt::SolidLine));  // Set a solid black outline
        canvas->setVisible(true);  // Ensure background is visible by default
        canvasScene->addItem(canvas);

        // Add background layer to layer list
        QListWidgetItem *backgroundItem = new QListWidgetItem("Background");
        backgroundItem->setFlags(backgroundItem->flags() | Qt::ItemIsUserCheckable);
        backgroundItem->setCheckState(Qt::Checked);  // Set initial visibility
        layerList->addItem(backgroundItem);

        // Adjust view to fit the canvas
        canvasView->fitInView(canvas, Qt::KeepAspectRatio);
    }
}

void MainWindow::createNewCanvasFromImage()
{
    logFunctionCall("createNewCanvasFromImage");
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Select Image for Canvas Size"), "",
        tr("Image Files (*.png *.jpg *.jpeg *.bmp *.gif)"));

    if (!fileName.isEmpty()) {
        QImage image(fileName);
        if (image.isNull()) {
            QMessageBox::warning(this, tr("Warning"),
                tr("Failed to load the image."));
            return;
        }

        // Clear existing scene and layer list
        canvasScene->clear();
        layerList->clear();

        // Create new canvas with white background
        QGraphicsRectItem *canvas = new QGraphicsRectItem(0, 0, image.width(), image.height());
        canvas->setBrush(Qt::white);
        canvas->setPen(QPen(Qt::black, 1, Qt::SolidLine));
        canvas->setVisible(true);
        canvasScene->addItem(canvas);

        // Add background layer to layer list
        QListWidgetItem *backgroundItem = new QListWidgetItem("Background");
        backgroundItem->setFlags(backgroundItem->flags() | Qt::ItemIsUserCheckable);
        backgroundItem->setCheckState(Qt::Checked);
        layerList->addItem(backgroundItem);

        // Convert image to ARGB32 format to ensure transparency support
        image = image.convertToFormat(QImage::Format_ARGB32);
        
        // Create a new image with transparency
        QImage transparentImage(image.size(), QImage::Format_ARGB32);
        transparentImage.fill(Qt::transparent);
        
        // Copy the original image onto the transparent image
        QPainter painter(&transparentImage);
        painter.drawImage(0, 0, image);
        painter.end();
        
        QPixmap pixmap = QPixmap::fromImage(transparentImage);

        // Add the image as a layer
        QString layerName = QFileInfo(fileName).baseName();
        addLayer(pixmap, layerName);

        // Adjust view to fit the canvas
        canvasView->fitInView(canvas, Qt::KeepAspectRatio);
    }
}

void MainWindow::openImage()
{
    logFunctionCall("openImage");
    if (!hasCanvas()) {
        QMessageBox::warning(this, tr("Warning"),
            tr("Please create a canvas first before adding images."));
        return;
    }

    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Open Image"), "",
        tr("Image Files (*.png *.jpg *.jpeg *.bmp *.gif)"));

    if (!fileName.isEmpty()) {
        // Load image with alpha channel support
        QImage image(fileName);
        if (image.isNull()) {
            QMessageBox::warning(this, tr("Warning"),
                tr("Failed to load the image."));
            return;
        }

        // Convert to ARGB32 format to ensure transparency support
        image = image.convertToFormat(QImage::Format_ARGB32);
        
        // Create a new image with transparency
        QImage transparentImage(image.size(), QImage::Format_ARGB32);
        transparentImage.fill(Qt::transparent);
        
        // Copy the original image onto the transparent image
        QPainter painter(&transparentImage);
        painter.drawImage(0, 0, image);
        painter.end();
        
        QPixmap pixmap = QPixmap::fromImage(transparentImage);

        // Create a layer name from the file name
        QString layerName = QFileInfo(fileName).baseName();
        addLayer(pixmap, layerName);
    }
}

void MainWindow::saveImage()
{
    logFunctionCall("saveImage");
    if (!hasCanvas()) {
        QMessageBox::warning(this, tr("Warning"),
            tr("Please create a canvas first before saving."));
        return;
    }

    QString fileName = QFileDialog::getSaveFileName(this,
        tr("Save Image"), "",
        tr("PNG Files (*.png);;JPEG Files (*.jpg);;BMP Files (*.bmp)"));

    if (!fileName.isEmpty()) {
        // Find the background/canvas item (assumed to be the last QGraphicsRectItem)
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
        
        // Always start with a transparent background
        image.fill(Qt::transparent);

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
        
        // If background is visible, draw it first
        if (canvas->isVisible() && canvas->brush().color() != Qt::transparent) {
            painter.fillRect(QRectF(QPointF(0, 0), canvasRect.size()), Qt::white);
        }
        
        // Render the scene (excluding the background if it's transparent)
        if (canvas->brush().color() == Qt::transparent) {
            // Temporarily hide the background for rendering
            canvas->setVisible(false);
            canvasScene->render(&painter, QRectF(QPointF(0, 0), canvasRect.size()), canvasRect);
            canvas->setVisible(true);
        } else {
            canvasScene->render(&painter, QRectF(QPointF(0, 0), canvasRect.size()), canvasRect);
        }
        
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
    logFunctionCall("addLayer");
    // Convert pixmap to image with alpha channel
    QImage image = pixmap.toImage().convertToFormat(QImage::Format_ARGB32);
    QPixmap transparentPixmap = QPixmap::fromImage(image);
    
    Layer *layer = new Layer(transparentPixmap, name);
    layer->setPos(0, 0);
    canvasScene->addItem(layer);
    
    // Add layer to layer list
    QListWidgetItem *item = new QListWidgetItem(name);
    item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
    item->setCheckState(Qt::Checked);  // Set initial visibility
    // Store Layer* in item for reliable selection sync
    item->setData(Qt::UserRole, QVariant::fromValue(static_cast<void*>(layer)));
    layerList->addItem(item);
    
    // Select the new layer
    layerList->setCurrentItem(item);
    
    // Update layer visibility
    layer->setVisible(item->checkState() == Qt::Checked);
}

void MainWindow::toggleLayerVisibility(QListWidgetItem* item)
{
    logFunctionCall("toggleLayerVisibility");
    int index = layerList->row(item);
    QList<QGraphicsItem*> items = canvasScene->items(Qt::DescendingOrder);  // Get items in reverse order to match layer list
    
    if (index == 0) {
        // Background layer
        QGraphicsRectItem* canvas = qgraphicsitem_cast<QGraphicsRectItem*>(items.last());  // Background is last item
        if (canvas) {
            bool isVisible = item->checkState() == Qt::Checked;
            canvas->setVisible(true);  // Always keep the canvas visible
            canvas->setBrush(isVisible ? Qt::white : Qt::transparent);  // Only toggle the fill
            canvas->setPen(QPen(Qt::black, 1, Qt::SolidLine));  // Keep the outline visible
        }
    } else {
        // Regular layer
        Layer* layer = qgraphicsitem_cast<Layer*>(items[index - 1]);
        if (layer) {
            layer->setVisible(item->checkState() == Qt::Checked);
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
    updateResizeSpinBoxes();
    updateLayerInteraction();
}

bool MainWindow::hasCanvas() const
{
    return !canvasScene->items().isEmpty();
}

void MainWindow::onGlobalOpacitySliderChanged(int value)
{
    logFunctionCall("onGlobalOpacitySliderChanged");
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
    updateResizeSpinBoxes();
    updateLayerInteraction();
}

void MainWindow::onMoveToolClicked()
{
    logFunctionCall("onMoveToolClicked");
    currentTool = Move;
    rotationSlider->setVisible(false);
    rotationLabel->setVisible(false);
    widthSpinBox->setVisible(false);
    widthLabel->setVisible(false);
    heightSpinBox->setVisible(false);
    heightLabel->setVisible(false);
    updateLayerInteraction();
}

void MainWindow::onResizeToolClicked()
{
    logFunctionCall("onResizeToolClicked");
    currentTool = Resize;
    rotationSlider->setVisible(false);
    rotationLabel->setVisible(false);
    widthSpinBox->setVisible(true);
    widthLabel->setVisible(true);
    heightSpinBox->setVisible(true);
    heightLabel->setVisible(true);
    updateResizeSpinBoxes();
    updateLayerInteraction();
}

void MainWindow::onRotateToolClicked()
{
    logFunctionCall("onRotateToolClicked");
    currentTool = Rotate;
    rotationSlider->setVisible(true);
    rotationLabel->setVisible(true);
    widthSpinBox->setVisible(false);
    widthLabel->setVisible(false);
    heightSpinBox->setVisible(false);
    heightLabel->setVisible(false);
    // Set slider to current layer's rotation
    QListWidgetItem *currentItem = layerList->currentItem();
    if (currentItem) {
        Layer* layer = static_cast<Layer*>(currentItem->data(Qt::UserRole).value<void*>());
        if (layer) {
            rotationSlider->blockSignals(true);
            rotationSlider->setValue(static_cast<int>(layer->rotation()));
            rotationSlider->blockSignals(false);
        }
    }
    updateLayerInteraction();
}

void MainWindow::onRotationSliderChanged(int value)
{
    if (currentTool != Rotate)
        return;
    QListWidgetItem *currentItem = layerList->currentItem();
    if (currentItem) {
        Layer* layer = static_cast<Layer*>(currentItem->data(Qt::UserRole).value<void*>());
        if (layer) {
            qreal oldRotation = layer->rotation();
            layer->setRotation(value);
            logRotation(layer->getName(), oldRotation, value);
        }
    }
}

void MainWindow::updateLayerInteraction()
{
    // Only the selected layer is affected
    QListWidgetItem *currentItem = layerList->currentItem();
    for (int i = 0; i < layerList->count(); ++i) {
        Layer* layer = static_cast<Layer*>(layerList->item(i)->data(Qt::UserRole).value<void*>());
        if (!layer) continue;
        if (layerList->item(i) == currentItem) {
            if (currentTool == Move) {
                layer->setFlag(QGraphicsItem::ItemIsMovable, true);
                layer->setFlag(QGraphicsItem::ItemIsSelectable, true);
                layer->setResizingEnabled(false);
            } else if (currentTool == Resize) {
                layer->setFlag(QGraphicsItem::ItemIsMovable, false);
                layer->setFlag(QGraphicsItem::ItemIsSelectable, true);
                layer->setResizingEnabled(true);
            } else if (currentTool == Brush) {
                layer->setFlag(QGraphicsItem::ItemIsMovable, false);
                layer->setFlag(QGraphicsItem::ItemIsSelectable, true);
                layer->setResizingEnabled(false);
            } else if (currentTool == Eraser) {
                layer->setFlag(QGraphicsItem::ItemIsMovable, false);
                layer->setFlag(QGraphicsItem::ItemIsSelectable, true);
                layer->setResizingEnabled(false);
            } else {
                layer->setFlag(QGraphicsItem::ItemIsMovable, false);
                layer->setFlag(QGraphicsItem::ItemIsSelectable, true);
                layer->setResizingEnabled(false);
            }
        } else {
            layer->setFlag(QGraphicsItem::ItemIsMovable, false);
            layer->setFlag(QGraphicsItem::ItemIsSelectable, false);
            layer->setResizingEnabled(false);
        }
    }
}

void MainWindow::updateResizeSpinBoxes()
{
    QListWidgetItem *currentItem = layerList->currentItem();
    if (currentItem) {
        Layer* layer = static_cast<Layer*>(currentItem->data(Qt::UserRole).value<void*>());
        if (layer) {
            QSizeF size = layer->targetSize();
            widthSpinBox->setValue(qRound(size.width()));
            heightSpinBox->setValue(qRound(size.height()));
        }
    }
}

void MainWindow::onWidthSpinChanged(int value)
{
    // No-op: only update on editingFinished
}

void MainWindow::onHeightSpinChanged(int value)
{
    // No-op: only update on editingFinished
}

void MainWindow::onWidthSpinEditingFinished()
{
    if (currentTool != Resize) return;
    QListWidgetItem *currentItem = layerList->currentItem();
    if (currentItem) {
        Layer* layer = static_cast<Layer*>(currentItem->data(Qt::UserRole).value<void*>());
        if (layer) {
            QSizeF oldSize = layer->targetSize();
            int newWidth = widthSpinBox->value();
            int newHeight = heightSpinBox->value();
            layer->setSize(newWidth, newHeight);
            logResize(layer->getName(), oldSize, QSizeF(newWidth, newHeight));
            QTimer::singleShot(0, this, &MainWindow::updateResizeSpinBoxes);
        }
    }
}

void MainWindow::onHeightSpinEditingFinished()
{
    if (currentTool != Resize) return;
    QListWidgetItem *currentItem = layerList->currentItem();
    if (currentItem) {
        Layer* layer = static_cast<Layer*>(currentItem->data(Qt::UserRole).value<void*>());
        if (layer) {
            QSizeF oldSize = layer->targetSize();
            int newWidth = widthSpinBox->value();
            int newHeight = heightSpinBox->value();
            layer->setSize(newWidth, newHeight);
            logResize(layer->getName(), oldSize, QSizeF(newWidth, newHeight));
            QTimer::singleShot(0, this, &MainWindow::updateResizeSpinBoxes);
        }
    }
}

void MainWindow::onBlurFilter()
{
    logFunctionCall("onBlurFilter");
    QListWidgetItem *currentItem = layerList->currentItem();
    if (!currentItem) {
        QMessageBox::warning(this, tr("No Layer Selected"), tr("Please select a layer to apply the blur filter."));
        return;
    }
    Layer* layer = static_cast<Layer*>(currentItem->data(Qt::UserRole).value<void*>());
    if (!layer) {
        QMessageBox::warning(this, tr("No Layer Selected"), tr("Please select a valid layer to apply the blur filter."));
        return;
    }

    QDialog dialog(this);
    dialog.setWindowTitle(tr("Blur Filter"));
    QVBoxLayout* layout = new QVBoxLayout(&dialog);

    // Add slider for blur intensity
    QLabel* label = new QLabel(tr("Blur Intensity:"), &dialog);
    QSlider* slider = new QSlider(Qt::Horizontal, &dialog);
    slider->setRange(1, 31);
    slider->setSingleStep(2);
    slider->setPageStep(2);
    slider->setValue(5);
    slider->setTickInterval(2);
    slider->setTickPosition(QSlider::TicksBelow);

    // Add value label
    QLabel* valueLabel = new QLabel(QString::number(slider->value()), &dialog);
    connect(slider, &QSlider::valueChanged, valueLabel, static_cast<void(QLabel::*)(int)>(&QLabel::setNum));

    // Add preview checkbox
    QCheckBox* previewCheck = new QCheckBox(tr("Live Preview"), &dialog);
    previewCheck->setChecked(true);

    // Add buttons
    QPushButton* okButton = new QPushButton(tr("OK"), &dialog);
    QPushButton* cancelButton = new QPushButton(tr("Cancel"), &dialog);

    // Layout
    QHBoxLayout* sliderLayout = new QHBoxLayout;
    sliderLayout->addWidget(slider);
    sliderLayout->addWidget(valueLabel);

    QHBoxLayout* buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);

    layout->addWidget(label);
    layout->addLayout(sliderLayout);
    layout->addWidget(previewCheck);
    layout->addLayout(buttonLayout);

    // Connect signals
    connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

    // Store original pixmap for preview
    QPixmap originalPixmap = layer->pixmap();
    QMetaObject::Connection previewConnection;

    // Function to apply blur preview
    auto applyPreview = [=](int value) {
        int ksize = value;
        if (ksize % 2 == 0) ksize += 1;
        
        // Apply blur to the original image
        QPixmap tempPixmap = originalPixmap;
        cv::Mat mat = Layer::pixmapToMat(tempPixmap);
        if (!mat.empty()) {
            cv::Mat blurred;
            cv::blur(mat, blurred, cv::Size(ksize, ksize));
            tempPixmap = Layer::matToPixmap(blurred);
            layer->setPixmap(tempPixmap);
        }
    };

    // Connect preview checkbox
    connect(previewCheck, &QCheckBox::toggled, [&previewConnection, slider, applyPreview, layer, originalPixmap](bool checked) {
        if (checked) {
            // Connect slider for live preview
            previewConnection = connect(slider, &QSlider::valueChanged, applyPreview);
            // Apply current value
            applyPreview(slider->value());
        } else {
            // Disconnect slider and restore original
            disconnect(previewConnection);
            layer->setPixmap(originalPixmap);
        }
    });

    // Initial preview if checked
    if (previewCheck->isChecked()) {
        previewConnection = connect(slider, &QSlider::valueChanged, applyPreview);
        applyPreview(slider->value());
    }

    // Show dialog
    if (dialog.exec() == QDialog::Accepted) {
        // Final application of blur
        int ksize = slider->value();
        if (ksize % 2 == 0) ksize += 1;
        
        // Apply blur to the original image
        cv::Mat mat = Layer::pixmapToMat(originalPixmap);
        if (!mat.empty()) {
            cv::Mat blurred;
            cv::blur(mat, blurred, cv::Size(ksize, ksize));
            layer->setPixmap(Layer::matToPixmap(blurred));
        }
    } else {
        // Restore original pixmap if cancelled
        layer->setPixmap(originalPixmap);
    }
}

void MainWindow::onSaturationFilter()
{
    logFunctionCall("onSaturationFilter");
    QListWidgetItem *currentItem = layerList->currentItem();
    if (!currentItem) {
        QMessageBox::warning(this, tr("No Layer Selected"), tr("Please select a layer to apply the saturation filter."));
        return;
    }
    Layer* layer = static_cast<Layer*>(currentItem->data(Qt::UserRole).value<void*>());
    if (!layer) {
        QMessageBox::warning(this, tr("No Layer Selected"), tr("Please select a valid layer to apply the saturation filter."));
        return;
    }
    QDialog dialog(this);
    dialog.setWindowTitle(tr("Saturation Filter"));
    QVBoxLayout* layout = new QVBoxLayout(&dialog);
    QLabel* label = new QLabel(tr("Saturation (0% = grayscale, 100% = original, 200% = double):"), &dialog);
    QSlider* slider = new QSlider(Qt::Horizontal, &dialog);
    slider->setRange(0, 200);
    slider->setValue(100);
    QLabel* valueLabel = new QLabel(QString::number(slider->value()) + "%", &dialog);
    connect(slider, &QSlider::valueChanged, [=](int v){ valueLabel->setText(QString::number(v) + "%"); });
    QHBoxLayout* hLayout = new QHBoxLayout;
    hLayout->addWidget(slider);
    hLayout->addWidget(valueLabel);
    QPushButton* okBtn = new QPushButton(tr("OK"), &dialog);
    QPushButton* cancelBtn = new QPushButton(tr("Cancel"), &dialog);
    QHBoxLayout* btnLayout = new QHBoxLayout;
    btnLayout->addWidget(okBtn);
    btnLayout->addWidget(cancelBtn);
    layout->addWidget(label);
    layout->addLayout(hLayout);
    layout->addLayout(btnLayout);
    QObject::connect(okBtn, &QPushButton::clicked, &dialog, &QDialog::accept);
    QObject::connect(cancelBtn, &QPushButton::clicked, &dialog, &QDialog::reject);
    if (dialog.exec() == QDialog::Accepted) {
        double factor = slider->value() / 100.0;
        layer->applySaturation(factor);
    }
}

void MainWindow::onBrightnessFilter()
{
    logFunctionCall("onBrightnessFilter");
    QListWidgetItem *currentItem = layerList->currentItem();
    if (!currentItem) {
        QMessageBox::warning(this, tr("No Layer Selected"), tr("Please select a layer to adjust brightness."));
        return;
    }
    Layer* layer = static_cast<Layer*>(currentItem->data(Qt::UserRole).value<void*>());
    if (!layer) {
        QMessageBox::warning(this, tr("No Layer Selected"), tr("Please select a valid layer to adjust brightness."));
        return;
    }

    QDialog dialog(this);
    dialog.setWindowTitle(tr("Brightness Adjustment"));
    QVBoxLayout* layout = new QVBoxLayout(&dialog);

    // Add slider for brightness
    QLabel* label = new QLabel(tr("Brightness:"), &dialog);
    QSlider* slider = new QSlider(Qt::Horizontal, &dialog);
    slider->setRange(-100, 100);
    slider->setValue(0);
    slider->setTickInterval(20);
    slider->setTickPosition(QSlider::TicksBelow);

    // Add value label
    QLabel* valueLabel = new QLabel("0", &dialog);
    connect(slider, &QSlider::valueChanged, valueLabel, static_cast<void(QLabel::*)(int)>(&QLabel::setNum));

    // Add preview checkbox
    QCheckBox* previewCheck = new QCheckBox(tr("Live Preview"), &dialog);
    previewCheck->setChecked(true);

    // Add buttons
    QPushButton* okButton = new QPushButton(tr("OK"), &dialog);
    QPushButton* cancelButton = new QPushButton(tr("Cancel"), &dialog);

    // Layout
    QHBoxLayout* sliderLayout = new QHBoxLayout;
    sliderLayout->addWidget(slider);
    sliderLayout->addWidget(valueLabel);

    QHBoxLayout* buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);

    layout->addWidget(label);
    layout->addLayout(sliderLayout);
    layout->addWidget(previewCheck);
    layout->addLayout(buttonLayout);

    // Connect signals
    connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

    // Store original pixmap for preview
    QPixmap originalPixmap = layer->pixmap();
    QMetaObject::Connection previewConnection;

    // Function to apply brightness preview
    auto applyPreview = [=](int value) {
        cv::Mat mat = Layer::pixmapToMat(originalPixmap);
        if (!mat.empty()) {
            cv::Mat adjusted;
            mat.convertTo(adjusted, -1, 1, value);  // alpha=1, beta=value
            layer->setPixmap(Layer::matToPixmap(adjusted));
        }
    };

    // Connect preview checkbox
    connect(previewCheck, &QCheckBox::toggled, [&previewConnection, slider, applyPreview, layer, originalPixmap](bool checked) {
        if (checked) {
            // Connect slider for live preview
            previewConnection = connect(slider, &QSlider::valueChanged, applyPreview);
            // Apply current value
            applyPreview(slider->value());
        } else {
            // Disconnect slider and restore original
            disconnect(previewConnection);
            layer->setPixmap(originalPixmap);
        }
    });

    // Initial preview if checked
    if (previewCheck->isChecked()) {
        previewConnection = connect(slider, &QSlider::valueChanged, applyPreview);
        applyPreview(slider->value());
    }

    // Show dialog
    if (dialog.exec() == QDialog::Accepted) {
        // Final application of brightness
        cv::Mat mat = Layer::pixmapToMat(originalPixmap);
        if (!mat.empty()) {
            cv::Mat adjusted;
            mat.convertTo(adjusted, -1, 1, slider->value());  // alpha=1, beta=value
            layer->setPixmap(Layer::matToPixmap(adjusted));
        }
    } else {
        // Restore original pixmap if cancelled
        layer->setPixmap(originalPixmap);
    }
}

void MainWindow::onContrastFilter()
{
    logFunctionCall("onContrastFilter");
    QListWidgetItem *currentItem = layerList->currentItem();
    if (!currentItem) {
        QMessageBox::warning(this, tr("No Layer Selected"), tr("Please select a layer to adjust contrast."));
        return;
    }
    Layer* layer = static_cast<Layer*>(currentItem->data(Qt::UserRole).value<void*>());
    if (!layer) {
        QMessageBox::warning(this, tr("No Layer Selected"), tr("Please select a valid layer to adjust contrast."));
        return;
    }

    QDialog dialog(this);
    dialog.setWindowTitle(tr("Contrast Adjustment"));
    QVBoxLayout* layout = new QVBoxLayout(&dialog);

    // Add slider for contrast
    QLabel* label = new QLabel(tr("Contrast:"), &dialog);
    QSlider* slider = new QSlider(Qt::Horizontal, &dialog);
    slider->setRange(0, 200);
    slider->setValue(100);
    slider->setTickInterval(20);
    slider->setTickPosition(QSlider::TicksBelow);

    // Add value label
    QLabel* valueLabel = new QLabel("100%", &dialog);
    connect(slider, &QSlider::valueChanged, [=](int value) {
        valueLabel->setText(QString::number(value) + "%");
    });

    // Add preview checkbox
    QCheckBox* previewCheck = new QCheckBox(tr("Live Preview"), &dialog);
    previewCheck->setChecked(true);

    // Add buttons
    QPushButton* okButton = new QPushButton(tr("OK"), &dialog);
    QPushButton* cancelButton = new QPushButton(tr("Cancel"), &dialog);

    // Layout
    QHBoxLayout* sliderLayout = new QHBoxLayout;
    sliderLayout->addWidget(slider);
    sliderLayout->addWidget(valueLabel);

    QHBoxLayout* buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);

    layout->addWidget(label);
    layout->addLayout(sliderLayout);
    layout->addWidget(previewCheck);
    layout->addLayout(buttonLayout);

    // Connect signals
    connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

    // Store original pixmap for preview
    QPixmap originalPixmap = layer->pixmap();
    QMetaObject::Connection previewConnection;

    // Function to apply contrast preview
    auto applyPreview = [=](int value) {
        double factor = value / 100.0;
        cv::Mat mat = Layer::pixmapToMat(originalPixmap);
        if (!mat.empty()) {
            cv::Mat adjusted;
            mat.convertTo(adjusted, -1, factor, 0);
            layer->setPixmap(Layer::matToPixmap(adjusted));
        }
    };

    // Connect preview checkbox
    connect(previewCheck, &QCheckBox::toggled, [&previewConnection, slider, applyPreview, layer, originalPixmap](bool checked) {
        if (checked) {
            // Connect slider for live preview
            previewConnection = connect(slider, &QSlider::valueChanged, applyPreview);
            // Apply current value
            applyPreview(slider->value());
        } else {
            // Disconnect slider and restore original
            disconnect(previewConnection);
            layer->setPixmap(originalPixmap);
        }
    });

    // Initial preview if checked
    if (previewCheck->isChecked()) {
        previewConnection = connect(slider, &QSlider::valueChanged, applyPreview);
        applyPreview(slider->value());
    }

    // Show dialog
    if (dialog.exec() == QDialog::Accepted) {
        // Final application of contrast
        double factor = slider->value() / 100.0;
        cv::Mat mat = Layer::pixmapToMat(originalPixmap);
        if (!mat.empty()) {
            cv::Mat adjusted;
            mat.convertTo(adjusted, -1, factor, 0);
            layer->setPixmap(Layer::matToPixmap(adjusted));
        }
    } else {
        // Restore original pixmap if cancelled
        layer->setPixmap(originalPixmap);
    }
}

void MainWindow::onGrayscaleFilter()
{
    showFilterPreviewDialog(tr("Grayscale Filter"), 
                          [this](const QPixmap& input, QPixmap& output) {
                              this->applyGrayscale(input, output);
                          },
                          "Grayscale Filter");
}

void MainWindow::onCartoonFilter()
{
    showFilterPreviewDialog(tr("Cartoon Filter"), 
                          [this](const QPixmap& input, QPixmap& output) {
                              this->applyCartoon(input, output);
                          },
                          "Cartoon Filter");
}

void MainWindow::onVintageFilter()
{
    showFilterPreviewDialog(tr("Vintage Filter"), 
                          [this](const QPixmap& input, QPixmap& output) {
                              this->applyVintage(input, output);
                          },
                          "Vintage Filter");
}

void MainWindow::onEdgeFilter()
{
    showFilterPreviewDialog(tr("Edge Detection Filter"), 
                          [this](const QPixmap& input, QPixmap& output) {
                              this->applyEdge(input, output);
                          },
                          "Edge Detection Filter");
}

void MainWindow::onInvertFilter()
{
    logFunctionCall("onInvertFilter");
    QListWidgetItem *currentItem = layerList->currentItem();
    if (!currentItem) {
        QMessageBox::warning(this, tr("No Layer Selected"), tr("Please select a layer to invert colors."));
        return;
    }
    Layer* layer = static_cast<Layer*>(currentItem->data(Qt::UserRole).value<void*>());
    if (!layer) {
        QMessageBox::warning(this, tr("No Layer Selected"), tr("Please select a valid layer to invert colors."));
        return;
    }

    layer->applyInvert();
}

void MainWindow::showFilterPreviewDialog(const QString& title, 
                                       std::function<void(const QPixmap&, QPixmap&)> filterFunction,
                                       const QString& filterName)
{
    QListWidgetItem *currentItem = layerList->currentItem();
    if (!currentItem) {
        QMessageBox::warning(this, tr("No Layer Selected"), 
            tr("Please select a layer to apply the filter."));
        return;
    }

    Layer* layer = static_cast<Layer*>(currentItem->data(Qt::UserRole).value<void*>());
    if (!layer) {
        QMessageBox::warning(this, tr("No Layer Selected"), 
            tr("Please select a valid layer to apply the filter."));
        return;
    }

    // Store original pixmap
    QPixmap originalPixmap = layer->pixmap();
    QPixmap previewPixmap = originalPixmap;

    // Create dialog
    QDialog dialog(this);
    dialog.setWindowTitle(title);
    QVBoxLayout* layout = new QVBoxLayout(&dialog);

    // Create preview label
    QLabel* previewLabel = new QLabel(&dialog);
    previewLabel->setMinimumSize(400, 300);
    previewLabel->setAlignment(Qt::AlignCenter);
    previewLabel->setPixmap(previewPixmap.scaled(400, 300, Qt::KeepAspectRatio, Qt::SmoothTransformation));

    // Add buttons
    QPushButton* okButton = new QPushButton(tr("Apply"), &dialog);
    QPushButton* cancelButton = new QPushButton(tr("Cancel"), &dialog);
    QHBoxLayout* buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);

    // Add widgets to layout
    layout->addWidget(previewLabel);
    layout->addLayout(buttonLayout);

    // Connect buttons
    connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

    // Apply filter to preview
    filterFunction(originalPixmap, previewPixmap);
    previewLabel->setPixmap(previewPixmap.scaled(400, 300, Qt::KeepAspectRatio, Qt::SmoothTransformation));

    // Show dialog
    if (dialog.exec() == QDialog::Accepted) {
        layer->setPixmap(previewPixmap);
        logFunctionCall(filterName);
    } else {
        layer->setPixmap(originalPixmap);
    }
}

void MainWindow::applyGrayscale(const QPixmap& input, QPixmap& output)
{
    QImage image = input.toImage();
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            QRgb pixel = image.pixel(x, y);
            int gray = qGray(pixel);
            image.setPixel(x, y, qRgba(gray, gray, gray, qAlpha(pixel)));
        }
    }
    output = QPixmap::fromImage(image);
}

void MainWindow::applyCartoon(const QPixmap& input, QPixmap& output)
{
    cv::Mat inputMat = Layer::pixmapToMat(input);
    cv::Mat outputMat;

    // Convert to grayscale
    cv::Mat gray;
    cv::cvtColor(inputMat, gray, cv::COLOR_BGRA2GRAY);

    // Apply median blur to reduce noise
    cv::Mat medianBlurred;
    cv::medianBlur(gray, medianBlurred, 7);

    // Apply edge detection using Laplacian
    cv::Mat edges;
    cv::Laplacian(medianBlurred, edges, CV_8U, 5);
    cv::threshold(edges, edges, 80, 255, cv::THRESH_BINARY_INV);

    // Apply bilateral filter to the color image for color quantization
    cv::Mat color;
    cv::bilateralFilter(inputMat, color, 9, 300, 300);

    // Reduce color palette using k-means clustering
    cv::Mat color32;
    color.convertTo(color32, CV_32F);
    cv::Mat samples = color32.reshape(1, color32.total());
    cv::Mat labels, centers;
    cv::kmeans(samples, 8, labels, cv::TermCriteria(cv::TermCriteria::EPS + cv::TermCriteria::COUNT, 10, 1.0),
               3, cv::KMEANS_PP_CENTERS, centers);
    centers.convertTo(centers, CV_8U);
    cv::Mat reduced = centers.reshape(4, color.rows);
    reduced.convertTo(reduced, CV_8U);

    // Combine edges with reduced color
    cv::Mat cartoon;
    cv::bitwise_and(reduced, reduced, cartoon, edges);

    // Add slight smoothing to the final result
    cv::GaussianBlur(cartoon, outputMat, cv::Size(3, 3), 0);

    output = Layer::matToPixmap(outputMat);
}

void MainWindow::applyVintage(const QPixmap& input, QPixmap& output)
{
    cv::Mat inputMat = Layer::pixmapToMat(input);
    cv::Mat outputMat;

    // Convert to LAB color space
    cv::Mat lab;
    cv::cvtColor(inputMat, lab, cv::COLOR_BGRA2BGR);
    cv::cvtColor(lab, lab, cv::COLOR_BGR2Lab);

    // Split channels
    std::vector<cv::Mat> channels;
    cv::split(lab, channels);

    // Adjust L channel for vintage look
    channels[0] = channels[0] * 0.8;  // Darken
    channels[1] = channels[1] * 1.2;  // Increase green-yellow
    channels[2] = channels[2] * 0.8;  // Decrease blue

    // Merge channels
    cv::merge(channels, lab);

    // Convert back to BGR
    cv::cvtColor(lab, outputMat, cv::COLOR_Lab2BGR);
    cv::cvtColor(outputMat, outputMat, cv::COLOR_BGR2BGRA);

    // Add slight sepia tone
    cv::Mat sepia = outputMat.clone();
    sepia.setTo(cv::Scalar(20, 66, 112, 0));
    cv::addWeighted(outputMat, 0.7, sepia, 0.3, 0, outputMat);

    output = Layer::matToPixmap(outputMat);
}

void MainWindow::applyEdge(const QPixmap& input, QPixmap& output)
{
    cv::Mat inputMat = Layer::pixmapToMat(input);
    cv::Mat outputMat;

    // Convert to grayscale
    cv::Mat gray;
    cv::cvtColor(inputMat, gray, cv::COLOR_BGRA2GRAY);

    // Apply Gaussian blur
    cv::GaussianBlur(gray, gray, cv::Size(3, 3), 0);

    // Apply Canny edge detection
    cv::Mat edges;
    cv::Canny(gray, edges, 50, 150);

    // Convert edges to BGRA
    cv::cvtColor(edges, outputMat, cv::COLOR_GRAY2BGRA);

    // Invert colors
    cv::bitwise_not(outputMat, outputMat);

    output = Layer::matToPixmap(outputMat);
}

void MainWindow::createActions()
{
    openAction = new QAction(QIcon(":/icons/open.png"), tr("&Open..."), this);
    openAction->setShortcuts(QKeySequence::Open);
    openAction->setStatusTip(tr("Open an existing file"));
    connect(openAction, &QAction::triggered, this, &MainWindow::openImage);

    saveAction = new QAction(QIcon(":/icons/save.png"), tr("&Save..."), this);
    saveAction->setShortcuts(QKeySequence::Save);
    saveAction->setStatusTip(tr("Save the document to disk"));
    connect(saveAction, &QAction::triggered, this, &MainWindow::saveImage);

    addLayerAction = new QAction(QIcon(":/icons/add_layer.png"), tr("&Add Layer"), this);
    addLayerAction->setStatusTip(tr("Add a new layer"));
    connect(addLayerAction, &QAction::triggered, this, &MainWindow::addNewLayer);

    deleteLayerAction = new QAction(QIcon(":/icons/delete_layer.png"), tr("&Delete Layer"), this);
    deleteLayerAction->setStatusTip(tr("Delete selected layer"));
    connect(deleteLayerAction, &QAction::triggered, this, &MainWindow::deleteLayer);

    gridAction = new QAction(QIcon(":/icons/grid.png"), tr("&Grid"), this);
    gridAction->setStatusTip(tr("Toggle grid"));
    gridAction->setCheckable(true);
    connect(gridAction, &QAction::triggered, this, &MainWindow::toggleGrid);

    blurAction = new QAction(QIcon(":/icons/blur.png"), tr("&Blur"), this);
    blurAction->setStatusTip(tr("Apply blur filter"));
    connect(blurAction, &QAction::triggered, this, &MainWindow::showBlurDialog);

    saturationAction = new QAction(QIcon(":/icons/saturation.png"), tr("&Saturation"), this);
    saturationAction->setStatusTip(tr("Adjust saturation"));
    connect(saturationAction, &QAction::triggered, this, &MainWindow::onSaturationFilter);

    resizeAction = new QAction(QIcon(":/icons/resize.png"), tr("&Resize"), this);
    resizeAction->setStatusTip(tr("Resize canvas"));
    connect(resizeAction, &QAction::triggered, this, &MainWindow::showResizeDialog);
}

void MainWindow::createToolBar()
{
    toolBar = addToolBar(tr("Tools"));
    toolBar->addAction(undoAction);  // Add undo button to toolbar
    toolBar->addAction(openAction);
    toolBar->addAction(saveAction);
    toolBar->addSeparator();
    toolBar->addAction(addLayerAction);
    toolBar->addAction(deleteLayerAction);
    toolBar->addSeparator();
    toolBar->addAction(gridAction);
    toolBar->addSeparator();
    toolBar->addAction(blurAction);
    toolBar->addAction(saturationAction);
    toolBar->addSeparator();
    toolBar->addAction(resizeAction);
    toolBar->addSeparator();
    toolBar->addWidget(moveToolButton);
    toolBar->addWidget(resizeToolButton);
    toolBar->addWidget(rotateToolButton);
    toolBar->addWidget(brushToolButton);
    toolBar->addWidget(eraserToolButton);
}

void MainWindow::deleteLayer()
{
    logFunctionCall("deleteLayer");
    QListWidgetItem* currentItem = layerList->currentItem();
    if (!currentItem) {
        QMessageBox::warning(this, tr("No Layer Selected"), tr("Please select a layer to delete."));
        return;
    }

    int index = layerList->row(currentItem);
    if (index == 0) {
        QMessageBox::warning(this, tr("Cannot Delete"), tr("Cannot delete the background layer."));
        return;
    }

    // Get the layer pointer directly from the list item
    Layer* layer = static_cast<Layer*>(currentItem->data(Qt::UserRole).value<void*>());
    if (!layer) {
        QMessageBox::warning(this, tr("Error"), tr("Invalid layer data."));
        return;
    }

    // Remove the layer from the scene
    canvasScene->removeItem(layer);
    
    // Remove the item from the list first
    delete currentItem;
    
    // Delete the layer object last
    delete layer;

    // Always select the background layer after deletion
    layerList->setCurrentRow(0);
}

void MainWindow::updateLayerPanel()
{
    // Clear the current list
    layerList->clear();

    // Add background layer first
    QListWidgetItem* backgroundItem = new QListWidgetItem("Background");
    backgroundItem->setFlags(backgroundItem->flags() | Qt::ItemIsUserCheckable);
    backgroundItem->setCheckState(Qt::Checked);
    layerList->addItem(backgroundItem);

    // Add all layers from the scene
    QList<QGraphicsItem*> items = canvasScene->items(Qt::DescendingOrder);
    for (QGraphicsItem* item : items) {
        Layer* layer = qgraphicsitem_cast<Layer*>(item);
        if (layer) {
            QListWidgetItem* listItem = new QListWidgetItem(layer->getName());
            listItem->setFlags(listItem->flags() | Qt::ItemIsUserCheckable);
            listItem->setCheckState(layer->isVisible() ? Qt::Checked : Qt::Unchecked);
            listItem->setData(Qt::UserRole, QVariant::fromValue(static_cast<void*>(layer)));
            layerList->addItem(listItem);
        }
    }
}

void MainWindow::showResizeDialog()
{
    logFunctionCall("showResizeDialog");
    if (!hasCanvas()) {
        QMessageBox::warning(this, tr("Warning"),
            tr("Please create a canvas first before resizing."));
        return;
    }

    // Find the background/canvas item
    QGraphicsRectItem* canvas = nullptr;
    for (QGraphicsItem* item : canvasScene->items(Qt::AscendingOrder)) {
        canvas = qgraphicsitem_cast<QGraphicsRectItem*>(item);
        if (canvas) break;
    }
    if (!canvas) {
        QMessageBox::warning(this, tr("Warning"), tr("No canvas found."));
        return;
    }

    // Create dialog
    QDialog dialog(this);
    dialog.setWindowTitle(tr("Resize Canvas"));
    QVBoxLayout* layout = new QVBoxLayout(&dialog);

    // Width input
    QHBoxLayout* widthLayout = new QHBoxLayout;
    QLabel* widthLabel = new QLabel(tr("Width:"), &dialog);
    QSpinBox* widthSpinBox = new QSpinBox(&dialog);
    widthSpinBox->setRange(1, 10000);
    widthSpinBox->setValue(qRound(canvas->rect().width()));
    widthLayout->addWidget(widthLabel);
    widthLayout->addWidget(widthSpinBox);

    // Height input
    QHBoxLayout* heightLayout = new QHBoxLayout;
    QLabel* heightLabel = new QLabel(tr("Height:"), &dialog);
    QSpinBox* heightSpinBox = new QSpinBox(&dialog);
    heightSpinBox->setRange(1, 10000);
    heightSpinBox->setValue(qRound(canvas->rect().height()));
    heightLayout->addWidget(heightLabel);
    heightLayout->addWidget(heightSpinBox);

    // Buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout;
    QPushButton* okButton = new QPushButton(tr("OK"), &dialog);
    QPushButton* cancelButton = new QPushButton(tr("Cancel"), &dialog);
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);

    // Add all layouts to main layout
    layout->addLayout(widthLayout);
    layout->addLayout(heightLayout);
    layout->addLayout(buttonLayout);

    // Connect buttons
    connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

    // Show dialog
    if (dialog.exec() == QDialog::Accepted) {
        int newWidth = widthSpinBox->value();
        int newHeight = heightSpinBox->value();
        
        // Update canvas size
        canvas->setRect(0, 0, newWidth, newHeight);
        
        // Adjust view to fit the new canvas size
        canvasView->fitInView(canvas, Qt::KeepAspectRatio);
    }
}

void MainWindow::toggleGrid()
{
    logFunctionCall("toggleGrid");
    // Toggle grid visibility
    bool showGrid = gridAction->isChecked();
    
    // Find the background/canvas item
    QGraphicsRectItem* canvas = nullptr;
    for (QGraphicsItem* item : canvasScene->items(Qt::AscendingOrder)) {
        canvas = qgraphicsitem_cast<QGraphicsRectItem*>(item);
        if (canvas) break;
    }
    if (!canvas) return;

    // Update canvas pen to show/hide grid
    if (showGrid) {
        // Create a grid pattern
        QPen gridPen(Qt::lightGray);
        gridPen.setStyle(Qt::DotLine);
        canvas->setPen(gridPen);
    } else {
        // Hide grid
        canvas->setPen(QPen(Qt::black));
    }

    // Force a redraw
    canvasScene->update();
}

void MainWindow::addNewLayer()
{
    logFunctionCall("addNewLayer");
    if (!hasCanvas()) {
        QMessageBox::warning(this, tr("Warning"),
            tr("Please create a canvas first before adding layers."));
        return;
    }

    // Find the background/canvas item to get its size
    QGraphicsRectItem* canvas = nullptr;
    for (QGraphicsItem* item : canvasScene->items(Qt::AscendingOrder)) {
        canvas = qgraphicsitem_cast<QGraphicsRectItem*>(item);
        if (canvas) break;
    }
    if (!canvas) {
        QMessageBox::warning(this, tr("Warning"), tr("No canvas found."));
        return;
    }

    // Create a new transparent layer with the same size as the canvas
    QPixmap newLayerPixmap(canvas->rect().size().toSize());
    newLayerPixmap.fill(Qt::transparent);
    
    // Add the new layer
    QString layerName = QString("Layer %1").arg(layerList->count());
    addLayer(newLayerPixmap, layerName);
}

void MainWindow::applySaturation()
{
    QListWidgetItem *currentItem = layerList->currentItem();
    if (!currentItem) {
        QMessageBox::warning(this, tr("No Layer Selected"), tr("Please select a layer to adjust saturation."));
        return;
    }
    Layer* layer = static_cast<Layer*>(currentItem->data(Qt::UserRole).value<void*>());
    if (!layer) {
        QMessageBox::warning(this, tr("No Layer Selected"), tr("Please select a valid layer to adjust saturation."));
        return;
    }

    QDialog dialog(this);
    dialog.setWindowTitle(tr("Saturation Adjustment"));
    QVBoxLayout* layout = new QVBoxLayout(&dialog);

    // Add slider for saturation
    QLabel* label = new QLabel(tr("Saturation:"), &dialog);
    QSlider* slider = new QSlider(Qt::Horizontal, &dialog);
    slider->setRange(0, 200);
    slider->setValue(100);
    slider->setTickInterval(20);
    slider->setTickPosition(QSlider::TicksBelow);

    // Add value label
    QLabel* valueLabel = new QLabel("100%", &dialog);
    connect(slider, &QSlider::valueChanged, [=](int value) {
        valueLabel->setText(QString::number(value) + "%");
    });

    // Add preview checkbox
    QCheckBox* previewCheck = new QCheckBox(tr("Live Preview"), &dialog);
    previewCheck->setChecked(true);

    // Add buttons
    QPushButton* okButton = new QPushButton(tr("OK"), &dialog);
    QPushButton* cancelButton = new QPushButton(tr("Cancel"), &dialog);

    // Layout
    QHBoxLayout* sliderLayout = new QHBoxLayout;
    sliderLayout->addWidget(slider);
    sliderLayout->addWidget(valueLabel);

    QHBoxLayout* buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);

    layout->addWidget(label);
    layout->addLayout(sliderLayout);
    layout->addWidget(previewCheck);
    layout->addLayout(buttonLayout);

    // Connect signals
    connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

    // Store original pixmap for preview
    QPixmap originalPixmap = layer->pixmap();

    // Connect slider for live preview
    if (previewCheck->isChecked()) {
        connect(slider, &QSlider::valueChanged, [=](int value) {
            double factor = value / 100.0;
            layer->applySaturation(factor);
        });
    }

    // Show dialog
    if (dialog.exec() == QDialog::Accepted) {
        // Final application of saturation
        double factor = slider->value() / 100.0;
        layer->applySaturation(factor);
    } else {
        // Restore original pixmap if cancelled
        layer->setPixmap(originalPixmap);
    }
}

void MainWindow::updateCanvasSize()
{
    if (!hasCanvas()) return;

    // Find the background/canvas item
    QGraphicsRectItem* canvas = nullptr;
    for (QGraphicsItem* item : canvasScene->items(Qt::AscendingOrder)) {
        canvas = qgraphicsitem_cast<QGraphicsRectItem*>(item);
        if (canvas) break;
    }
    if (!canvas) return;

    // Get current canvas size
    QSizeF currentSize = canvas->rect().size();

    // Create dialog
    QDialog dialog(this);
    dialog.setWindowTitle(tr("Update Canvas Size"));
    QVBoxLayout* layout = new QVBoxLayout(&dialog);

    // Width input
    QHBoxLayout* widthLayout = new QHBoxLayout;
    QLabel* widthLabel = new QLabel(tr("Width:"), &dialog);
    QSpinBox* widthSpinBox = new QSpinBox(&dialog);
    widthSpinBox->setRange(1, 10000);
    widthSpinBox->setValue(qRound(currentSize.width()));
    widthLayout->addWidget(widthLabel);
    widthLayout->addWidget(widthSpinBox);

    // Height input
    QHBoxLayout* heightLayout = new QHBoxLayout;
    QLabel* heightLabel = new QLabel(tr("Height:"), &dialog);
    QSpinBox* heightSpinBox = new QSpinBox(&dialog);
    heightSpinBox->setRange(1, 10000);
    heightSpinBox->setValue(qRound(currentSize.height()));
    heightLayout->addWidget(heightLabel);
    heightLayout->addWidget(heightSpinBox);

    // Maintain aspect ratio checkbox
    QCheckBox* maintainAspectRatio = new QCheckBox(tr("Maintain Aspect Ratio"), &dialog);
    maintainAspectRatio->setChecked(true);

    // Connect aspect ratio maintenance
    double aspectRatio = currentSize.width() / currentSize.height();
    connect(widthSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), [=](int width) {
        if (maintainAspectRatio->isChecked()) {
            heightSpinBox->blockSignals(true);
            heightSpinBox->setValue(qRound(width / aspectRatio));
            heightSpinBox->blockSignals(false);
        }
    });
    connect(heightSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), [=](int height) {
        if (maintainAspectRatio->isChecked()) {
            widthSpinBox->blockSignals(true);
            widthSpinBox->setValue(qRound(height * aspectRatio));
            widthSpinBox->blockSignals(false);
        }
    });

    // Buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout;
    QPushButton* okButton = new QPushButton(tr("OK"), &dialog);
    QPushButton* cancelButton = new QPushButton(tr("Cancel"), &dialog);
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);

    // Add all layouts to main layout
    layout->addLayout(widthLayout);
    layout->addLayout(heightLayout);
    layout->addWidget(maintainAspectRatio);
    layout->addLayout(buttonLayout);

    // Connect buttons
    connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

    // Show dialog
    if (dialog.exec() == QDialog::Accepted) {
        int newWidth = widthSpinBox->value();
        int newHeight = heightSpinBox->value();
        
        // Update canvas size
        canvas->setRect(0, 0, newWidth, newHeight);
        
        // Adjust view to fit the new canvas size
        canvasView->fitInView(canvas, Qt::KeepAspectRatio);
    }
}

void MainWindow::showBlurDialog()
{
    logFunctionCall("showBlurDialog");
    onBlurFilter();
}

void MainWindow::showLayerContextMenu(const QPoint& pos)
{
    QListWidgetItem* item = layerList->itemAt(pos);
    if (!item) return;

    QMenu contextMenu(this);
    
    // Add layer operations
    QAction* deleteAction = new QAction(tr("Delete Layer"), this);
    QAction* renameAction = new QAction(tr("Rename Layer"), this);
    QAction* duplicateAction = new QAction(tr("Duplicate Layer"), this);
    
    // Disable delete for background layer
    if (layerList->row(item) == 0) {
        deleteAction->setEnabled(false);
    }
    
    contextMenu.addAction(deleteAction);
    contextMenu.addAction(renameAction);
    contextMenu.addAction(duplicateAction);
    
    // Show menu and handle actions
    QAction* selectedAction = contextMenu.exec(layerList->mapToGlobal(pos));
    if (selectedAction == deleteAction) {
        deleteLayer();
    } else if (selectedAction == renameAction) {
        bool ok;
        QString newName = QInputDialog::getText(this, tr("Rename Layer"),
            tr("New name:"), QLineEdit::Normal, item->text(), &ok);
        if (ok && !newName.isEmpty()) {
            item->setText(newName);
            Layer* layer = static_cast<Layer*>(item->data(Qt::UserRole).value<void*>());
            if (layer) {
                layer->setName(newName);
            }
        }
    } else if (selectedAction == duplicateAction) {
        Layer* layer = static_cast<Layer*>(item->data(Qt::UserRole).value<void*>());
        if (layer) {
            QPixmap pixmap = layer->pixmap();
            QString newName = item->text() + " (copy)";
            addLayer(pixmap, newName);
        }
    }
}

void MainWindow::addEmptyLayer()
{
    logFunctionCall("addEmptyLayer");
    if (!hasCanvas()) {
        QMessageBox::warning(this, tr("Warning"),
            tr("Please create a canvas first before adding layers."));
        return;
    }

    // Find the background/canvas item to get its size
    QGraphicsRectItem* canvas = nullptr;
    for (QGraphicsItem* item : canvasScene->items(Qt::AscendingOrder)) {
        canvas = qgraphicsitem_cast<QGraphicsRectItem*>(item);
        if (canvas) break;
    }
    if (!canvas) {
        QMessageBox::warning(this, tr("Warning"), tr("No canvas found."));
        return;
    }

    // Create a new transparent layer with the same size as the canvas
    QPixmap newLayerPixmap(canvas->rect().size().toSize());
    newLayerPixmap.fill(Qt::transparent);
    
    // Add the new layer
    QString layerName = QString("Layer %1").arg(layerList->count());
    addLayer(newLayerPixmap, layerName);
}

void MainWindow::setupBrushTool()
{
    // Create brush size slider
    brushSizeSlider = new QSlider(Qt::Horizontal, this);
    brushSizeSlider->setRange(1, 50);
    brushSizeSlider->setValue(brushSize);
    brushSizeSlider->setFixedWidth(100);
    brushSizeLabel = new QLabel("Brush Size:", this);
    connect(brushSizeSlider, &QSlider::valueChanged, this, &MainWindow::onBrushSizeChanged);

    // Create color button
    brushColorButton = new QPushButton(this);
    brushColorButton->setFixedSize(30, 30);
    updateBrushPreview();
    connect(brushColorButton, &QPushButton::clicked, [this]() {
        QColor color = QColorDialog::getColor(brushColor, this, "Select Brush Color");
        if (color.isValid()) {
            brushColor = color;
            updateBrushPreview();
        }
    });

    // Add brush controls to layout
    topLayout->addWidget(brushSizeLabel);
    topLayout->addWidget(brushSizeSlider);
    topLayout->addWidget(brushColorButton);

    // Hide brush controls initially
    brushSizeSlider->setVisible(false);
    brushSizeLabel->setVisible(false);
    brushColorButton->setVisible(false);
}

void MainWindow::onBrushToolClicked()
{
    logFunctionCall("onBrushToolClicked");
    currentTool = Brush;
    rotationSlider->setVisible(false);
    rotationLabel->setVisible(false);
    widthSpinBox->setVisible(false);
    widthLabel->setVisible(false);
    heightSpinBox->setVisible(false);
    heightLabel->setVisible(false);
    brushSizeSlider->setVisible(true);
    brushSizeLabel->setVisible(true);
    brushColorButton->setVisible(true);
    updateLayerInteraction();
}

void MainWindow::onEraserToolClicked()
{
    logFunctionCall("onEraserToolClicked");
    currentTool = Eraser;
    rotationSlider->setVisible(false);
    rotationLabel->setVisible(false);
    widthSpinBox->setVisible(false);
    widthLabel->setVisible(false);
    heightSpinBox->setVisible(false);
    heightLabel->setVisible(false);
    brushSizeSlider->setVisible(true);
    brushSizeLabel->setVisible(true);
    brushColorButton->setVisible(false);
    updateLayerInteraction();
}

void MainWindow::onBrushSizeChanged(int size)
{
    brushSize = size;
    updateBrushPreview();
}

void MainWindow::onBrushColorChanged(const QColor& color)
{
    brushColor = color;
    updateBrushPreview();
}

void MainWindow::updateBrushPreview()
{
    QPixmap pixmap(brushColorButton->size());
    pixmap.fill(brushColor);
    brushColorButton->setIcon(QIcon(pixmap));
    brushColorButton->setIconSize(brushColorButton->size());
}

bool MainWindow::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == canvasView->viewport() && (currentTool == Brush || currentTool == Eraser)) {
        QListWidgetItem* currentItem = layerList->currentItem();
        if (!currentItem) return false;

        Layer* layer = static_cast<Layer*>(currentItem->data(Qt::UserRole).value<void*>());
        if (!layer) return false;

        switch (event->type()) {
            case QEvent::MouseButtonPress:
            case QEvent::MouseMove:
            case QEvent::MouseButtonRelease: {
                QMouseEvent* mouseEvent = dynamic_cast<QMouseEvent*>(event);
                if (!mouseEvent) return false;
                
                QPointF scenePos = canvasView->mapToScene(mouseEvent->pos());

                if (event->type() == QEvent::MouseButtonPress && mouseEvent->button() == Qt::LeftButton) {
                    startDrawing(scenePos);
                    return true;
                } else if (event->type() == QEvent::MouseMove && isDrawing) {
                    continueDrawing(scenePos);
                    return true;
                } else if (event->type() == QEvent::MouseButtonRelease && mouseEvent->button() == Qt::LeftButton && isDrawing) {
                    endDrawing();
                    return true;
                }
                break;
            }
            default:
                break;
        }
    }
    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::startDrawing(const QPointF& pos)
{
    logFunctionCall("startDrawing");
    QListWidgetItem* currentItem = layerList->currentItem();
    if (!currentItem) return;

    Layer* layer = static_cast<Layer*>(currentItem->data(Qt::UserRole).value<void*>());
    if (!layer) return;

    isDrawing = true;
    lastPoint = pos;

    // Create a new pixmap for drawing
    QPixmap currentLayerPixmap = layer->pixmap();
    currentPixmap = new QPixmap(currentLayerPixmap);
    currentPainter = new QPainter(currentPixmap);
    
    // Set up the painter based on the current tool
    if (currentTool == Eraser) {
        // For eraser, we'll use a black pen with CompositionMode_Clear to make areas transparent
        currentPainter->setPen(QPen(Qt::black, brushSize, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        currentPainter->setCompositionMode(QPainter::CompositionMode_Clear);
        currentPainter->setRenderHint(QPainter::Antialiasing);
        currentPainter->setRenderHint(QPainter::SmoothPixmapTransform);
    } else {
        currentPainter->setPen(QPen(brushColor, brushSize, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        currentPainter->setCompositionMode(QPainter::CompositionMode_SourceOver);
        currentPainter->setRenderHint(QPainter::Antialiasing);
        currentPainter->setRenderHint(QPainter::SmoothPixmapTransform);
    }
}

void MainWindow::continueDrawing(const QPointF& pos)
{
    logFunctionCall("continueDrawing");
    if (!isDrawing || !currentPainter) return;

    QListWidgetItem* currentItem = layerList->currentItem();
    if (!currentItem) return;

    Layer* layer = static_cast<Layer*>(currentItem->data(Qt::UserRole).value<void*>());
    if (!layer) return;

    // Convert scene coordinates to layer coordinates
    QPointF layerPos = layer->mapFromScene(pos);
    QPointF lastLayerPos = layer->mapFromScene(lastPoint);

    // Get current layer bounds
    QRectF layerBounds = currentPixmap->rect();

    // Check if points are within layer bounds
    if (!layerBounds.contains(layerPos) || !layerBounds.contains(lastLayerPos)) {
        return;
    }

    // Draw the line in layer coordinates
    currentPainter->drawLine(lastLayerPos, layerPos);
    lastPoint = pos;

    // Update the layer with the new pixmap
    layer->setPixmap(*currentPixmap);
    canvasScene->update();
}

void MainWindow::endDrawing()
{
    logFunctionCall("endDrawing");
    if (!isDrawing) return;

    isDrawing = false;
    if (currentPainter) {
        currentPainter->end();
        delete currentPainter;
        currentPainter = nullptr;
    }
    if (currentPixmap) {
        delete currentPixmap;
        currentPixmap = nullptr;
    }
}

void MainWindow::logMovement(const QString& layerName, const QPointF& oldPos, const QPointF& newPos)
{
    // Only log if the movement is significant (more than 1 pixel)
    if ((newPos - oldPos).manhattanLength() < 1.0) {
        return;
    }

    // Store operation in history
    OperationHistory history;
    history.type = OperationHistory::Type::Move;
    history.layerName = layerName;
    history.oldPosition = oldPos;
    history.newPosition = newPos;
    operationHistory.append(history);

    // Enable undo action if we have history
    undoAction->setEnabled(!operationHistory.isEmpty());

    // Log to file
    QFile logFile("log.txt");
    if (logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        QTextStream out(&logFile);
        QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
        out << timestamp << " - Layer '" << layerName << "' moved from (" 
            << oldPos.x() << ", " << oldPos.y() << ") to ("
            << newPos.x() << ", " << newPos.y() << ")\n";
        logFile.close();
    }
}

void MainWindow::logRotation(const QString& layerName, qreal oldRotation, qreal newRotation)
{
    // Only log if the rotation change is significant (more than 1 degree)
    if (qAbs(newRotation - oldRotation) < 1.0) {
        return;
    }

    // Store operation in history
    OperationHistory history;
    history.type = OperationHistory::Type::Rotate;
    history.layerName = layerName;
    history.oldRotation = oldRotation;
    history.newRotation = newRotation;
    operationHistory.append(history);

    // Enable undo action if we have history
    undoAction->setEnabled(!operationHistory.isEmpty());

    // Log to file
    QFile logFile("log.txt");
    if (logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        QTextStream out(&logFile);
        QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
        out << timestamp << " - Layer '" << layerName << "' rotated from " 
            << oldRotation << "° to " << newRotation << "°\n";
        logFile.close();
    }
}

void MainWindow::logResize(const QString& layerName, const QSizeF& oldSize, const QSizeF& newSize)
{
    // Only log if the size change is significant (more than 1 pixel)
    if (qAbs(newSize.width() - oldSize.width()) < 1.0 && 
        qAbs(newSize.height() - oldSize.height()) < 1.0) {
        return;
    }

    // Store operation in history
    OperationHistory history;
    history.type = OperationHistory::Type::Resize;
    history.layerName = layerName;
    history.oldSize = oldSize;
    history.newSize = newSize;
    operationHistory.append(history);

    // Enable undo action if we have history
    undoAction->setEnabled(!operationHistory.isEmpty());

    // Log to file
    QFile logFile("log.txt");
    if (logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        QTextStream out(&logFile);
        QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
        out << timestamp << " - Layer '" << layerName << "' resized from (" 
            << oldSize.width() << "x" << oldSize.height() << ") to ("
            << newSize.width() << "x" << newSize.height() << ")\n";
        logFile.close();
    }
}

void MainWindow::undoLastOperation()
{
    if (operationHistory.isEmpty()) {
        return;
    }

    // Get the last operation from history
    OperationHistory lastOperation = operationHistory.takeLast();

    // Find the layer by name
    for (int i = 0; i < layerList->count(); ++i) {
        QListWidgetItem* item = layerList->item(i);
        Layer* layer = static_cast<Layer*>(item->data(Qt::UserRole).value<void*>());
        if (layer && layer->getName() == lastOperation.layerName) {
            // Undo the operation based on its type
            switch (lastOperation.type) {
                case OperationHistory::Type::Move:
                    layer->setPos(lastOperation.oldPosition);
                    break;
                case OperationHistory::Type::Rotate:
                    layer->setRotation(lastOperation.oldRotation);
                    break;
                case OperationHistory::Type::Resize:
                    layer->setSize(lastOperation.oldSize.width(), lastOperation.oldSize.height());
                    break;
            }
            
            // Log the undo operation
            QFile logFile("log.txt");
            if (logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
                QTextStream out(&logFile);
                QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
                out << timestamp << " - UNDO: Layer '" << lastOperation.layerName;
                switch (lastOperation.type) {
                    case OperationHistory::Type::Move:
                        out << "' moved back to (" << lastOperation.oldPosition.x() 
                            << ", " << lastOperation.oldPosition.y() << ")";
                        break;
                    case OperationHistory::Type::Rotate:
                        out << "' rotated back to " << lastOperation.oldRotation << "°";
                        break;
                    case OperationHistory::Type::Resize:
                        out << "' resized back to " << lastOperation.oldSize.width() 
                            << "x" << lastOperation.oldSize.height();
                        break;
                }
                out << "\n";
                logFile.close();
            }
            break;
        }
    }

    // Disable undo action if no more history
    undoAction->setEnabled(!operationHistory.isEmpty());
}

void MainWindow::onHueFilter()
{
    logFunctionCall("onHueFilter");
    QListWidgetItem *currentItem = layerList->currentItem();
    if (!currentItem) {
        QMessageBox::warning(this, tr("No Layer Selected"), tr("Please select a layer to adjust hue."));
        return;
    }
    Layer* layer = static_cast<Layer*>(currentItem->data(Qt::UserRole).value<void*>());
    if (!layer) {
        QMessageBox::warning(this, tr("No Layer Selected"), tr("Please select a valid layer to adjust hue."));
        return;
    }

    QDialog dialog(this);
    dialog.setWindowTitle(tr("Hue Adjustment"));
    QVBoxLayout* layout = new QVBoxLayout(&dialog);

    // Add slider for hue
    QLabel* label = new QLabel(tr("Hue Shift:"), &dialog);
    QSlider* slider = new QSlider(Qt::Horizontal, &dialog);
    slider->setRange(-180, 180);
    slider->setValue(0);
    slider->setTickInterval(30);
    slider->setTickPosition(QSlider::TicksBelow);

    // Add value label
    QLabel* valueLabel = new QLabel("0°", &dialog);
    connect(slider, &QSlider::valueChanged, [=](int value) {
        valueLabel->setText(QString::number(value) + "°");
    });

    // Add preview checkbox
    QCheckBox* previewCheck = new QCheckBox(tr("Live Preview"), &dialog);
    previewCheck->setChecked(true);

    // Add buttons
    QPushButton* okButton = new QPushButton(tr("OK"), &dialog);
    QPushButton* cancelButton = new QPushButton(tr("Cancel"), &dialog);

    // Layout
    QHBoxLayout* sliderLayout = new QHBoxLayout;
    sliderLayout->addWidget(slider);
    sliderLayout->addWidget(valueLabel);

    QHBoxLayout* buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);

    layout->addWidget(label);
    layout->addLayout(sliderLayout);
    layout->addWidget(previewCheck);
    layout->addLayout(buttonLayout);

    // Connect signals
    connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

    // Store original pixmap for preview
    QPixmap originalPixmap = layer->pixmap();
    QMetaObject::Connection previewConnection;

    // Function to apply hue preview
    auto applyPreview = [=](int value) {
        QPixmap previewPixmap;
        applyHue(originalPixmap, previewPixmap, value);
        layer->setPixmap(previewPixmap);
    };

    // Connect preview checkbox
    connect(previewCheck, &QCheckBox::toggled, [&previewConnection, slider, applyPreview, layer, originalPixmap](bool checked) {
        if (checked) {
            // Connect slider for live preview
            previewConnection = connect(slider, &QSlider::valueChanged, applyPreview);
            // Apply current value
            applyPreview(slider->value());
        } else {
            // Disconnect slider and restore original
            disconnect(previewConnection);
            layer->setPixmap(originalPixmap);
        }
    });

    // Initial preview if checked
    if (previewCheck->isChecked()) {
        previewConnection = connect(slider, &QSlider::valueChanged, applyPreview);
        applyPreview(slider->value());
    }

    // Show dialog
    if (dialog.exec() == QDialog::Accepted) {
        // Final application of hue
        QPixmap finalPixmap;
        applyHue(originalPixmap, finalPixmap, slider->value());
        layer->setPixmap(finalPixmap);
    } else {
        // Restore original pixmap if cancelled
        layer->setPixmap(originalPixmap);
    }
}

void MainWindow::applyHue(const QPixmap& input, QPixmap& output, int hueShift)
{
    cv::Mat inputMat = Layer::pixmapToMat(input);
    cv::Mat outputMat;

    // Convert to HSV color space
    cv::Mat hsv;
    cv::cvtColor(inputMat, hsv, cv::COLOR_BGRA2BGR);
    cv::cvtColor(hsv, hsv, cv::COLOR_BGR2HSV);

    // Split channels
    std::vector<cv::Mat> channels;
    cv::split(hsv, channels);

    // Create a matrix of the same size filled with the hue shift value
    cv::Mat shiftMat = cv::Mat::zeros(channels[0].size(), channels[0].type());
    shiftMat.setTo(hueShift);

    // Add the shift to the hue channel
    cv::add(channels[0], shiftMat, channels[0]);

    // Ensure hue values stay in valid range (0-179)
    cv::Mat mask = channels[0] < 0;
    channels[0].setTo(179, mask);
    mask = channels[0] > 179;
    channels[0].setTo(0, mask);

    // Merge channels back
    cv::merge(channels, hsv);

    // Convert back to BGR
    cv::cvtColor(hsv, outputMat, cv::COLOR_HSV2BGR);
    cv::cvtColor(outputMat, outputMat, cv::COLOR_BGR2BGRA);

    output = Layer::matToPixmap(outputMat);
}

void MainWindow::onSelectiveColorFilter()
{
    logFunctionCall("onSelectiveColorFilter");
    QListWidgetItem *currentItem = layerList->currentItem();
    if (!currentItem) {
        QMessageBox::warning(this, tr("No Layer Selected"), tr("Please select a layer to adjust colors."));
        return;
    }
    Layer* layer = static_cast<Layer*>(currentItem->data(Qt::UserRole).value<void*>());
    if (!layer) {
        QMessageBox::warning(this, tr("No Layer Selected"), tr("Please select a valid layer to adjust colors."));
        return;
    }

    QDialog dialog(this);
    dialog.setWindowTitle(tr("Selective Color Adjustment"));
    QVBoxLayout* layout = new QVBoxLayout(&dialog);

    // Create sliders for each color
    struct ColorSlider {
        QLabel* label;
        QSlider* slider;
        QLabel* valueLabel;
    };

    std::vector<ColorSlider> colorSliders = {
        {new QLabel(tr("Red:"), &dialog), new QSlider(Qt::Horizontal, &dialog), new QLabel("0°", &dialog)},
        {new QLabel(tr("Green:"), &dialog), new QSlider(Qt::Horizontal, &dialog), new QLabel("0°", &dialog)},
        {new QLabel(tr("Blue:"), &dialog), new QSlider(Qt::Horizontal, &dialog), new QLabel("0°", &dialog)},
        {new QLabel(tr("Cyan:"), &dialog), new QSlider(Qt::Horizontal, &dialog), new QLabel("0°", &dialog)},
        {new QLabel(tr("Magenta:"), &dialog), new QSlider(Qt::Horizontal, &dialog), new QLabel("0°", &dialog)},
        {new QLabel(tr("Yellow:"), &dialog), new QSlider(Qt::Horizontal, &dialog), new QLabel("0°", &dialog)}
    };

    // Setup each slider
    for (auto& cs : colorSliders) {
        cs.slider->setRange(-180, 180);
        cs.slider->setValue(0);
        cs.slider->setTickInterval(30);
        cs.slider->setTickPosition(QSlider::TicksBelow);
        
        QHBoxLayout* sliderLayout = new QHBoxLayout;
        sliderLayout->addWidget(cs.label);
        sliderLayout->addWidget(cs.slider);
        sliderLayout->addWidget(cs.valueLabel);
        layout->addLayout(sliderLayout);

        connect(cs.slider, &QSlider::valueChanged, [=](int value) {
            cs.valueLabel->setText(QString::number(value) + "°");
        });
    }

    // Add preview checkbox
    QCheckBox* previewCheck = new QCheckBox(tr("Live Preview"), &dialog);
    previewCheck->setChecked(true);

    // Add buttons
    QPushButton* okButton = new QPushButton(tr("OK"), &dialog);
    QPushButton* cancelButton = new QPushButton(tr("Cancel"), &dialog);
    QHBoxLayout* buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);

    layout->addWidget(previewCheck);
    layout->addLayout(buttonLayout);

    // Connect signals
    connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

    // Store original pixmap for preview
    QPixmap originalPixmap = layer->pixmap();
    QMetaObject::Connection previewConnection;

    // Function to apply color preview
    auto applyPreview = [=]() {
        QPixmap previewPixmap;
        applySelectiveColor(originalPixmap, previewPixmap,
            colorSliders[0].slider->value(),  // Red
            colorSliders[1].slider->value(),  // Green
            colorSliders[2].slider->value(),  // Blue
            colorSliders[3].slider->value(),  // Cyan
            colorSliders[4].slider->value(),  // Magenta
            colorSliders[5].slider->value()   // Yellow
        );
        layer->setPixmap(previewPixmap);
    };

    // Connect preview checkbox
    connect(previewCheck, &QCheckBox::toggled, [&previewConnection, &colorSliders, applyPreview, layer, originalPixmap](bool checked) {
        if (checked) {
            // Connect all sliders for live preview
            for (const auto& cs : colorSliders) {
                previewConnection = connect(cs.slider, &QSlider::valueChanged, [=]() { applyPreview(); });
            }
            // Apply current values
            applyPreview();
        } else {
            // Disconnect sliders and restore original
            disconnect(previewConnection);
            layer->setPixmap(originalPixmap);
        }
    });

    // Initial preview if checked
    if (previewCheck->isChecked()) {
        for (const auto& cs : colorSliders) {
            previewConnection = connect(cs.slider, &QSlider::valueChanged, [=]() { applyPreview(); });
        }
        applyPreview();
    }

    // Show dialog
    if (dialog.exec() == QDialog::Accepted) {
        // Final application of color adjustments
        QPixmap finalPixmap;
        applySelectiveColor(originalPixmap, finalPixmap,
            colorSliders[0].slider->value(),  // Red
            colorSliders[1].slider->value(),  // Green
            colorSliders[2].slider->value(),  // Blue
            colorSliders[3].slider->value(),  // Cyan
            colorSliders[4].slider->value(),  // Magenta
            colorSliders[5].slider->value()   // Yellow
        );
        layer->setPixmap(finalPixmap);
    } else {
        // Restore original pixmap if cancelled
        layer->setPixmap(originalPixmap);
    }
}

void MainWindow::applySelectiveColor(const QPixmap& input, QPixmap& output, 
                                   int redHue, int greenHue, int blueHue,
                                   int cyanHue, int magentaHue, int yellowHue)
{
    cv::Mat inputMat = Layer::pixmapToMat(input);
    cv::Mat outputMat;

    // Convert to HSV color space
    cv::Mat hsv;
    cv::cvtColor(inputMat, hsv, cv::COLOR_BGRA2BGR);
    cv::cvtColor(hsv, hsv, cv::COLOR_BGR2HSV);

    // Split channels
    std::vector<cv::Mat> channels;
    cv::split(hsv, channels);

    // Define color ranges in HSV
    struct ColorRange {
        int minHue;
        int maxHue;
        int adjustment;
    };

    std::vector<ColorRange> colorRanges = {
        {0, 30, redHue},      // Red
        {30, 90, greenHue},   // Green
        {90, 150, blueHue},   // Blue
        {90, 120, cyanHue},   // Cyan
        {150, 180, magentaHue}, // Magenta
        {30, 60, yellowHue}   // Yellow
    };

    // Apply adjustments to each color range
    cv::Mat& hue = channels[0];
    for (const auto& range : colorRanges) {
        cv::Mat mask;
        cv::inRange(hue, range.minHue, range.maxHue, mask);
        
        // Create adjustment matrix
        cv::Mat adjustment = cv::Mat::zeros(hue.size(), hue.type());
        adjustment.setTo(range.adjustment, mask);
        
        // Apply adjustment
        cv::add(hue, adjustment, hue, mask);
        
        // Ensure values stay in valid range
        cv::Mat overflowMask = hue > 179;
        hue.setTo(0, overflowMask);
        cv::Mat underflowMask = hue < 0;
        hue.setTo(179, underflowMask);
    }

    // Merge channels back
    cv::merge(channels, hsv);

    // Convert back to BGR
    cv::cvtColor(hsv, outputMat, cv::COLOR_HSV2BGR);
    cv::cvtColor(outputMat, outputMat, cv::COLOR_BGR2BGRA);

    output = Layer::matToPixmap(outputMat);
}
