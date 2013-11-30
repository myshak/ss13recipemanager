#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSettings>
#include <QStandardItemModel>
#include <QList>

#include <unordered_map>

#include "reagent.h"
#include "recipelist.h"
#include "recipelistproxymodel.h"

#define SETTINGS_FILENAME "recipemanager.ini"

namespace Ui {
class MainWindow;
}

namespace std
{
    template<>
    struct hash<QString> : public unary_function<QString, size_t> {
        size_t operator()(QString _Val) const
        {
            return qHash(_Val);
        }
    };
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

protected:
    void closeEvent(QCloseEvent *event);

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    template <typename T>
    static T getSetting(const QString& name, const QSettings &settings = QSettings(SETTINGS_FILENAME, QSettings::IniFormat)) {
        QString key(name);
        if(settings.group() != "") {
            // We currently are in a group - append the group name to the key
            key = QString("%1/%2").arg(settings.group(), name);
        }

        return settings.value(name).value<T>();
    }

    enum ReactionStepTypes {StepReagent, StepInstruction, StepHeat, StepIntermediateResult};
    typedef QPair<QPair<Reagent, MainWindow::ReactionStepTypes>, int> ReactionStep;

    void load_saved_recipelists();
    void load_recipelist(QString filename);
    void save_settings();

private slots:
    void reagentlist_selection_changed(const QItemSelection & selected, const QItemSelection & deselected);
    void ingredientlist_selection_doubleclicked(int x, int y);

    void on_actionAdd_recipe_list_triggered();
    void on_recipelists_selector_currentIndexChanged(int index);

private:
    Ui::MainWindow *ui;
    QStandardItemModel *recipelist_model;
    RecipeListProxyModel *recipelist_proxy_model;

    bool settings_changed = false;

    QList<RecipeList> recipeLists;
    QList<Reagent> reagents;

    QWidget *create_ingredient_tab(const Reagent *reagent);
    QWidget *create_info_tab(const Reagent *reagent);
    QWidget *create_directions_tab(const Reagent *reagent);
    QList<ReactionStep>  gather_reactions(Reagent reagent, int level=0);
};

#endif // MAINWINDOW_H
