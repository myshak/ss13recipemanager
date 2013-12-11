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

void RecipeListProxyModel::setFilterString(const QString &pattern)
{
    QStringList fixed_pattern;

    tags.clear();

    for(QString &i: pattern.split(" ")) {
        if(i.startsWith("tag:", Qt::CaseInsensitive)) {
            QString tag = i.mid(4); // From end if 'tag:' onwards
            if(!tag.isEmpty()) {
                tags.append(tag);
            }
        } else {
            fixed_pattern.append(i);
        }
    }

    QSortFilterProxyModel::setFilterFixedString(fixed_pattern.join(" ").trimmed());
}

bool RecipeListProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    bool accept = QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent);
    bool matches = accept;

    if (!recipelist && tags.isEmpty())
        return matches;

    if (filterKeyColumn() == -1) {

        int column_count = sourceModel()->columnCount(sourceParent);
        for (int column = 0; column < column_count; ++column) {
            QModelIndex source_index = sourceModel()->index(sourceRow, column, sourceParent);
            Reagent *r = sourceModel()->data(source_index, reagentRole).value<Reagent*>();

            if(recipelist) {
                if(r->recipelist != recipelist->name || r->fromFile != recipelist->filename) {
                    matches = false;
                }
            }
            if(tags.isEmpty() || !r->matches_tags(tags)) {
                matches = false;
            }
        }
    } else {

        QModelIndex source_index = sourceModel()->index(sourceRow, filterKeyColumn(), sourceParent);
        if (source_index.isValid()) {
            Reagent *r = sourceModel()->data(source_index, reagentRole).value<Reagent*>();
            //assert(r != nullptr);
            if(recipelist) {
                if(r->recipelist != recipelist->name || r->fromFile != recipelist->filename) {
                    matches = false;
                }
            }
            if(!tags.isEmpty() && !r->matches_tags(tags)) {
                matches = false;
            }
        }
    }

    return matches;
}
