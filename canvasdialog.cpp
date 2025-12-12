#include "mainwindow.h"

CanvasDialog::CanvasDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("New Canvas"));
    
    // Create spin boxes for width and height
    widthSpinBox = new QSpinBox(this);
    heightSpinBox = new QSpinBox(this);
    
    // Set reasonable ranges and default values
    widthSpinBox->setRange(100, 4000);
    heightSpinBox->setRange(100, 4000);
    widthSpinBox->setValue(800);
    heightSpinBox->setValue(600);
    
    // Create labels
    QLabel *widthLabel = new QLabel(tr("Width:"), this);
    QLabel *heightLabel = new QLabel(tr("Height:"), this);
    
    // Create buttons
    QPushButton *okButton = new QPushButton(tr("OK"), this);
    QPushButton *cancelButton = new QPushButton(tr("Cancel"), this);
    
    // Connect buttons
    connect(okButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    
    // Create layouts
    QHBoxLayout *widthLayout = new QHBoxLayout;
    widthLayout->addWidget(widthLabel);
    widthLayout->addWidget(widthSpinBox);
    
    QHBoxLayout *heightLayout = new QHBoxLayout;
    heightLayout->addWidget(heightLabel);
    heightLayout->addWidget(heightSpinBox);
    
    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    
    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addLayout(widthLayout);
    mainLayout->addLayout(heightLayout);
    mainLayout->addLayout(buttonLayout);
    
    setLayout(mainLayout);
} 