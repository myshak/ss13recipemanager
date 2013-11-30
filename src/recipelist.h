#ifndef RECIPELIST_H
#define RECIPELIST_H

#include <QMetaType>
#include <QString>

class RecipeList
{
public:
    RecipeList();

    QString filename;
    QString name;
};
Q_DECLARE_METATYPE(RecipeList*)

#endif // RECIPELIST_H
