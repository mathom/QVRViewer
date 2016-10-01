#include "mainwindow.h"
#include <QApplication>
#include <QSurfaceFormat>
#include <QSettings>
#include <QDebug>

int main(int argc, char *argv[])
{
    QCoreApplication::setOrganizationName("Skeletonbrain");
    QCoreApplication::setOrganizationDomain("skeletonbrain.com");
    QCoreApplication::setApplicationName("QVRViewer");

    QSettings settings;
    qDebug() << settings.value("Load/PanoramaDialog").value<QByteArray>();

    QSurfaceFormat glFormat;
    glFormat.setVersion( 4, 1 );
    glFormat.setProfile( QSurfaceFormat::CoreProfile );
    //glFormat.setSamples( 0 );
    glFormat.setSwapInterval(0);
    glFormat.setSwapBehavior(QSurfaceFormat::SingleBuffer);

    glFormat.setOption(QSurfaceFormat::DebugContext);

    QSurfaceFormat::setDefaultFormat(glFormat);

    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    return a.exec();
}
