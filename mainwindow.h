#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMenu>
#include <QAction>
#include <QDialog>
#include <QSpinBox>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QFileDialog>
#include <QMessageBox>
#include <QListWidget>
#include <QDockWidget>
#include <QSlider>
#include <QToolButton>
#include <QButtonGroup>
#include <QToolBar>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QList>
#include <QTranslator>
#include <QSettings>
#include "layer.h"
#include "layerwidget.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class CanvasDialog : public QDialog {
    Q_OBJECT
public:
    CanvasDialog(QWidget *parent = nullptr);
    int getWidth() const { return widthSpinBox->value(); }
    int getHeight() const { return heightSpinBox->value(); }

private:
    QSpinBox *widthSpinBox;
    QSpinBox *heightSpinBox;
};

// Structure to store operation history
struct OperationHistory {
    enum class Type { Move, Rotate, Resize };
    Type type;
    QString layerName;
    QPointF oldPosition;
    QPointF newPosition;
    qreal oldRotation;
    qreal newRotation;
    QSizeF oldSize;
    QSizeF newSize;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void createNewCanvas();
    void createNewCanvasFromImage();
    void openImage();
    void saveImage();
    void saveProject();
    void openProject();
    void addLayer(const QPixmap& pixmap, const QString& name);
    void toggleLayerVisibility(QListWidgetItem* item);
    void layerSelectionChanged();
    void onGlobalOpacitySliderChanged(int value);
    void onSceneSelectionChanged();
    void onMoveToolClicked();
    void onResizeToolClicked();
    void onRotateToolClicked();
    void onBrushToolClicked();
    void onEraserToolClicked();
    void onRotationSliderChanged(int value);
    void onWidthSpinChanged(int value);
    void onHeightSpinChanged(int value);
    void onWidthSpinEditingFinished();
    void onHeightSpinEditingFinished();
    void onBlurFilter();
    void onSaturationFilter();
    void createActions();
    void createToolBar();
    void deleteLayer();
    void updateLayerPanel();
    void showResizeDialog();
    void toggleGrid();
    void addNewLayer();
    void applySaturation();
    void updateCanvasSize();
    void showBlurDialog();
    void showLayerContextMenu(const QPoint& pos);
    void addEmptyLayer();
    void onBrushSizeChanged(int size);
    void onBrushColorChanged(const QColor& color);
    void onBrightnessFilter();
    void onContrastFilter();
    void onGrayscaleFilter();
    void onInvertFilter();
    void undoLastOperation();
    void onCartoonFilter();
    void onVintageFilter();
    void onEdgeFilter();
    void onHueFilter();
    void onSelectiveColorFilter();
    void showSettings();
    void changeLanguage(const QString& language);
    void changeFileExtension(const QString& extension);
    void loadSettings();
    void saveSettings();

private:
    Ui::MainWindow *ui;
    QGraphicsView *canvasView;
    QGraphicsScene *canvasScene;
    QListWidget *layerList;
    QDockWidget *layerDock;
    QSlider* globalOpacitySlider;
    QLabel* globalOpacityLabel;
    QToolButton* moveToolButton;
    QToolButton* resizeToolButton;
    QToolButton* rotateToolButton;
    QToolButton* brushToolButton;
    QToolButton* eraserToolButton;
    QButtonGroup* toolButtonGroup;
    QSlider* rotationSlider;
    QLabel* rotationLabel;
    QSpinBox* widthSpinBox;
    QSpinBox* heightSpinBox;
    QLabel* widthLabel;
    QLabel* heightLabel;
    QSlider* brushSizeSlider;
    QLabel* brushSizeLabel;
    QPushButton* brushColorButton;
    QHBoxLayout* topLayout;
    enum Tool { None, Move, Resize, Rotate, Brush, Eraser } currentTool;

    // Brush properties
    int brushSize;
    QColor brushColor;
    bool isDrawing;
    QPointF lastPoint;
    QPixmap* currentPixmap;
    QPainter* currentPainter;

    // Actions
    QAction* openAction;
    QAction* saveAction;
    QAction* addLayerAction;
    QAction* deleteLayerAction;
    QAction* gridAction;
    QAction* blurAction;
    QAction* saturationAction;
    QAction* brightnessAction;
    QAction* contrastAction;
    QAction* grayscaleAction;
    QAction* invertAction;
    QAction* resizeAction;
    QAction* newCanvasAction;
    QAction* newCanvasFromImageAction;
    QAction* undoAction;
    QAction* saveProjectAction;
    QAction* openProjectAction;
    QAction* settingsAction;
    QToolBar* toolBar;

    // Add undo-related members
    QList<OperationHistory> operationHistory;

    QTranslator* translator;
    QString currentLanguage;
    QString projectFileExtension;
    QSettings* appSettings;

    void setupMenu();
    void setupLayerDock();
    bool hasCanvas() const;
    void updateLayerInteraction();
    void updateResizeSpinBoxes();
    void setupBrushTool();
    void startDrawing(const QPointF& pos);
    void continueDrawing(const QPointF& pos);
    void endDrawing();
    void updateBrushPreview();
    bool eventFilter(QObject* watched, QEvent* event) override;

    // Layer update functions
    void updateLayerVisibility(int index, bool visible);
    void updateLayerOpacity(int index, int value);
    void updateLayerName(int index, const QString& name);
    void updateLayerSelection(int index);
    void updateLayerOrder(int fromIndex, int toIndex);

    // Layer panel item update functions
    void updateLayerPanelItem(int index, Layer* layer);
    void updateLayerPanelItemVisibility(int index, bool visible);
    void updateLayerPanelItemOpacity(int index, int value);
    void updateLayerPanelItemName(int index, const QString& name);
    void updateLayerPanelItemSelection(int index, bool selected);
    void updateLayerPanelItemOrder(int fromIndex, int toIndex);
    void updateLayerPanelItemSize(int index, const QSize& size);
    void updateLayerPanelItemPosition(int index, const QPointF& position);
    void updateLayerPanelItemRotation(int index, qreal rotation);
    void updateLayerPanelItemScale(int index, qreal scale);
    void updateLayerPanelItemTransform(int index, const QTransform& transform);
    void updateLayerPanelItemPixmap(int index, const QPixmap& pixmap);
    void updateLayerPanelItemOriginalPixmapSize(int index, const QSizeF& size);
    void updateLayerPanelItemTargetSize(int index, const QSizeF& size);
    void updateLayerPanelItemOriginalPixmap(int index, const QPixmap& pixmap);
    void updateLayerPanelItemTargetPixmap(int index, const QPixmap& pixmap);
    void updateLayerPanelItemOriginalPixmapRect(int index, const QRectF& rect);
    void updateLayerPanelItemTargetPixmapRect(int index, const QRectF& rect);
    void updateLayerPanelItemOriginalPixmapPos(int index, const QPointF& pos);
    void updateLayerPanelItemTargetPixmapPos(int index, const QPointF& pos);
    void updateLayerPanelItemOriginalPixmapTransform(int index, const QTransform& transform);
    void updateLayerPanelItemTargetPixmapTransform(int index, const QTransform& transform);
    void updateLayerPanelItemOriginalPixmapOpacity(int index, qreal opacity);
    void updateLayerPanelItemTargetPixmapOpacity(int index, qreal opacity);
    void updateLayerPanelItemOriginalPixmapVisible(int index, bool visible);
    void updateLayerPanelItemTargetPixmapVisible(int index, bool visible);
    void updateLayerPanelItemOriginalPixmapSelected(int index, bool selected);
    void updateLayerPanelItemTargetPixmapSelected(int index, bool selected);
    void updateLayerPanelItemOriginalPixmapOrder(int index, int order);
    void updateLayerPanelItemTargetPixmapOrder(int index, int order);

    // Add logging function
    void logFunctionCall(const QString& functionName);
    void logMovement(const QString& layerName, const QPointF& oldPos, const QPointF& newPos);
    void logRotation(const QString& layerName, qreal oldRotation, qreal newRotation);
    void logResize(const QString& layerName, const QSizeF& oldSize, const QSizeF& newSize);

    void showFilterPreviewDialog(const QString& title, 
                               std::function<void(const QPixmap&, QPixmap&)> filterFunction,
                               const QString& filterName);
    void applyGrayscale(const QPixmap& input, QPixmap& output);
    void applyCartoon(const QPixmap& input, QPixmap& output);
    void applyVintage(const QPixmap& input, QPixmap& output);
    void applyEdge(const QPixmap& input, QPixmap& output);
    void applyHue(const QPixmap& input, QPixmap& output, int hueShift);
    void applySelectiveColor(const QPixmap& input, QPixmap& output, 
                           int redHue, int greenHue, int blueHue,
                           int cyanHue, int magentaHue, int yellowHue);

    void setupSettingsMenu();
    void applyLanguage(const QString& language);
    void applyFileExtension(const QString& extension);
};
#endif // MAINWINDOW_H
