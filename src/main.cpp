#include "mainwindow.h"
#include <QApplication>
#include <QtCore/QLocale>
#include <QTranslator>
#include <QFile>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QTranslator translator;
    if(!translator.load(QLocale::system(), "ss13recipemanager_", ":/translations/")) {
        // Load default english translation file
        translator.load("ss13recipemanager_en", ":/translations/");
    }
    a.installTranslator(&translator);

    QFile qss("style.qss");
    qss.open(QFile::ReadOnly);
    a.setStyleSheet( QString::fromUtf8(qss.readAll()) );
    qss.close();

    MainWindow w;
    w.show();

    return a.exec();
}
