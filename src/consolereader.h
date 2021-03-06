#ifndef CONSOLEREADER_H
#define CONSOLEREADER_H


#include <QObject>
#include <QSocketNotifier>
#include <QFile>

class ConsoleReader : public QObject
{
    Q_OBJECT
public:
    explicit ConsoleReader(QObject *parent = 0);
signals:
    void textReceived(QString message);
public slots:
    void text();
private:
    QSocketNotifier notifier;
    QFile file;
};

#endif
