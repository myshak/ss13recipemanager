#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "yaml-cpp/yaml.h"

#include <algorithm>

#include <QDebug>
#include <QLabel>
#include <QCloseEvent>
#include <QTableWidget>
#include <QMessageBox>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QTranslator>
#include <QFileDialog>
#include <QScrollArea>
#include <QPair>
#include <QMap>

#if 0
// Strings for lupdate
QStringList lupdate_unused = {
QT_TRANSLATE_NOOP("InfoStrings","depletion_rate"),
QT_TRANSLATE_NOOP("InfoStrings","penetrates_skin"),
QT_TRANSLATE_NOOP("InfoStrings","ignition_temp"),
QT_TRANSLATE_NOOP("InfoStrings","application_effect"),
QT_TRANSLATE_NOOP("InfoStrings","overdose_threshold"),
QT_TRANSLATE_NOOP("InfoStrings","notes"),
QT_TRANSLATE_NOOP("InfoStrings","per_cycle")
};
#endif

static QMap<MainWindow::ReactionStepTypes, QColor> init_StepColors() {
    QMap<MainWindow::ReactionStepTypes, QColor> map;

    map.insert(MainWindow::ReactionStepTypes::StepReagent,               QColor(255,255,255));
    map.insert(MainWindow::ReactionStepTypes::StepIntermediateResult,    QColor(230,230,255));
    map.insert(MainWindow::ReactionStepTypes::StepInstruction,           QColor(230,255,230));
    map.insert(MainWindow::ReactionStepTypes::StepHeat,                  QColor(255,230,230));

    return map;
}

static const QMap<MainWindow::ReactionStepTypes, QColor> StepColors = init_StepColors();

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    recipelist_model = new QStandardItemModel(this);
    recipelist_proxy_model = new RecipeListProxyModel(this);
    recipelist_proxy_model->setSourceModel(recipelist_model);
    recipelist_proxy_model->setFilterCaseSensitivity(Qt::CaseInsensitive);

    ui->reagent_table->setModel(recipelist_proxy_model);
    ui->reagent_table->verticalHeader()->setResizeMode(QHeaderView::ResizeToContents);

    ui->splitter->setSizes({300,700});
    ui->recipelists_selector->addItem(tr("All recipe lists"),QVariant::fromValue<RecipeList*>(nullptr));

    connect(ui->reagent_table->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(reagentlist_selection_changed(QItemSelection/*,QItemSelection*/)));
    connect(ui->search_filter, SIGNAL(textChanged(QString)), recipelist_proxy_model, SLOT(setFilterString(QString)));

    ui->menuWindow->addAction(ui->dockWidget->toggleViewAction());

    restoreGeometry(getSetting<QByteArray>("general/geometry"));
    restoreState(getSetting<QByteArray>("general/windowState"));

    load_saved_recipelists();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::load_recipelist(QString filename)
{
    RecipeList rl;
    rl.filename = filename;

    YAML::Node list;
    try {
        list = YAML::LoadFile(filename.toStdString());
    } catch(YAML::Exception e) {
        QMessageBox::critical(this, tr("Error loading recipe list"), tr("There was an error during loading of:\n%0\n\n%1").arg(filename,QString::fromStdString(e.what())));
        return;
    }

    for(auto &i: recipeLists) {
        if(filename == i.filename){
            QMessageBox::warning(this,tr("Recipe list already added"), tr("The '%0' recipe list contained in %1 is already present").arg(i.name, i.filename));
            return;
        }
    }

    rl.name = QString::fromStdString(list["name"].as<std::string>());
    recipeLists.append(rl);

    for(YAML::const_iterator it=list["recipes"].begin();it!=list["recipes"].end();++it) {
        YAML::Node entry = *it;
        Reagent r;

        if(list["name"]) {
            r.recipelist = QString::fromStdString(list["name"].as<std::string>());
        } else {
            r.recipelist = tr("Unnamed recipe list");
        }

        r.name = QString::fromStdString(entry["name"].as<std::string>());
        r.fromFile = filename;

        for(YAML::const_iterator it=entry["ingredients"].begin();it!=entry["ingredients"].end();++it) {
            r.ingredients.append(QString::fromStdString(it->as<std::string>()));
        }

        for(YAML::const_iterator it=entry.begin();it!=entry.end();++it) {
            if(it->first.as<std::string>() == "ingredients")
                continue;

            if(it->second.IsMap()) {
                QMap<QString, QVariant> map;
                for(YAML::const_iterator m_it=it->second.begin();m_it!=it->second.end();++m_it) {
                    map[QString::fromStdString(m_it->first.as<std::string>())] = QString::fromStdString(m_it->second.as<std::string>());
                }
                r.properties[QString::fromStdString(it->first.as<std::string>())] = map;

            } else if(it->second.IsSequence()){
                QList<QVariant> sequence;
                for(YAML::const_iterator m_it=it->second.begin();m_it!=it->second.end();++m_it) {
                    YAML::Node sequence_entry = *m_it;
                    sequence.append(QString::fromStdString(sequence_entry.as<std::string>()));
                }
                r.properties[QString::fromStdString(it->first.as<std::string>())] = sequence;

            } else {
                r.properties[QString::fromStdString(it->first.as<std::string>())] = QString::fromStdString(it->second.as<std::string>());
            }

        }

        reagents.append(r);
    }

    reload_tag_cloud();
    reload_recipe_list();
    reload_recipelist_selector();
}

void MainWindow::reload_tag_cloud()
{
    static QStringList sizes {"small", "medium", "large", "x-large"};
    QMap<QString,int> tags;

    ui->tag_browser->clear();

    for(auto r: reagents) {
        if(r.properties.contains("tags")) {
            for(auto i: r.properties["tags"].toStringList()) {
                tags[i]++;
            }
        }
    }

    QList<int> tag_values = tags.values();
    QList<int>::const_iterator it_max = std::max_element(tag_values.constBegin(), tag_values.constEnd());
    float max = *it_max;

    QStringList links;
    for(auto it=tags.constBegin(); it != tags.constEnd(); ++it) {
        // Scale the sizes from 0 to sizes.count - 1
        float size = static_cast<float>(it.value())/max;
        links.append(QString("<a href=\"%0\"><span style=\"font-size:%1\">%0</span></a>").arg(it.key(), sizes[static_cast<int>(size*(sizes.length()-1))]));
    }

    ui->tag_browser->setHtml(links.join(" "));
    QTextCursor cursor = ui->tag_browser->textCursor();
    cursor.setPosition(0);
    ui->tag_browser->setTextCursor(cursor);
}

void MainWindow::reload_recipelist_selector()
{
    ui->recipelists_selector->clear();

    ui->recipelists_selector->addItem(tr("All recipe lists"),QVariant::fromValue<RecipeList*>(nullptr));

    for(auto &i: recipeLists) {
        ui->recipelists_selector->addItem(i.name, QVariant::fromValue(&i));
    }
}

void MainWindow::reload_recipelists(QStringList rl)
{
    recipeLists.clear();
    reagents.clear();
    recipelist_model->clear();

    for(auto &i: rl) {
        load_recipelist(i);
    }
    settings_changed = true;
}

void MainWindow::reload_recipe_list()
{
    recipelist_model->clear();

    for(auto &i: reagents) {
        QStandardItem* item = new QStandardItem(i.name);
        item->setData(QVariant::fromValue(&(i)), Qt::UserRole+1);
        recipelist_model->appendRow(item);
    }
    recipelist_model->sort(0);
}

void MainWindow::load_saved_recipelists()
{
    QSettings settings(SETTINGS_FILENAME, QSettings::IniFormat);
    int size = settings.beginReadArray("recipelists");

    for(int i=0; i< size; i++) {
        settings.setArrayIndex(i);
        QString filename = settings.value("file").toString();

        load_recipelist(filename);
    }
    settings.endArray();

}

QWidget* MainWindow::create_ingredient_tab(const Reagent *reagent)
{
    QWidget* w = new QWidget();
    QGridLayout* layout = new QGridLayout(w);
    QTableWidget* ingredient_table = new QTableWidget(0,1,w);

    ingredient_table->setHorizontalHeaderLabels({tr("Ingredient")});
    ingredient_table->verticalHeader()->setVisible(false);
    ingredient_table->horizontalHeader()->setStretchLastSection(true);
    ingredient_table->setShowGrid(false);
    ingredient_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    ingredient_table->setSelectionMode(QAbstractItemView::SingleSelection);
    ingredient_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ingredient_table->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    connect(ingredient_table, SIGNAL(cellDoubleClicked(int,int)), this, SLOT(ingredientlist_selection_doubleclicked(int,int)));

    layout->addWidget(ingredient_table);

    for(auto &i: reagent->ingredients) {
        QTableWidgetItem *newItem = new QTableWidgetItem(i);
        ingredient_table->insertRow(ingredient_table->rowCount());
        ingredient_table->setItem(ingredient_table->rowCount()-1, 0, newItem);
    }

    if(reagent->properties.contains("heat_to")) {
        QLabel* l = new QLabel(tr("Heat to %0").arg(reagent->properties["heat_to"].toString()));
        layout->addWidget(l);
    }

    if(reagent->properties.contains("reaction_message")) {
        QLabel* l = new QLabel(tr("Reaction message:\n%0").arg(reagent->properties["reaction_message"].toString()));
        l->setWordWrap(true);
        l->setSizePolicy(QSizePolicy::MinimumExpanding,QSizePolicy::Minimum);
        l->setAlignment(Qt::AlignTop);
        layout->addWidget(l);
    }

    w->setLayout(layout);

    return w;
}

QWidget* MainWindow::create_sources_tab(const Reagent *reagent)
{
    QWidget* w = new QWidget();
    QGridLayout* layout = new QGridLayout(w);
    QScrollArea* scroll_area = new QScrollArea(w);
    QGridLayout* scrollarea_layout = new QGridLayout(scroll_area);

    scroll_area->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    layout->addWidget(scroll_area);

    for(auto &i: reagent->properties["sources"].toList()) {
        QLabel *newSource = new QLabel(i.toString());
        newSource->setWordWrap(true);
        newSource->setSizePolicy(QSizePolicy::MinimumExpanding,QSizePolicy::Minimum);
        newSource->setAlignment(Qt::AlignTop);

        scrollarea_layout->addWidget(newSource);
    }

    scrollarea_layout->setRowStretch(scrollarea_layout->rowCount(),100);
    scroll_area->setLayout(scrollarea_layout);
    w->setLayout(layout);

    return w;
}

QWidget* MainWindow::create_usedin_tab(const Reagent *reagent)
{
    QWidget* w = new QWidget();
    QGridLayout* layout = new QGridLayout(w);
    QTableWidget* reagent_table = new QTableWidget(0,1,w);

    reagent_table->setHorizontalHeaderLabels({tr("Ingredient")});
    reagent_table->verticalHeader()->setVisible(false);
    reagent_table->horizontalHeader()->setStretchLastSection(true);
    reagent_table->setShowGrid(false);
    reagent_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    reagent_table->setSelectionMode(QAbstractItemView::SingleSelection);
    reagent_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    reagent_table->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    connect(reagent_table, SIGNAL(cellDoubleClicked(int,int)), this, SLOT(ingredientlist_selection_doubleclicked(int,int)));


    for(auto &i: reagents) {
        if(i.ingredients.contains(reagent->name, Qt::CaseInsensitive)) {
            QTableWidgetItem *newItem = new QTableWidgetItem(i.name);
            reagent_table->insertRow(reagent_table->rowCount());
            reagent_table->setItem(reagent_table->rowCount()-1, 0, newItem);
        }
    }

    if(reagent_table->rowCount() > 0) {
        layout->addWidget(reagent_table);
    } else {
        reagent_table->deleteLater();
        QLabel *l = new QLabel(tr("No uses found."));
        l->setAlignment(Qt::AlignCenter);
        layout->addWidget(l);
    }

    w->setLayout(layout);

    return w;
}

bool ReactionListLessThan(const MainWindow::ReactionStep &s1, const MainWindow::ReactionStep &s2)
{
    return s1.second > s2.second;
}

QList<MainWindow::ReactionStep> MainWindow::gather_reactions(Reagent reagent, int level)
{
    /*
     * QList of:
     * QPair:
     *   QPair:
     *     reagent name
     *     step type  - StepReagent - normal reagent
     *                  StepIntermediateResult - reaction result
     *                  StepInstruction - instructions
     *   reaction level - higher = must be made first
    */
    QList<ReactionStep> reagents_list;

    if(reagent.ingredients.isEmpty()) {
        reagents_list.append({{reagent,MainWindow::ReactionStepTypes::StepReagent}, level});
    } else {
        for(auto &ingredient: reagent.ingredients) {
            Reagent r;
            for(auto &i: reagents) {
                if(i.name.compare(ingredient, Qt::CaseInsensitive) == 0) {
                    r = i;
                    break;
                }
            }

            if(!r.name.isEmpty()) {
                reagents_list.append(gather_reactions(r, level+1));

            } else {
                r.name = ingredient;
                reagents_list.append({{r,MainWindow::ReactionStepTypes::StepReagent}, level+1});
            }
        }
        if(reagent.properties.contains("heat_to")) {
            reagents_list.append({{reagent, MainWindow::ReactionStepTypes::StepHeat}, level+1});
        }
        reagents_list.append({{reagent, MainWindow::ReactionStepTypes::StepIntermediateResult}, level+1});
    }

    return reagents_list;
}


QWidget *MainWindow::create_directions_tab(const Reagent* reagent)
{
    QWidget* w = new QWidget();
    QGridLayout* layout = new QGridLayout(w);
    QTableWidget* directions_table = new QTableWidget(0,1,w);

    directions_table->setHorizontalHeaderLabels({tr("Step")});
    directions_table->horizontalHeader()->setStretchLastSection(true);
    directions_table->setShowGrid(false);
    directions_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    directions_table->setSelectionMode(QAbstractItemView::SingleSelection);
    directions_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    directions_table->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::MinimumExpanding);
    directions_table->setShowGrid(true);

    layout->addWidget(directions_table);

    auto reaction_list = gather_reactions(*reagent);
    qStableSort(reaction_list.begin(),reaction_list.end(), ReactionListLessThan);

    for(auto &i: reaction_list) {
        QColor color = StepColors[i.first.second];
        QString text = QString((i.second-1)*3, ' ');

        switch(i.first.second) {
        case ReactionStepTypes::StepIntermediateResult:
            text += QString(tr("Makes: %0")).arg(i.first.first.name);
            break;
        case ReactionStepTypes::StepInstruction:
            text += QString(tr("Makes: %0")).arg(i.first.first.name);
            break;
        case ReactionStepTypes::StepHeat:
            text += QString(tr("Heat to %0")).arg(i.first.first.properties["heat_to"].toString());
            break;
        case ReactionStepTypes::StepReagent:
        default:
            text += i.first.first.name;
            break;
        }

        QTableWidgetItem *newItem = new QTableWidgetItem(text);
        newItem->setBackgroundColor(color);
        directions_table->insertRow(directions_table->rowCount());
        directions_table->setItem(directions_table->rowCount()-1, 0, newItem);
    }

    w->setLayout(layout);

    return w;
}

QWidget *MainWindow::create_info_tab(const Reagent *reagent)
{
    QWidget* w = new QWidget();
    QFormLayout* layout = new QFormLayout(w);
    layout->setRowWrapPolicy(QFormLayout::WrapLongRows);
    layout->setHorizontalSpacing(20);

    if(reagent->properties.contains("info")) {
        QMap<QString, QVariant> info = reagent->properties["info"].toMap();
        for(auto it = info.constBegin(); it != info.constEnd(); ++it) {
            QLabel *l = new QLabel(it.value().toString());
            l->setWordWrap(true);
            l->setSizePolicy(QSizePolicy::MinimumExpanding,QSizePolicy::Minimum);
            l->setAlignment(Qt::AlignTop);

            layout->addRow(QApplication::translate("InfoStrings", it.key().toAscii()), l);
        }
    }

    if(reagent->properties.contains("tags")) {
        QLabel *l = new QLabel(reagent->properties["tags"].toStringList().join(", "));
        l->setWordWrap(true);
        l->setSizePolicy(QSizePolicy::MinimumExpanding,QSizePolicy::Minimum);
        l->setAlignment(Qt::AlignTop);

        layout->addRow(tr("Tags"), l);
    }

    w->setLayout(layout);
    return w;
}

void MainWindow::reagentlist_selection_changed(const QItemSelection &selected/*, const QItemSelection &deselected*/)
{
    if(selected.length() == 0)
        return;

    for(int i = 0; i < ui->main_tabWidget->count(); i++) {
        ui->main_tabWidget->widget(i)->deleteLater();
    }

    ui->main_tabWidget->clear();
    Reagent* r = selected.indexes()[0].data(Qt::UserRole+1).value<Reagent*>();
    ui->main_groupbox->setTitle(r->name);

    if(r->ingredients.length() > 0) {
        QWidget* ing=create_ingredient_tab(r);
        ui->main_tabWidget->addTab(ing, tr("&Ingredients"));
        QWidget* dir=create_directions_tab(r);
        ui->main_tabWidget->addTab(dir, tr("&Directions"));
    }

    if(r->properties.contains("info") || r->properties.contains("tags")) {
        QWidget* w=create_info_tab(r);
        ui->main_tabWidget->addTab(w, tr("I&nformation"));
    }

    if(r->properties.contains("sources")) {
        QWidget* w=create_sources_tab(r);
        ui->main_tabWidget->addTab(w, tr("&Sources"));
    }

    QWidget* usedin=create_usedin_tab(r);
    ui->main_tabWidget->addTab(usedin, tr("&Used in"));
}

void MainWindow::ingredientlist_selection_doubleclicked(int x, int y)
{
    QTableWidget* ingredient_table = static_cast<QTableWidget*>(sender());

    // Find the selected reagent in all recipe lists, case insensitive
    QString reagent = ingredient_table->item(x,y)->text();

    QList<QStandardItem*> found_reagents = recipelist_model->findItems(reagent, Qt::MatchFixedString);

    if(!found_reagents.empty()) {
        // Try the current list first
        QModelIndex index = recipelist_proxy_model->mapFromSource(found_reagents[0]->index());
        if(!index.isValid()) {
            // Not in current recipe list, try all
            ui->search_filter->clear(); // Clear the filtering, so we have all the available reagents to select
            ui->recipelists_selector->setCurrentIndex(0); // Set the recipe list filter to all
            recipelist_proxy_model->setRecipeList(nullptr);
        }

        index = recipelist_proxy_model->mapFromSource(found_reagents[0]->index());

        ui->reagent_table->selectionModel()->reset();
        ui->reagent_table->selectionModel()->select(index, QItemSelectionModel::Select);
        ui->reagent_table->viewport()->update();
        ui->reagent_table->scrollTo(index);
    } else {
        QMessageBox::information(ingredient_table,tr("Reagent not found"), tr("Reagent %0 was not found in any of your recipe lists").arg(reagent));
    }
}

void MainWindow::save_settings()
{
    QSettings settings(SETTINGS_FILENAME, QSettings::IniFormat);
    settings.clear();
    settings.beginWriteArray("recipelists");
    for(int i=0; i < recipeLists.length(); ++i) {
        settings.setArrayIndex(i);
        settings.setValue("file", recipeLists[i].filename);
    }
    settings.endArray();

    settings.sync();

    settings_changed = false;

}

void MainWindow::closeEvent(QCloseEvent *event)
{
    setSetting("general/geometry", saveGeometry());
    setSetting("general/windowState", saveState());

    if (settings_changed) {
        QMessageBox::StandardButton save;
        save = QMessageBox::question(this, tr("Save before quitting?"), tr("Do you want to save modified settings befre exiting?"),QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel);
        if(save == QMessageBox::Yes) {
            save_settings();
            event->accept();
        } else if (save == QMessageBox::Cancel) {
            event->ignore();
        } else {
            event->accept();
        }
    } else {
        event->accept();
    }
}

void MainWindow::on_actionAdd_recipe_list_triggered()
{
    QString file = QFileDialog::getOpenFileName(this, tr("Add recipe list"));
    if(!file.isEmpty()) {
        settings_changed = true;
        load_recipelist(file);
    }
}

void MainWindow::on_recipelists_selector_currentIndexChanged(int index)
{
    RecipeList* r = ui->recipelists_selector->itemData(index, Qt::UserRole).value<RecipeList*>();
    recipelist_proxy_model->setRecipeList(r);
}

void MainWindow::on_action_Manage_recipe_lists_triggered()
{
    ManageDialog* dialog = new ManageDialog(this);
    QObject::connect(dialog, SIGNAL(reload_recipelists(QStringList)), this, SLOT(reload_recipelists(QStringList)));
    dialog->exec();
}

void MainWindow::on_action_Save_settings_triggered()
{
    save_settings();
    settings_changed = false;
}

void MainWindow::on_actionAbout_Recipe_Manager_triggered()
{

    QMessageBox::about(this, tr("About Recipe Manager"), trUtf8("Recipe Manager\n© 2013 by mysha (mysha@mysha.cu.cc)\n"
                                                            "\n"
                                                            "Donations in bitcoins or goon membership are appreciated.\n\n"
                                                            "BTC donation address: %0").arg("1Gzk3F4C4FiMVjTHCCkRuRwqZoCKujtBXd"));
}

void MainWindow::on_tag_browser_anchorClicked(const QUrl &arg1)
{
    ui->search_filter->setText(QString("tag:%0").arg(arg1.toString()));
}
