#-------------------------------------------------
#
# Project created by QtCreator 2014-03-27T12:57:13
#
#-------------------------------------------------

QT       += core gui
QT       +=network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Chat
TEMPLATE = app



SOURCES += main.cpp\
        widget.cpp \
    tcpserver.cpp \
    tcpclient.cpp

HEADERS  += widget.h \
    tcpserver.h \
    tcpclient.h

FORMS    += widget.ui \
    tcpserver.ui \
    tcpclient.ui

RESOURCES += \
    images.qrc
