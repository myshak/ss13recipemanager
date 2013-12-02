#include "recipelist.h"

RecipeList::RecipeList()
{
}

RecipeList::RecipeList(QString name, QString file)
    : name(name), filename(file)
{
}

bool RecipeList::operator==(const RecipeList &r) const
{
    return (this->name == r.name) && (this->filename == r.filename);
}
