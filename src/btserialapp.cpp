#include "btserialapp.h"
#include <QtCore>
#include <iostream>
#include <QCoreApplication>


BtSerialApp::BtSerialApp(QObject *parent) : QObject(parent)
{
  app = QCoreApplication::instance();
  this->console = new ConsoleReader(this);
  bleServer = new BtLESerialServer(this->console);

  QObject::connect(bleServer, SIGNAL(done()), this, SLOT(aboutToQuitApp()));
}

BtSerialApp::~BtSerialApp()
{
}

void BtSerialApp::run(){
  qDebug("Starting bluetooth LE");
  if (bleServer->startServer(QCoreApplication::arguments()) == -1)
  {
    qWarning() << "Cannot start ble serial server! Application will exit...";
    emit done();
    return;
  }
  qDebug("Started");
  qInfo("bleSerialServer now running!");
}

void BtSerialApp::aboutToQuitApp()
{
  qDebug("about to Quit!");
  qDebug("ble stop!");
  bleServer->stopServer();
  qDebug("ble delete!");
  delete bleServer;
  qDebug("console delete!");
  delete this->console;
  qDebug("almost done!");
  emit done();
  qDebug("done");
}
