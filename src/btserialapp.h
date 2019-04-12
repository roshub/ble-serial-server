#ifndef BTSERIALAPP_H
#define BTSERIALAPP_H

#include <QtCore>
#include <QObject>
#include "consolereader.h"
#include "btleserialserver.h"

class BtSerialApp : public QObject
{
    Q_OBJECT
  public:
    explicit BtSerialApp(QObject *parent = 0);
    ~BtSerialApp();

  signals:
    void done();

  public slots:
    void run(QMap<QString,QString> idList,  QMap<QString,unsigned char> typeList );
    void aboutToQuitApp();
  private:
    BtLESerialServer* bleServer;
    QCoreApplication *app;
    ConsoleReader* console;
};

#endif // BTSERIALAPP_H
