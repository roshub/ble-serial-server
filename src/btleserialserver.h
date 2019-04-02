#ifndef BTLESERIALSERVER_H
#define BTLESERIALSERVER_H

#include <QObject>

#include <QtBluetooth/qlowenergyadvertisingdata.h>
#include <QtBluetooth/qlowenergyadvertisingparameters.h>
#include <QtBluetooth/qlowenergycharacteristic.h>
#include <QtBluetooth/qlowenergycharacteristicdata.h>
#include <QtBluetooth/qlowenergydescriptordata.h>
#include <QtBluetooth/qlowenergycontroller.h>
#include <QtBluetooth/qlowenergyservice.h>
#include <QtBluetooth/qlowenergyservicedata.h>
#include <QtCore/qbytearray.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qlist.h>
#include <QtCore/qscopedpointer.h>
#include <QtCore/qtimer.h>

#include "consolereader.h"
#include "btlecommand.h"

enum ValueChange { ValueUp, ValueDown };

// QUuid("00001101-0000-1000-8000-00805F9B34FB")
#define ROSHUB_SERVICE_UUID ((quint32) 0x00011311)
#define ROSHUB_TX_CHAR_UUID QUuid("00000003-abcd-42d0-bc79-df8168b55f04")
#define ROSHUB_RX_CHAR_UUID QUuid("00000004-abcd-42d0-bc79-df8168b55f04")

#define ROSHUB_INFO_SERVICE_UUID ((quint32) 0x00011311)
#define ROSHUB_OWNER_CHAR_UUID QUuid("00000001-abcd-42d0-bc79-df8168b55f04")
#define ROSHUB_ID_CHAR_UUID QUuid("00000002-abcd-42d0-bc79-df8168b55f04")

#define ROSHUB_OWNER_ID QByteArray::fromHex("5b8ddbb8d83bb0136e15a060")
#define ROSHUB_DEVICE_ID QByteArray::fromHex("5b8ddbb9d83bb0136e15a067")


class BtLESerialServer : public QObject
{
   

    Q_OBJECT

  public:

   enum RosHub_Types {
      user     = 0x1,
      team     = 0x2,
      org      = 0x3,
      device   = 0x4,
      app      = 0x5
    };
    Q_ENUM(RosHub_Types);

    explicit BtLESerialServer(ConsoleReader* input, QObject *parent = 0);

    int startServer(QMap<QString,QString> idList,  QMap<QString,unsigned char> typeList );
    void stopServer();

  signals:

    void done();

  public slots:
    void dropClient();
    void startAdvertising();

    void handleBLEError(QLowEnergyController::Error error);
    void handleControllerStateChanged(QLowEnergyController::ControllerState state);
    void handleCharactersiticChanged(const QLowEnergyCharacteristic& characteristic, const QByteArray& newValue);

    void handleSendPacket(BtLEPacket packet);
    void handleCommandRxTimeout();
    void handleCommandTxTimeout();
    void handleCommandFailure();
    void handleCommandSuccess();
    void handleCommandRequest(quint8 seq, QJsonDocument jsonDoc);
    void handleCommandJsonError(QJsonParseError error, QByteArray json);
    void handleConsoleMessage(QString text);

  private:
    QTimer advStartTimer;
    ConsoleReader* consoleReader;

    QLowEnergyAdvertisingData advertisingData;
    QLowEnergyCharacteristicData txCharData;
    QLowEnergyCharacteristicData rxCharData;

    QLowEnergyCharacteristicData serialNumberCharData;
    QLowEnergyCharacteristicData softwareVersionCharData;
    QLowEnergyCharacteristicData manufacturerCharData;

    QLowEnergyCharacteristicData roshubOwnerCharData;
    QLowEnergyCharacteristicData roshubIdCharData;

    QLowEnergyServiceData serviceData;
    QLowEnergyServiceData informationServiceData;
    QLowEnergyServiceData roshubInfoServiceData;
    QLowEnergyDescriptorData clientConfig;

    QLowEnergyController* leController;
    QLowEnergyService* service;
    QLowEnergyService* informationService;
    QLowEnergyService* roshubInfoService;


    QMap<quint8, BtLECommand*> commands;
};

#endif // BTLESERIALSERVER_H
