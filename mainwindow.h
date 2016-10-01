#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

class VRView;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

protected slots:
    void showFramerate(float fps);

private slots:
    void on_action_Load_Panorama_triggered();

private:
    Ui::MainWindow *ui;
    VRView *vr;
};

#endif // MAINWINDOW_H
