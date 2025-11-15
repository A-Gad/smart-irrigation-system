#include <QApplication>
#include <QLabel>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    QLabel label("Hello Smart Irrigation System!");
    label.show();
    return app.exec();
}



