######################################################################
# Automatically generated by qmake (2.01a) Mon Nov 8 20:30:42 2010
######################################################################

TEMPLATE = app
TARGET = client
DEPENDPATH += .
INCLUDEPATH += . /System/Library/Frameworks/AppKit.framework/Versions/C/Headers /System/Library/Frameworks/Foundation.framework/Versions/C/Headers
LIBS += -framework AppKit -framework Foundation

RESOURCES += client.qrc

QT += network

ICON = client.icns

# Input
HEADERS += alert.h
SOURCES += main.cpp
OBJECTIVE_SOURCES += alert.mm
