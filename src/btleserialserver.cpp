#include "btleserialserver.h"
#include <stdio.h>
#include <QHostInfo>

BtLESerialServer::BtLESerialServer(ConsoleReader* input, QObject *parent) : QObject(parent)
{
  this->consoleReader = input;
  connect(&this->advStartTimer, SIGNAL(timeout()), this, SLOT(startAdvertising()));
  qDebug("BtLESerialServer - constructor done");
}

//owner_id, device_id
int BtLESerialServer::startServer(QStringList args){
  QByteArray owner_id;
  QByteArray device_id;
  if (args.length() < 3){
    owner_id = ROSHUB_OWNER_ID;
    device_id = ROSHUB_DEVICE_ID;
  } else {
    owner_id = QByteArray::fromHex(args.at(1).toUtf8());
    device_id = QByteArray::fromHex(args.at(2).toUtf8());
  }
  qDebug("BtLESerialServer - start server");
  QString localName = QString("RosHub-").append(QHostInfo::localHostName());
  localName.truncate(16);
  this->advertisingData.setDiscoverability(QLowEnergyAdvertisingData::DiscoverabilityGeneral);
  this->advertisingData.setIncludePowerLevel(true);
  this->advertisingData.setLocalName(localName);
  this->advertisingData.setServices(QList<QBluetoothUuid>() /*<< QBluetoothUuid::SerialPort << QBluetoothUuid::DeviceInformation*/ << QBluetoothUuid(ROSHUB_INFO_SERVICE_UUID));
  //! Used serial profile due to adv packet size limits

  this->txCharData.setUuid(QBluetoothUuid(ROSHUB_TX_CHAR_UUID));
  this->txCharData.setValue(QByteArray(20, 0));
  this->txCharData.setProperties(QLowEnergyCharacteristic::Notify | QLowEnergyCharacteristic::Read);
  this->clientConfig = QLowEnergyDescriptorData(QBluetoothUuid::ClientCharacteristicConfiguration, QByteArray(2, 0));
  this->txCharData.addDescriptor(this->clientConfig);

  this->rxCharData.setUuid(QBluetoothUuid(ROSHUB_RX_CHAR_UUID));
  this->rxCharData.setValue(QByteArray(20, 0));
  this->rxCharData.setProperties(QLowEnergyCharacteristic::Indicate | QLowEnergyCharacteristic::Write);
  this->clientConfig = QLowEnergyDescriptorData(QBluetoothUuid::ClientCharacteristicConfiguration, QByteArray(2, 0));
  this->rxCharData.addDescriptor(this->clientConfig);


  this->serviceData.setType(QLowEnergyServiceData::ServiceTypePrimary);
  //this->serviceData.setUuid(QBluetoothUuid::SerialPort);
  this->serviceData.setUuid(QBluetoothUuid(ROSHUB_INFO_SERVICE_UUID));
  this->serviceData.addCharacteristic(this->txCharData);
  this->serviceData.addCharacteristic(this->rxCharData);



  this->serialNumberCharData.setUuid(QBluetoothUuid(QBluetoothUuid::SerialNumberString));
  this->serialNumberCharData.setValue(device_id.toHex());
  this->serialNumberCharData.setProperties(QLowEnergyCharacteristic::Read);
  /*this->clientConfig = QLowEnergyDescriptorData(QBluetoothUuid::ClientCharacteristicConfiguration, QByteArray(2, 0));
  this->serialNumberCharData.addDescriptor(this->clientConfig);*/

  this->softwareVersionCharData.setUuid(QBluetoothUuid(QBluetoothUuid::SoftwareRevisionString));
  this->softwareVersionCharData.setValue(QByteArray::fromStdString("v1.0"));
  this->softwareVersionCharData.setProperties(QLowEnergyCharacteristic::Read);
  /*this->clientConfig = QLowEnergyDescriptorData(QBluetoothUuid::ClientCharacteristicConfiguration, QByteArray(2, 0));
  this->softwareVersionCharData.addDescriptor(this->clientConfig);*/


  this->manufacturerCharData.setUuid(QBluetoothUuid(QBluetoothUuid::ManufacturerNameString));
  this->manufacturerCharData.setValue(QByteArray::fromStdString("RosHub Inc"));
  this->manufacturerCharData.setProperties(QLowEnergyCharacteristic::Read);
  /*this->clientConfig = QLowEnergyDescriptorData(QBluetoothUuid::ClientCharacteristicConfiguration, QByteArray(2, 0));
  this->manufacturerCharData.addDescriptor(this->clientConfig);*/


  this->informationServiceData.setType(QLowEnergyServiceData::ServiceTypePrimary);
  this->informationServiceData.setUuid(QBluetoothUuid::DeviceInformation);
  this->informationServiceData.addCharacteristic(this->serialNumberCharData);
  this->informationServiceData.addCharacteristic(this->softwareVersionCharData);
  this->informationServiceData.addCharacteristic(this->manufacturerCharData);


  this->roshubOwnerCharData.setUuid(QBluetoothUuid(ROSHUB_OWNER_CHAR_UUID));
  this->roshubOwnerCharData.setValue(owner_id.prepend(0xff & User_RosHub).toHex());
  //this->roshubOwnerCharData.setValue("");
  this->roshubOwnerCharData.setProperties(QLowEnergyCharacteristic::Read);

  this->roshubIdCharData.setUuid(QBluetoothUuid(ROSHUB_ID_CHAR_UUID));
  this->roshubIdCharData.setValue(device_id.prepend(0xff & Device_RosHub).toHex());
  //this->roshubIdCharData.setValue("");
  this->roshubIdCharData.setProperties(QLowEnergyCharacteristic::Read);

  //this->roshubInfoServiceData.setType(QLowEnergyServiceData::ServiceTypePrimary);
  //this->roshubInfoServiceData.setUuid(QBluetoothUuid(ROSHUB_INFO_SERVICE_UUID));
  this->serviceData.addCharacteristic(this->roshubOwnerCharData);
  this->serviceData.addCharacteristic(this->roshubIdCharData);

  this->leController = QLowEnergyController::createPeripheral();
  this->service = leController->addService(this->serviceData);
  this->informationService = leController->addService(this->informationServiceData);
  //this->roshubInfoService = leController->addService(this->roshubInfoServiceData);

  connect(this->leController, SIGNAL(error(QLowEnergyController::Error)), this, SLOT(handleBLEError(QLowEnergyController::Error)));
  connect(this->leController, SIGNAL(stateChanged(QLowEnergyController::ControllerState)), this, SLOT(handleControllerStateChanged(QLowEnergyController::ControllerState)));
  connect(this->service, SIGNAL(characteristicChanged(QLowEnergyCharacteristic,QByteArray)), this, SLOT(handleCharactersiticChanged(QLowEnergyCharacteristic,QByteArray)));
  connect(this->consoleReader, SIGNAL(textReceived(QString)), this, SLOT(handleConsoleMessage(QString)));

  //leController->startAdvertising(QLowEnergyAdvertisingParameters(), this->advertisingData, this->advertisingData);

  qDebug("BtLESerialServer - ready");

  if(this->advStartTimer.isActive()){
    qDebug() << "BtLESerialServer - advertise timer already running";
    return 0;
  }
  this->advStartTimer.setSingleShot(true);
  this->advStartTimer.start(5000);

  return 0;
}

void BtLESerialServer::dropClient(){

  if(this->leController->state() == QLowEnergyController::ConnectedState){
    qDebug() << "disconnectFromDevice";
    this->leController->disconnectFromDevice();

    //Probably need to do somethings here to deal with restarting the ble controller
  }
}

void BtLESerialServer::handleBLEError(QLowEnergyController::Error error){
  QTextStream out(stdout);

  QJsonObject object;
  QJsonObject errObj;
  QJsonObject clientInfo;

  errObj.insert("code", (int)error);
  errObj.insert("msg", this->leController->errorString());

  object.insert("code", "error");
  object.insert("source", "ble");
  object.insert("error", errObj);
  clientInfo.insert("address", this->leController->remoteAddress().toString());
  clientInfo.insert("type", (int)this->leController->remoteAddressType());
  if(!this->leController->remoteName().isEmpty()){
    clientInfo.insert("name", this->leController->remoteName());
  }
  object.insert("client", clientInfo);

  QJsonDocument errDoc(object);
  out << errDoc.toJson(QJsonDocument::Compact);
}

void BtLESerialServer::handleConsoleMessage(QString message){
  qDebug("recieved a message from STDIN!");
  QByteArray text = message.toUtf8();
  QJsonObject object;
  QJsonDocument document = QJsonDocument::fromJson(text);

  if(!document.isObject()){ qDebug() << "received a non-JSON message, ignoring"; return;}

  object = document.object();

  if(object["code"] == "tx"){
    qDebug() << "handling tx";
    QJsonDocument dataDoc;
    int seq = -1;
    if(object.value("data").isObject()){
      if(object.value("data").toObject().contains("seq")){
        seq = object.value("data").toObject().value("seq").toInt(-1);
      }
      dataDoc = QJsonDocument(object.value("data").toObject());
    }
    else{
      QJsonObject obj;
      obj.insert("data", object.value("data"));
      dataDoc = QJsonDocument(obj);
    }

    if(seq < 0){
      if(this->commands.count() == 1){
        this->commands.first()->setResponse(dataDoc);
      }
      else{ qDebug() << "No command response sequence provided"; }
    }
    else{
      if(this->commands.contains((quint8)seq)){
        this->commands[(quint8)seq]->setResponse(dataDoc);
      }
    }

  }
  else if(object["code"] == "drop"){
    qDebug() << "handling drop";
    this->dropClient();
    this->commands.clear();
  }
}


void BtLESerialServer::handleControllerStateChanged(QLowEnergyController::ControllerState state){
  qDebug() << "Controller state changed: " << state;

  if(this->leController==NULL || this->service==NULL){
    qDebug() << "BLE deleted from under us!";
    return;
  }

  if(state == QLowEnergyController::ConnectedState){
    qDebug() << "New client: " << this->leController->remoteAddress().toString();
    QTextStream out(stdout);
    QJsonObject status;
    QJsonObject clientInfo;
    QJsonDocument doc;

    status.insert("code", "status");
    status.insert("source", "ble");
    clientInfo.insert("address", this->leController->remoteAddress().toString());
    clientInfo.insert("type", (int)this->leController->remoteAddressType());
    if(!this->leController->remoteName().isEmpty()){
      clientInfo.insert("name", this->leController->remoteName());
    }
    status.insert("client", clientInfo);
    status.insert("state", "connected");

    doc = QJsonDocument(status);
    out << doc.toJson(QJsonDocument::Compact);

    this->leController->stopAdvertising();
  }
  else if(state == QLowEnergyController::UnconnectedState ||
          (state == QLowEnergyController::AdvertisingState && this->leController->remoteAddress().isNull())){
    QTextStream out(stdout);
    QJsonObject status;
    QJsonDocument doc;

    status.insert("code", "status");
    status.insert("source", "ble");
    if(state == QLowEnergyController::AdvertisingState){ status.insert("state", "listen"); }
    else{ status.insert("state", "reset"); }

    doc = QJsonDocument(status);
    out << doc.toJson(QJsonDocument::Compact);

    if(state != QLowEnergyController::AdvertisingState){
      if(this->leController->error() == QLowEnergyController::AdvertisingError){
        qDebug() << "BtLESerialServer - error state :(";
        /*delete this->service;
        delete this->leController;

        this->service = NULL;
        this->leController = NULL;
        this->startServer();*/
        //return;
      }

      if(this->advStartTimer.isActive()){
        qDebug() << "BtLESerialServer - advertise timer already running";
        return;
      }
      qDebug() << "BtLESerialServer - advertise timer";
      this->advStartTimer.setSingleShot(true);
      this->advStartTimer.start(5000);
      //this->leController->startAdvertising(QLowEnergyAdvertisingParameters(), this->advertisingData, this->advertisingData);
    }
  }
}

void BtLESerialServer::startAdvertising(){
  qDebug() << "BtLESerialServer - startAdvertising()";
  if(this->leController){
    this->leController->startAdvertising(QLowEnergyAdvertisingParameters(), this->advertisingData, this->advertisingData);
  }
}

void BtLESerialServer::handleCharactersiticChanged(const QLowEnergyCharacteristic& characteristic, const QByteArray& newValue){
  qDebug() << "Characteristic[" << characteristic.uuid().toString() << "] changed";

  if(characteristic.uuid().toString() == ROSHUB_RX_CHAR_UUID.toString()){
    qDebug("BtLESerialServer - Incoming command");

    BtLEPacket packet = BtLEPacket::fromRawPacket(newValue);

    if(!this->commands.contains(packet.header.sequence)){
      BtLECommand* command = new BtLECommand(packet.header.sequence, this);
      this->commands.insert(packet.header.sequence, command);

      connect(command, SIGNAL(commandFailed()), this, SLOT(handleCommandFailure()));
      connect(command, SIGNAL(commandFinished()), this, SLOT(handleCommandSuccess()));
      connect(command, SIGNAL(requestJsonError(QJsonParseError,QByteArray)), this, SLOT(handleCommandJsonError(QJsonParseError,QByteArray)));
      connect(command, SIGNAL(requestReceived(quint8,QJsonDocument)), this, SLOT(handleCommandRequest(quint8,QJsonDocument)));
      connect(command, SIGNAL(requestTimeout()), this, SLOT(handleCommandRxTimeout()));
      connect(command, SIGNAL(responseTimeout()), this, SLOT(handleCommandTxTimeout()));
      connect(command, SIGNAL(sendPacket(BtLEPacket)), this, SLOT(handleSendPacket(BtLEPacket)));
    }

    this->commands[packet.header.sequence]->handleRequestPacket(packet);
  }
  else{
    qDebug("Not RX characteristic change");
  }
}

void BtLESerialServer::stopServer(){
  if(this->leController){
    if(!this->leController->remoteAddress().isNull()){
      this->dropClient();
    }
    this->leController->stopAdvertising();
    delete this->service;
    delete this->leController;

    this->service = NULL;
    this->leController = NULL;
  }

  this->commands.clear();
}


void BtLESerialServer::handleSendPacket(BtLEPacket packet){
  qDebug() << "BtLESerialServer - Sending packet[" << packet.header.sequence << "," << packet.header.count << ", " << packet.header.FLAGS.flags << "]";
  QByteArray value = packet.serialize();
  Q_ASSERT(value.length() == 20);

  QLowEnergyCharacteristic characteristic = this->service->characteristic(QBluetoothUuid(ROSHUB_TX_CHAR_UUID));

  Q_ASSERT(characteristic.isValid());
  service->writeCharacteristic(characteristic, value); // Potentially causes notification.
}

void BtLESerialServer::handleCommandRxTimeout(){
  BtLECommand* command = qobject_cast<BtLECommand*>(QObject::sender());
  Q_ASSERT(command != NULL);
  qDebug()<<"BtLESerialServer - Command[" << command->sequence() << "] RX timeout";
}

void BtLESerialServer::handleCommandTxTimeout() {
  BtLECommand* command = qobject_cast<BtLECommand*>(QObject::sender());
  Q_ASSERT(command != NULL);
  qDebug()<<"BtLESerialServer - Command[" << command->sequence() << "] TX timeout";
}

void BtLESerialServer::handleCommandFailure() {
  BtLECommand* command = qobject_cast<BtLECommand*>(QObject::sender());
  Q_ASSERT(command != NULL);

  qDebug() << "BtLESerialServer - Command [" << command->sequence() << "] failed";
  this->commands.remove(command->sequence());
  delete command;
}

void BtLESerialServer::handleCommandSuccess() {
  BtLECommand* command = qobject_cast<BtLECommand*>(QObject::sender());
  Q_ASSERT(command != NULL);

  qDebug() << "BtLESerialServer - Command [" << command->sequence() << "] finished";

  this->commands.remove(command->sequence());
  delete command;
}

void BtLESerialServer::handleCommandRequest(quint8 seq, QJsonDocument jsonDoc) {
  BtLECommand* command = qobject_cast<BtLECommand*>(QObject::sender());
  Q_ASSERT(command != NULL);

  qDebug() << "BtLESerialServer - Got Request[" << command->sequence() << "]";
  qDebug() << "Content: " << jsonDoc.toJson(QJsonDocument::Compact);
  qDebug() << "Latin1: " << QString::fromLatin1(jsonDoc.toJson(QJsonDocument::Compact));
  qDebug() << "Utf8: " << QString::fromUtf8(jsonDoc.toJson(QJsonDocument::Compact));
  qDebug() << "Data: " << jsonDoc.toJson(QJsonDocument::Compact).data();


  QJsonObject object;
  object.insert("code", "rx");
  object.insert("source", "ble");
  object.insert("seq", QJsonValue(command->sequence()));
  object.insert("address", this->leController->remoteAddress().toString());
  object.insert("data", QJsonValue(jsonDoc.toJson(QJsonDocument::Compact).data()));
  QJsonDocument doc(object);

  QTextStream out(stdout);
  out << doc.toJson(QJsonDocument::Compact);
}

void BtLESerialServer::handleCommandJsonError(QJsonParseError error, QByteArray json) {
  BtLECommand* command = qobject_cast<BtLECommand*>(QObject::sender());
  Q_ASSERT(command != NULL);

  qDebug() << "BtLESerialServer - JSON Error in command [" << command->sequence() << "]";
  qDebug() << "Offset: " << error.offset;
  qDebug() << "Error: " << error.errorString();
  qDebug() << "Content: " << QString(json);
}
