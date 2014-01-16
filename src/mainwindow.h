#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSettings>
#include <QStandardItemModel>
#include <QStringList>
#include <QList>
#include <QTableWidget>

#include "managedialog.h"
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

    template <typename T>
    static void setSetting(const QString& name, const T &value) {
        QSettings settings{SETTINGS_FILENAME, QSettings::IniFormat};
        QString key(name);
        if(settings.group() != "") {
            // We currently are in a group - append the group name to the key
            key = QString("%1/%2").arg(settings.group(), name);
        }
        settings.setValue(name, value);
        settings.sync();
    }

    enum ReactionStepTypes {StepReagent, StepInstruction, StepHeat, StepIntermediateResult};

    typedef struct ReactionStep
    {
        QString reagent_name;
        QVariant entry;
        ReactionStepTypes type;
        int total_depth;
    } ReactionStep;

    void load_saved_recipelists();
    void load_recipelist(QString filename);
    void save_settings();
    void reload_recipe_list();
    void reload_recipelist_selector();
    void reload_tag_cloud();

public slots:
    void reload_recipelists(QStringList rl);


private slots:
    void reagentlist_selection_changed(const QItemSelection & selected/*, const QItemSelection & deselected*/);
    void ingredientlist_selection_doubleclicked(int x, int y);
    void reagentlist_select(const QString &reagent, const QModelIndex &model_index);

    void on_actionAdd_recipe_list_triggered();
    void on_recipelists_selector_currentIndexChanged(int index);
    void on_action_Manage_recipe_lists_triggered();
    void on_action_Save_settings_triggered();
    void on_actionAbout_Recipe_Manager_triggered();
    void on_tag_browser_anchorClicked(const QUrl &arg1);

private:
    friend class ManageDialog;
    Ui::MainWindow *ui;
    QStandardItemModel *recipelist_model;
    RecipeListProxyModel *recipelist_proxy_model;

    bool settings_changed = false;

    QList<RecipeList> recipeLists;
    QList<Reagent> reagents;

    QWidget *create_ingredient_tab(const Reagent *reagent);
    QWidget *create_info_tab(const Reagent *reagent);
    QWidget *create_directions_tab(const Reagent *reagent);
    QWidget *create_usedin_tab(const Reagent *reagent);
    QWidget *create_sources_tab(const Reagent *reagent);

    ReactionStep gather_reactions(Reagent reagent);
    void fill_directions(QTableWidget *table, ReactionStep step, int current_depth = 0);
};

Q_DECLARE_METATYPE(QList<MainWindow::ReactionStep>)
#endif // MAINWINDOW_H
