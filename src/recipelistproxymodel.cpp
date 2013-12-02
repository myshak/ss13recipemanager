#include "recipelistproxymodel.h"
#include "reagent.h"

RecipeListProxyModel::RecipeListProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
}

void RecipeListProxyModel::setRecipeList(RecipeList * const list)
{
    recipelist = list;
    invalidateFilter();
}

void RecipeListProxyModel::setFilterReagentRole(const int role)
{
    reagentRole = role;
    invalidateFilter();
}

bool RecipeListProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    bool accept = QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent);

    if (!recipelist)
        return accept;

    if (filterKeyColumn() == -1) {

        int column_count = sourceModel()->columnCount(sourceParent);
        for (int column = 0; column < column_count; ++column) {
            QModelIndex source_index = sourceModel()->index(sourceRow, column, sourceParent);
            Reagent *r = sourceModel()->data(source_index, reagentRole).value<Reagent*>();
            if(r->recipelist == recipelist->name && r->fromFile == recipelist->filename)
                return accept;
        }
        return false;
    }
    QModelIndex source_index = sourceModel()->index(sourceRow, filterKeyColumn(), sourceParent);
    if (!source_index.isValid()) // the column may not exist
        return accept;
    Reagent *r = sourceModel()->data(source_index, reagentRole).value<Reagent*>();
    if(r->recipelist == recipelist->name && r->fromFile == recipelist->filename)
        return accept;

    return false;
}
