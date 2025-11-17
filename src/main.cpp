#include <QApplication>
#include "app_controller.h" 
#include "mainwindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    
    // Create controller first
    AppController controller;
    controller.startupMessage();
    
    // Pass controller to MainWindow so they can communicate
    MainWindow w(&controller);
    
    w.show();
    return app.exec();
}