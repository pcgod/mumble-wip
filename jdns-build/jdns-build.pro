include(../compiler.pri)

TEMPLATE = lib
TARGET = jdns
CONFIG += staticlib

include(../iris/src/jdns/jdns.pri)

CONFIG(debug, debug|release) {
  CONFIG += console
}

include(../symbols.pri)

