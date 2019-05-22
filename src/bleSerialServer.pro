QT += core
QT += bluetooth
QT += network
QT -= gui

TARGET = bleSerialServer
CONFIG += console
CONFIG += c++11
CONFIG += debug
CONFIG -= app_bundle

#DEFINES += VERSION_NUMBER=\\\"$$system(git describe --always --abbrev=8 --tags --dirty)\\\"
DEFINES += VERSION_NUMBER=\\\"0.3\\\"

CONFIG(debug, debug|release) {
    QMAKE_CXXFLAGS_DEBUG += -g3 -O0
    message("DEBUG!")
} else {
    DEFINES += QT_NO_DEBUG
    DEFINES += QT_NO_DEBUG_OUTPUT
    message("RELEASE!")
}

TEMPLATE = app

SOURCES += main.cpp \
    btserialapp.cpp \
    consolereader.cpp \
    btleserialserver.cpp \
    btlecommand.cpp

HEADERS += \
    btserialapp.h \
    consolereader.h \
    btleserialserver.h \
    btlecommand.h

# install
target.path = /bin
INSTALLS += target
