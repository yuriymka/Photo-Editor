QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    layer.cpp \
    layerwidget.cpp \
    canvasdialog.cpp

# OpenCV configuration
INCLUDEPATH += M:/opencv/include
INCLUDEPATH += M:/opencvinclude/opencv2

# For MinGW compiler
LIBS += -LM:/opencv/x64/mingw/bin \
    -lopencv_core455 \
    -lopencv_imgproc455 \
    -lopencv_highgui455 \
    -lopencv_imgcodecs455


# For release configuration
CONFIG(release, debug|release) {
    LIBS += -lopencv_core455 \
            -lopencv_imgproc455 \
            -lopencv_highgui455 \
            -lopencv_imgcodecs455
}

HEADERS += \
    mainwindow.h \
    layer.h \
    layerwidget.h \
    canvasdialog.h

FORMS += \
    mainwindow.ui

RESOURCES += resources.qrc

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
