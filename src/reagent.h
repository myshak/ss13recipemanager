#ifndef REAGENT_H
#define REAGENT_H

#include <QMetaType>
#include <QHash>
#include <QStringList>
#include <QVariant>

class Reagent
{
public:
    Reagent();

    QString name;
    QString fromFile;

    QString recipelist;
    QStringList ingredients;
    QHash<QString, QVariant> properties;
};

Q_DECLARE_METATYPE(Reagent*)

#endif // REAGENT_H
