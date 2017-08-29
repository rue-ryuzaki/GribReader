#-------------------------------------------------
#
# Project created by QtCreator 2016-02-17T04:23:09
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = GribReader
TEMPLATE = app

QMAKE_CXXFLAGS += -std=c++11

SOURCES += main.cpp\
        mainwindow.cpp \
    log.cpp \
    gribchecker.cpp

HEADERS  += mainwindow.h \
    log.h \
    gribchecker.h \
    bds.h \
    pds4.h \
    gds.h \
    bms.h

FORMS    += mainwindow.ui
