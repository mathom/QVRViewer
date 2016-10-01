#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "vrview.h"

#include <QVBoxLayout>
#include <QSurfaceFormat>
#include <QSettings>
#include <QFileDialog>
#include <QStandardPaths>
#include <QOffscreenSurface>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    vr = new VRView(this);

    connect(vr, SIGNAL(deviceIdentifier(QString)), this, SLOT(setWindowTitle(QString)));
    connect(vr, SIGNAL(framesPerSecond(float)), this, SLOT(showFramerate(float)));

    ui->setupUi(this);
    //vr->makeCurrent();
    //vr->show();
    ui->rightLayout->addWidget(vr);
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

void MainWindow::on_action_Load_Panorama_triggered()
{
    QSettings settings;

    QFileDialog fileDialog;
    fileDialog.setFileMode(QFileDialog::ExistingFile);
    fileDialog.setNameFilter("Images (*.png *.jpg)");

    if (settings.value("Load/PanoramaDir").isValid())
    {
        fileDialog.setDirectory(settings.value("Load/PanoramaDir").toString());
    }
    else
    {
        fileDialog.setDirectory(QStandardPaths::standardLocations(QStandardPaths::PicturesLocation).first());
    }

    if (fileDialog.exec())
    {
        settings.setValue("Load/PanoramaDir", fileDialog.directory().path());
        qDebug() << fileDialog.saveState();
        qDebug() << settings.value("Load/PanoramaDir").value<QByteArray>();
        vr->loadPanorama(fileDialog.selectedFiles().first());
    }
}
