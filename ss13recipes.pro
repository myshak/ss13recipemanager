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
OBJECTS_DIR = bin
MOC_DIR = src/moc
UI_DIR = src/ui
RCC_DIR = src/rcc

INCLUDEPATH += src/include/
INCLUDEPATH += src/yaml-cpp/include/

SOURCES += src/main.cpp\
        src/mainwindow.cpp \
    src/reagent.cpp \
    src/recipelist.cpp \
    src/recipelistproxymodel.cpp \
    src/managedialog.cpp \
    src/textdelegate.cpp \
    src/optionsdialog.cpp

SOURCES += $$files(src/yaml-cpp/src/*.cpp)


HEADERS  += src/mainwindow.h \
    src/reagent.h \
    src/recipelist.h \
    src/recipelistproxymodel.h \
    src/managedialog.h \
    src/textdelegate.h \
    src/optionsdialog.h

HEADERS += $$files(src/yaml-cpp/src/*.h)

FORMS    += src/mainwindow.ui \
    src/managedialog.ui \
    src/optionsdialog.ui

TRANSLATIONS = \
    translations/ss13recipemanager_en.ts

RESOURCES += \
    res.qrc
