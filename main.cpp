#include "mainwindow.h"
#include <QApplication>
#include <QSurfaceFormat>

int main(int argc, char *argv[])
{
    QSurfaceFormat glFormat;
    glFormat.setVersion( 4, 1 );
    glFormat.setProfile( QSurfaceFormat::CoreProfile );
    //glFormat.setSamples( 0 );
    glFormat.setSwapInterval( 1 );

    glFormat.setOption(QSurfaceFormat::DebugContext);

    QSurfaceFormat::setDefaultFormat(glFormat);

    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    return a.exec();
}
