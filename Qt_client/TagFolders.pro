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
        mainwindow.cpp \
    treemodel.cpp \
    treeitem.cpp \
    GetName.cpp \
    filelist.cpp

HEADERS  += mainwindow.h \
    treemodel.h \
    treeitem.h \
    ../TagFolder.h \
    ../String/src/string_project/String.h \
    GetName.h \
    filelist.h

INCLUDEPATH += ../ \
 ../String/src/string_project/

LIBS += -L../ -lTagFolder -lsqlite3 -lssl -lcrypto -L../String/src/string_project-build/ -lStrings

FORMS    += mainwindow.ui

DISTFILES +=
