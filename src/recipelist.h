#ifndef RECIPELIST_H
#define RECIPELIST_H

#include <QMetaType>
#include <QString>

class RecipeList
{
public:
    RecipeList();
    RecipeList(QString name, QString file);

    QString name;
    QString filename;

    bool operator==(const RecipeList &r) const;
};
Q_DECLARE_METATYPE(RecipeList*)

#endif // RECIPELIST_H
