#ifndef APP_CONTROLLER_H
#define APP_CONTROLLER_H

#include <QObject>

class AppController : public QObject
{
    Q_OBJECT

public:
    AppController(QObject *parent = nullptr);
    void startupMessage();
};

#endif
