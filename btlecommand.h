#ifndef BTLECOMMAND_H
#define BTLECOMMAND_H

#include <QtCore>

struct BtLEPacket {
  public:
    struct {
      quint8 sequence;
      quint8 index;
      quint8 count;
      union {
        struct {
          quint8 data_length: 5;
          quint8 req: 1;
          quint8 nak: 1;
          quint8 ack: 1;

        };
        quint8 flags;
      } FLAGS;
    } header;

    QByteArray data;

    QByteArray serialize() const;

    static BtLEPacket fromRawPacket(QByteArray packet);
    static QMap<quint8,BtLEPacket> fromJson(quint8 sequence, QJsonDocument doc);
    static QByteArray getPayload(QMap<quint8, BtLEPacket> packets);
};


class BtLECommand : public QObject
{
    Q_OBJECT
  public:
    explicit BtLECommand(quint8 sequence, QObject *parent = 0);

    enum CommandState { RxPending, TxPending, Success, Failure};
    Q_ENUM(CommandState);

    quint8 sequence() const;

  signals:
    void requestTimeout();
    void responseTimeout();
    void requestReceived(quint8 commandSeq, QJsonDocument doc);
    void requestJsonError(QJsonParseError error, QByteArray json);
    void sendPacket(BtLEPacket packet);
    void commandFinished();
    void commandFailed();

  public slots:
    void handleRequestPacket(BtLEPacket packet);
    void handleResponseNak(quint8 packetIdx);
    void handleResponseAck(quint8 packetIdx);
    void setResponse(QJsonDocument doc);
    void handleActivityTimeout();

  protected:
    void setState(CommandState state);
    void sendNextResponse();

  private:
    CommandState state;
    QDateTime lastActivity;
    QTimer activityTimer;

    quint8 commandSeq;
    int rxCount;
    bool ackRequired;
    QMap<quint8, BtLEPacket> requestPackets;
    QMap<quint8, BtLEPacket> responsePackets;

    QMap<quint8, bool> rxMap;
    QMap<quint8, bool> txMap;
};

#endif // BTLECOMMAND_H
