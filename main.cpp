#include "mainwindow.h"
#include <QApplication>
#include <QSurfaceFormat>
#include <QSettings>

int main(int argc, char *argv[])
{
    QCoreApplication::setOrganizationName("Skeletonbrain");
    QCoreApplication::setOrganizationDomain("skeletonbrain.com");
    QCoreApplication::setApplicationName("QVRViewer");

    QSurfaceFormat glFormat;
    glFormat.setVersion(4, 1);
    glFormat.setProfile(QSurfaceFormat::CoreProfile);
    //glFormat.setSamples(0);
    glFormat.setSwapInterval(0);

    glFormat.setOption(QSurfaceFormat::DebugContext);

    QSurfaceFormat::setDefaultFormat(glFormat);

    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    return a.exec();
}
