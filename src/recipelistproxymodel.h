#ifndef RECIPELISTPROXYMODEL_H
#define RECIPELISTPROXYMODEL_H

#include <QSortFilterProxyModel>
#include <QStringList>

#include "recipelist.h"

class Reagent;

class RecipeListProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    RecipeListProxyModel(QObject *parent = 0);

    RecipeList *filterRecipeList() const { return recipelist; }
    void setRecipeList(RecipeList * const list);

    int filterReagentRole() const { return reagentRole; }
    void setFilterReagentRole(const int role);

public slots:
    void setFilterString(const QString &pattern);

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const;

private:
    RecipeList *recipelist;
    int reagentRole = Qt::UserRole + 1;
    QStringList tags;

    bool matchesTags(const Reagent *r) const;
};

#endif // RECIPELISTPROXYMODEL_H
