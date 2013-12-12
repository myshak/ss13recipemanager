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

    enum StepTypes {Ingredient, Instruction};

    struct ReagentStep
    {
        QString name;
        StepTypes type;
        QStringList tags;
    };

    bool contains_ingredient(const QString &r) const;
    bool matches_tags(const QStringList &tags) const;

    QString name;
    QString fromFile;

    QString recipelist;
    QList<ReagentStep> ingredients;
    QHash<QString, QVariant> properties;
};

Q_DECLARE_METATYPE(Reagent*)
Q_DECLARE_METATYPE(Reagent::ReagentStep)

#endif // REAGENT_H
