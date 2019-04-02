#include "btserialapp.h"
#include "btleserialserver.h"

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QMap>
#include <QMetaEnum>

#include <signal.h>
#include <unistd.h>

void catchUnixSignals(const std::vector<int>& quitSignals,
                      const std::vector<int>& ignoreSignals = std::vector<int>()) {

  auto handler = [](int sig) ->void {
    printf("\nquit the application (user request signal = %d).\n", sig);
    QCoreApplication::quit();
  };

  // all these signals will be ignored.
  for ( int sig : ignoreSignals ){
    signal(sig, SIG_IGN);
  }
  // each of these signals calls the handler (quits the QCoreApplication).
  for ( int sig : quitSignals ){
    signal(sig, handler);
  }
}


//owner id, device id
int main(int argc, char *argv[])
{
  QCoreApplication a(argc, argv);
  QCoreApplication::setApplicationName("btSerialServer");
  QCoreApplication::setApplicationVersion("0.1");

  QCommandLineParser parser;
  parser.setApplicationDescription("btSerialServer");
  parser.addHelpOption();
  parser.addVersionOption();
  
  QCommandLineOption actorIDOption(QStringList() << "a" << "actor-id",
        QCoreApplication::translate("main", "Actor ID in Hex (XX:XX...)"),
        QCoreApplication::translate("main", "actor-id"));
  parser.addOption(actorIDOption);

  QCommandLineOption ownerIDOption(QStringList() << "o" << "owner-id",
        QCoreApplication::translate("main", "Owner ID in Hex (XX:XX...)"),
        QCoreApplication::translate("main", "owner-id"));
  parser.addOption(ownerIDOption);

  QCommandLineOption actorTypeOption(QStringList() << "t" << "actor-type",
        QCoreApplication::translate("main", "Actor Type (device, app)"),
        QCoreApplication::translate("main", "actor-type"));
  parser.addOption(actorTypeOption);

  QCommandLineOption ownerTypeOption(QStringList() << "y" << "owner-type",
        QCoreApplication::translate("main", "Owner Type (user, team, org)"),
        QCoreApplication::translate("main", "owner-type"));
  parser.addOption(ownerTypeOption);


  parser.process(a);
  QMap<QString,QString> idList;
  QMap<QString,unsigned char> typeList;
  QMetaEnum typeparser = QMetaEnum::fromType<BtLESerialServer::RosHub_Types>();
  
  if (parser.isSet(ownerIDOption)){
    idList["owner-id"] = parser.value(ownerIDOption);
  }
  if (parser.isSet(actorIDOption)){
    idList["actor-id"] = parser.value(actorIDOption);
  }
  if (parser.isSet(actorTypeOption)){
    char actortype = char(parser.value(actorTypeOption).toUInt());
    if(!actortype) actortype = typeparser.keyToValue(parser.value(actorTypeOption).toLower().toUtf8());
    if(actortype){
      typeList["actor-type"] = actortype;
    }
  }
  if (parser.isSet(ownerTypeOption)){
    char ownertype = char(parser.value(ownerTypeOption).toUInt());
    if(!ownertype) ownertype = typeparser.keyToValue(parser.value(ownerTypeOption).toLower().toUtf8());
    if(ownertype){
      typeList["owner-type"] = ownertype;
    }
  }
  
  catchUnixSignals({SIGQUIT, SIGINT, SIGTERM, SIGHUP});
  BtSerialApp task;
  QObject::connect(&task, SIGNAL(done()), 
           &a, SLOT(quit()));
  QObject::connect(&a, SIGNAL(aboutToQuit()), 
           &task, SLOT(aboutToQuitApp()));
  task.run(idList, typeList);
  return a.exec();
}

