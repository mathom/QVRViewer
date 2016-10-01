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
    ui->rightLayout->addWidget(vr);

    connect(vr, SIGNAL(statusMessage(QString)), this, SLOT(showStatus(QString)));
}

MainWindow::~MainWindow()
{
    delete ui;
    delete vr;
}

void MainWindow::showFramerate(float fps)
{
    ui->fpsLabel->setText(tr("%1 FPS").arg(fps));
}

void MainWindow::showStatus(const QString &message)
{
    ui->statusBar->showMessage(message);
}

void MainWindow::on_action_Load_Panorama_triggered()
{
    QSettings settings;

    QFileDialog fileDialog;
    fileDialog.setFileMode(QFileDialog::ExistingFile);
    fileDialog.setNameFilter("Images (*.png *.jpg)");

    if (settings.value("Load/PanoramaDir").isValid())
        fileDialog.setDirectory(settings.value("Load/PanoramaDir").toString());
    else
        fileDialog.setDirectory(QStandardPaths::standardLocations(QStandardPaths::PicturesLocation).first());

    if (fileDialog.exec())
    {
        settings.setValue("Load/PanoramaDir", fileDialog.directory().path());
        vr->loadPanorama(fileDialog.selectedFiles().first());
    }
}
