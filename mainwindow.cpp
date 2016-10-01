#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "vrview.h"

#include <QVBoxLayout>
#include <QSurfaceFormat>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    vr = new VRView( this );

    connect(vr, SIGNAL(deviceIdentifier(QString)), this, SLOT(setWindowTitle(QString)));
    connect(vr, SIGNAL(framesPerSecond(float)), this, SLOT(showFramerate(float)));

    ui->setupUi(this);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(vr);

    ui->centralWidget->setLayout(layout);
}

MainWindow::~MainWindow()
{
    delete ui;
    delete vr;
}

void MainWindow::showFramerate(float fps)
{
    statusBar()->showMessage(QString("FPS: %1").arg(fps));
}
