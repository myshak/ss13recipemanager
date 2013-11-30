#-------------------------------------------------
#
# Project created by QtCreator 2013-11-27T03:50:51
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

QMAKE_CXXFLAGS += -std=c++0x -Wextra

TARGET = ss13recipes
TEMPLATE = app

INCLUDEPATH += include/
INCLUDEPATH += yaml-cpp/include/

SOURCES += main.cpp\
        mainwindow.cpp \
    reagent.cpp \
    recipelist.cpp \
    recipelistproxymodel.cpp

SOURCES += $$files(yaml-cpp/src/*.cpp)


HEADERS  += mainwindow.h \
    reagent.h \
    recipelist.h \
    recipelistproxymodel.h

HEADERS += $$files(yaml-cpp/src/*.h)

FORMS    += mainwindow.ui
