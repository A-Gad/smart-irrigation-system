#include <QApplication>
#include <QLabel>
#include "app_controller.h" 
#include "mainwindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    MainWindow w;
    AppController controller;

    controller.startupMessage();

    w.show();
    return app.exec();
}



