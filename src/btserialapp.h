#ifndef BTSERIALAPP_H
#define BTSERIALAPP_H

#include <QtCore>
#include <QObject>
#include "consolereader.h"
#include "btleserialserver.h"

class btSerialApp : public QObject
{
    Q_OBJECT
  public:
    explicit btSerialApp(QObject *parent = 0);
    ~btSerialApp();

  signals:
    void done();

  public slots:
    void run();
    void aboutToQuitApp();
  private:
    BtLESerialServer* bleServer;
    QCoreApplication *app;
    ConsoleReader* console;
};

#endif // BTSERIALAPP_H
