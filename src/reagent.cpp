#include "reagent.h"

Reagent::Reagent()
{
}

bool Reagent::contains_ingredient(const QString &r) const
{
    for(auto i: ingredients) {
        if(r.compare(i.name, Qt::CaseInsensitive) == 0) {
            return true;
        }
    }

    return false;
}

bool Reagent::matches_tags(const QStringList &tags) const
{
    if(tags.isEmpty()) {
        return true;
    }

    if(properties.contains("tags")) {
        QStringList reagent_tags = properties["tags"].toStringList();
        for(auto i: tags) {
            if(!reagent_tags.contains(i)) {
                return false;
            }
        }

        return true;
    }

    return false;
}
