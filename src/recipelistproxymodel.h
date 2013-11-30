#ifndef RECIPELISTPROXYMODEL_H
#define RECIPELISTPROXYMODEL_H

#include <QSortFilterProxyModel>

#include "recipelist.h"

class RecipeListProxyModel : public QSortFilterProxyModel
{
public:
    RecipeListProxyModel(QObject *parent = 0);

    RecipeList *filterRecipeList() const { return recipelist; }
    void setRecipeList(RecipeList * const list);

    int filterReagentRole() const { return reagentRole; }
    void setFilterReagentRole(const int role);


protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const;

private:
    RecipeList *recipelist;
    int reagentRole = Qt::UserRole + 1;
};

#endif // RECIPELISTPROXYMODEL_H
