include(../compiler.pri)

TEMPLATE = app
CONFIG -= qt
CONFIG += debug_and_release
# console
TARGET = mumble

win32 {
  DEFINES += WIN32 _WIN32
  RC_FILE = mumble.rc

  CONFIG(release, debug|release) {
    QMAKE_CXXFLAGS_RELEASE -= -MD
    QMAKE_CXXFLAGS += -MT
  }
  CONFIG(debug, debug|release) {
    QMAKE_CXXFLAGS_DEBUG -= -MDd
    QMAKE_CXXFLAGS += -MTd
  }
}

SOURCES *= mumble_exe.cc

CONFIG(debug, debug|release) {
  CONFIG += console
  DEFINES *= DEBUG
  DESTDIR	= ../debug
}

CONFIG(release, debug|release) {
  DEFINES *= NDEBUG
  DESTDIR	= ../release
}

include(../symbols.pri)
