#
# Author:  Luca Carlon
# Company: -
# Date:    12.14.2013
#

QT += core xml
QT -= gui

VERSION = 1.0

TARGET = ITunesEmbed
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

INCLUDEPATH += /opt/local/include \
   /opt/local/include/taglib \
   $$_PRO_FILE_PWD_/../LightLogger
LIBS += -L/opt/local/lib -ltag

SOURCES += main.cpp
