#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "yaml-cpp/yaml.h"

#include <QDebug>
#include <QLabel>
#include <QCloseEvent>
#include <QTableWidget>
#include <QMessageBox>
#include <QFormLayout>
#include <QTranslator>
#include <QFileDialog>


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

    ui->splitter->setSizes({300,800});
    ui->recipelists_selector->addItem("All",QVariant::fromValue<RecipeList*>(nullptr));

    connect(ui->reagent_table->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(reagentlist_selection_changed(QItemSelection,QItemSelection)));
    connect(ui->search_filter, SIGNAL(textChanged(QString)), recipelist_proxy_model, SLOT(setFilterFixedString(QString)));

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

    YAML::Node list = YAML::LoadFile(filename.toStdString());

    for(auto &i: recipeLists) {
        if(filename == i.filename){
            QMessageBox::warning(this,tr("Recipe list already added"), tr("The '%0' recipe list contained in %1 is already present").arg(i.name, i.filename));
            return;
        }
    }

    rl.name = QString::fromStdString(list["name"].as<std::string>());
    recipeLists.append(rl);
    ui->recipelists_selector->addItem(rl.name, QVariant::fromValue(&recipeLists.last()));

    for(YAML::const_iterator it=list["recipes"].begin();it!=list["recipes"].end();++it) {
        YAML::Node entry = *it;
        Reagent r;

        if(list["name"]) {
            r.recipelist = QString::fromStdString(list["name"].as<std::string>());
        } else {
            r.recipelist = "Unnamed recipe list";
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
                QHash<QString, QVariant> map;
                for(YAML::const_iterator m_it=it->second.begin();m_it!=it->second.end();++m_it) {
                    map[QString::fromStdString(m_it->first.as<std::string>())] = QString::fromStdString(m_it->second.as<std::string>());
                }
                r.properties[QString::fromStdString(it->first.as<std::string>())] = map;
            } else {
                r.properties[QString::fromStdString(it->first.as<std::string>())] = QString::fromStdString(it->second.as<std::string>());
            }

        }

        reagents.append(r);
        QStandardItem* item = new QStandardItem(r.name);
        item->setData(QVariant::fromValue(&(reagents.last())), Qt::UserRole+1);
        recipelist_model->appendRow(item);
    }
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

    ingredient_table->setHorizontalHeaderLabels({"Ingredient"});
    ingredient_table->verticalHeader()->setVisible(false);
    ingredient_table->horizontalHeader()->setStretchLastSection(true);
    ingredient_table->setShowGrid(false);
    ingredient_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    ingredient_table->setSelectionMode(QAbstractItemView::SingleSelection);
    ingredient_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ingredient_table->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    connect(ingredient_table, SIGNAL(cellDoubleClicked(int,int)), this, SLOT(ingredientlist_selection_doubleclicked(int,int)));

    layout->addWidget(ingredient_table);

    for(auto &i: reagent->ingredients) {
        QTableWidgetItem *newItem = new QTableWidgetItem(i);
        ingredient_table->insertRow(ingredient_table->rowCount());
        ingredient_table->setItem(ingredient_table->rowCount()-1, 0, newItem);
    }
    ingredient_table->resizeRowsToContents();

    if(reagent->properties.contains("heat_to")) {
        QLabel* l = new QLabel(QString("Heat to %0 degrees.").arg(reagent->properties["heat_to"].toString()));
        layout->addWidget(l);
    }
    layout->setRowStretch(layout->rowCount(),100);
    w->setLayout(layout);

    return w;
}

QWidget *MainWindow::create_info_tab(const Reagent *reagent)
{
    QWidget* w = new QWidget();
    QFormLayout* layout = new QFormLayout(w);
    layout->setRowWrapPolicy(QFormLayout::WrapLongRows);
    layout->setHorizontalSpacing(20);

    QHash<QString, QVariant> info = reagent->properties["info"].toHash();
    for(auto it = info.constBegin(); it != info.constEnd(); ++it) {
        QLabel *l = new QLabel(it.value().toString());
        l->setWordWrap(true);
        l->setSizePolicy(QSizePolicy::MinimumExpanding,QSizePolicy::Minimum);
        l->setAlignment(Qt::AlignTop);

        layout->addRow(QApplication::translate("InfoTab", it.key().toAscii()), l);
    }

    w->setLayout(layout);
    return w;
}

void MainWindow::reagentlist_selection_changed(const QItemSelection &selected, const QItemSelection &deselected)
{
    if(selected.length() == 0)
        return;

    for(int i = 0; i < ui->tabWidget->count(); i++) {
        ui->tabWidget->widget(i)->deleteLater();
    }

    ui->tabWidget->clear();
    Reagent* r = selected.indexes()[0].data(Qt::UserRole+1).value<Reagent*>();
    ui->groupBox->setTitle(r->name);

    if(r->ingredients.length() > 0) {
        QWidget* w=create_ingredient_tab(r);
        ui->tabWidget->addTab(w, "Ingredients");
    }

    if(r->properties.contains("info")) {
        QWidget* w=create_info_tab(r);
        ui->tabWidget->addTab(w, "Information");
    }
}

void MainWindow::ingredientlist_selection_doubleclicked(int x, int y)
{
    QTableWidget* ingredient_table = static_cast<QTableWidget*>(sender());
    QString reagent = ingredient_table->item(x,y)->text();
    QList<QStandardItem*> found_reagents = recipelist_model->findItems(reagent);

    if(!found_reagents.empty()) {
        ui->reagent_table->selectionModel()->clearSelection();
        ui->search_filter->clear(); // Clear the filtering, so we have all the available reagents
        QModelIndex i = recipelist_proxy_model->mapFromSource(found_reagents[0]->index());
        ui->reagent_table->selectionModel()->select(i, QItemSelectionModel::Select);
    } else {
        QMessageBox::information(ingredient_table,tr("Reagent not found"), QString("Reagent %0 was not found in any of your recipe lists").arg(reagent));
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

}

void MainWindow::closeEvent(QCloseEvent *event)
{
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
        load_recipelist(file);
    }
    settings_changed = true;
}

void MainWindow::on_recipelists_selector_currentIndexChanged(int index)
{
    RecipeList* r = ui->recipelists_selector->itemData(index, Qt::UserRole).value<RecipeList*>();
    recipelist_proxy_model->setRecipeList(r);
}
