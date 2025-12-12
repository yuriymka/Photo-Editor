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
#include "layer.h"

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

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void createNewCanvas();
    void openImage();
    void updateLayerList();
    void toggleLayerVisibility(QListWidgetItem* item);
    void layerSelectionChanged();

private:
    Ui::MainWindow *ui;
    QGraphicsView *canvasView;
    QGraphicsScene *canvasScene;
    QListWidget *layerList;
    QDockWidget *layerDock;
    void setupMenu();
    void setupLayerDock();
    bool hasCanvas() const;
    void addLayer(const QPixmap& pixmap, const QString& name);
};
#endif // MAINWINDOW_H
