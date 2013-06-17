#-------------------------------------------------
#
# Project created by QtCreator 2013-06-17T18:48:32
#
#-------------------------------------------------

QT += core gui
QT += opengl
LIBS += -lGLU
QMAKE_CXXFLAGS += -std=c++11

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = qt
TEMPLATE = app


SOURCES += main.cpp\
        main_window.cpp \
    player.cpp \
    cube.cpp

HEADERS  += main_window.h \
    player.h \
    cube.h
