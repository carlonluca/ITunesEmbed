#
# Author:  Luca Carlon
# Company: -
# Date:    12.14.2013
#

QT       += core xml
QT       -= gui

TARGET = ITunesEmbed
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

INCLUDEPATH += /opt/local/include \
    /opt/local/include/taglib
LIBS += -L/opt/local/lib -ltag

SOURCES += main.cpp
