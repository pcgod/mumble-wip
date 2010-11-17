include(../plugins.pri)

TARGET		= scripted
HEADERS		= scripted.h
SOURCES		= scripted.cpp
CONFIG		+= qt
FORMS		+= scripted.ui
QT			+= script scripttools
