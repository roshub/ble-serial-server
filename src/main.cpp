#include "btserialapp.h"

#include <QCoreApplication>

#include <signal.h>
#include <unistd.h>

void catchUnixSignals(const std::vector<int>& quitSignals,
                      const std::vector<int>& ignoreSignals = std::vector<int>()) {

    auto handler = [](int sig) ->void {
        printf("\nquit the application (user request signal = %d).\n", sig);
        QCoreApplication::quit();
    };

    // all these signals will be ignored.
    for ( int sig : ignoreSignals )
        signal(sig, SIG_IGN);

    // each of these signals calls the handler (quits the QCoreApplication).
    for ( int sig : quitSignals )
        signal(sig, handler);
}
//owner id, device id
int main(int argc, char *argv[])
{
  qDebug("hello!");
  QCoreApplication a(argc, argv);
  catchUnixSignals({SIGQUIT, SIGINT, SIGTERM, SIGHUP});
  btSerialApp task;
  QObject::connect(&task, SIGNAL(done()), 
           &a, SLOT(quit()));
  QObject::connect(&a, SIGNAL(aboutToQuit()), 
           &task, SLOT(aboutToQuitApp()));
  task.run();
  return a.exec();
}

