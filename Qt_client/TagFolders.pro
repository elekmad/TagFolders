#-------------------------------------------------
#
# Project created by QtCreator 2016-10-07T11:33:55
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = TagFolders
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp

HEADERS  += mainwindow.h

INCLUDEPATH += ../

LIBS += -L../ -lTagFolder -lsqlite3 -lssl -lcrypto

FORMS    += mainwindow.ui

DISTFILES +=
