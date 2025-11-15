#include "app_controller.h"
#include <QDebug>

AppController::AppController(QObject *parent)
    : QObject(parent)
{
}

void AppController::startupMessage()
{
    qDebug() << "Smart irrigation app has started!";
}
