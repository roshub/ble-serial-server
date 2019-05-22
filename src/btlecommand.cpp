#include "btlecommand.h"

#define BTLE_MTU (20)
#define MAX_INACTIVITY_SEC (25.0f)
#define MAX_INACTIVITY_MS (MAX_INACTIVITY_SEC*1000)

quint8 BtLEPacketSequence = 0;

QByteArray BtLEPacket::serialize() const {
  QByteArray output(20, 0x0);

  output[0] = (this->header.sequence);
  output[1] = (this->header.index);
  output[2] = (this->header.count);
  output[3] = (this->header.FLAGS.flags);
  output.replace(4, this->data.length(), this->data);

  Q_ASSERT(output.length() == 20);

  return output;
}


BtLEPacket BtLEPacket::fromRawPacket(QByteArray data) {
  BtLEPacket pkt;

  pkt.header.sequence = data[0];
  pkt.header.index = data[1];
  pkt.header.count = data[2];
  pkt.header.FLAGS.flags = data[3];

  pkt.data.fill(0x0, 16);

  for(int i=4; i<20; i++){
    pkt.data[i-4] = data[i];
  }

  return pkt;
}

QMap<quint8,BtLEPacket> BtLEPacket::fromJson(quint8 sequence, QJsonDocument doc) {
  QMap<quint8, BtLEPacket> packets;
  QByteArray data = doc.toJson(QJsonDocument::Compact);
  int packetCount = qCeil( ((qreal)data.length()) / (qreal)16.0f );

  Q_ASSERT(packetCount < 256);

  for(quint8 idx=0; idx<packetCount; idx++){
    BtLEPacket pkt;

    int startOffset = idx * (BTLE_MTU - 4);
    int endOffset = qMin((startOffset+(BTLE_MTU-4)-1), data.length()-1);
    int dataLen = (endOffset - startOffset) + 1;

    pkt.header.sequence = sequence;
    pkt.header.index = idx;
    pkt.header.count = packetCount;
    pkt.header.FLAGS.flags = dataLen & 0x1f;

    Q_ASSERT(dataLen <= (BTLE_MTU-4));

    pkt.data.fill(0x0, 16);

    for(quint8 i=0; i<dataLen; i++){
      pkt.data[i] = (data[i + idx*16]);
    }

    packets.insert(idx, pkt);
  }

  Q_ASSERT(packets.count() == packetCount);

  return packets;
}

QByteArray BtLEPacket::getPayload(QMap<quint8, BtLEPacket> packets){
  QByteArray data;
  int pktCount = -1;

  QMapIterator<quint8,BtLEPacket> iter(packets);

  while(iter.hasNext()){
    iter.next();
    if(pktCount < 1){
      pktCount = iter.value().header.count;
      Q_ASSERT(packets.count() == pktCount);
      Q_ASSERT(pktCount > 0);
    }

    int len = iter.value().header.FLAGS.data_length;

    qDebug() << "Packet " << iter.value().header.index;
    qDebug() << "\t Data Len: " << (int)iter.value().header.FLAGS.data_length;
    qDebug() << "\t Flags: " << (int)iter.value().header.FLAGS.flags;

    for(int i=0; i<len; i++){
      data.push_back(iter.value().data[i]);
    }
  }

  //data.push_back((char) 0x0);
  qDebug() << "\t Buffer: " << data;
  qDebug() << "\t Data: " << QString(data);
  qDebug() << "\t TotalLen: " << data.length();
  return data;
}



BtLECommand::BtLECommand(quint8 sequence, QObject *parent) : QObject(parent)
{
  this->commandSeq = sequence;
  this->rxCount = -1;
  this->ackRequired = false;
  this->state = BtLECommand::RxPending;
  this->activityTimer.start((int) ((MAX_INACTIVITY_SEC/2.0f) * 1000));

  connect(&this->activityTimer, SIGNAL(timeout()), this, SLOT(handleActivityTimeout()));
}

void BtLECommand::setState(CommandState state){
  this->state = state;
  if(state == BtLECommand::Success || state == BtLECommand::Failure){
    this->activityTimer.stop();

    if(state == BtLECommand::Success){
      emit commandFinished();
    }
    else{
      emit commandFailed();
    }
  }
}

quint8 BtLECommand::sequence() const {
  return this->commandSeq;
}

void BtLECommand::handleRequestPacket(BtLEPacket packet) {
  if(this->rxCount < 0){
    this->rxCount = packet.header.count;
  }

  if(!this->requestPackets.contains(packet.header.index)){
    // Has not been read
    Q_ASSERT(!this->requestPackets.contains(packet.header.index));

    this->lastActivity = QDateTime::currentDateTimeUtc();

    this->requestPackets.insert(packet.header.index, packet);
  }

  if(this->ackRequired){
    BtLEPacket ackPacket;

    ackPacket.header.sequence = this->commandSeq;
    ackPacket.header.index = packet.header.index;
    ackPacket.header.count = packet.header.count;
    ackPacket.header.FLAGS.ack = 0x1;
    ackPacket.header.FLAGS.data_length = 0x0;

    emit sendPacket(ackPacket);

    this->rxMap[packet.header.index] = true;
  }

  if(this->rxCount == this->requestPackets.count()){
    // RX Done
    QJsonParseError error;
    QByteArray jsonData = BtLEPacket::getPayload(this->requestPackets);
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &error);

    if(error.error != QJsonParseError::NoError){
      emit requestJsonError(error, jsonData);
      this->setState(BtLECommand::Failure);
      return;
    }

    this->setState(BtLECommand::TxPending);
    emit requestReceived(this->commandSeq, doc);
  }
}

void BtLECommand::handleActivityTimeout() {
  QDateTime now = QDateTime::currentDateTimeUtc();
  qint64 idleSec = this->lastActivity.msecsTo(now);

  if(idleSec > MAX_INACTIVITY_MS){

    if(this->state == BtLECommand::RxPending){
      emit requestTimeout();
    }
    else if(this->state == BtLECommand::TxPending){
      emit responseTimeout();
    }
    else{
      Q_ASSERT(false);
    }

    this->setState(BtLECommand::Failure);
  }
}

void BtLECommand::handleResponseNak(quint8 packetIdx) {
  this->txMap[packetIdx] = false;
  this->sendNextResponse();
}

void BtLECommand::handleResponseAck(quint8 packetIdx) {
  this->txMap[packetIdx] = true;
  this->sendNextResponse();
}

void BtLECommand::setResponse(QJsonDocument doc) {
  this->responsePackets = BtLEPacket::fromJson(this->commandSeq, doc);


  if(this->ackRequired){
    this->sendNextResponse();
  }
  else{
    this->sendNextResponse();

    this->setState(BtLECommand::Success);
  }
}

void BtLECommand::sendNextResponse() {
  QMapIterator<quint8,BtLEPacket> pktIter(this->responsePackets);


  while(pktIter.hasNext()){
    pktIter.next();

    if(!this->txMap.contains(pktIter.key()) || !this->txMap[pktIter.key()]){
      emit sendPacket(pktIter.value());

      if(this->ackRequired){
        break;
      }
      else{
        this->txMap[pktIter.key()] = true;
      }
    }
  }

  int txAckCount = 0;
  QMapIterator<quint8,bool> ackIter(this->txMap);
  while(ackIter.hasNext()){
    ackIter.next();
    if(ackIter.value()){
      txAckCount++;
    }
  }

  if(txAckCount == this->responsePackets.count()){
    this->setState(BtLECommand::Success);
  }
}
