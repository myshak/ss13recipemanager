#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "yaml-cpp/yaml.h"

#include <algorithm>

#include <QDebug>
#include <QLabel>
#include <QCloseEvent>
#include <QMessageBox>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QTranslator>
#include <QFileDialog>
#include <QScrollArea>
#include <QPair>
#include <QMap>
#include <QRegExp>

#include "textdelegate.h"

#include "managedialog.h"
#include "optionsdialog.h"

#if 0
// Strings for lupdate
QStringList lupdate_unused = {
QT_TRANSLATE_NOOP("InfoStrings","depletion_rate"),
QT_TRANSLATE_NOOP("InfoStrings","penetrates_skin"),
QT_TRANSLATE_NOOP("InfoStrings","ignition_temp"),
QT_TRANSLATE_NOOP("InfoStrings","application_effect"),
QT_TRANSLATE_NOOP("InfoStrings","overdose_threshold"),
QT_TRANSLATE_NOOP("InfoStrings","notes"),
QT_TRANSLATE_NOOP("InfoStrings","per_cycle"),
QT_TRANSLATE_NOOP("InfoStrings","description"),
QT_TRANSLATE_NOOP("InfoStrings","result"),
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
    ui(new Ui::MainWindow),
    indentation_level(3),
    directions_style(DirectionsStyle::Normal)
{
    ui->setupUi(this);

    recipelist_model = new QStandardItemModel(this);
    recipelist_proxy_model = new RecipeListProxyModel(this);
    recipelist_proxy_model->setSourceModel(recipelist_model);
    recipelist_proxy_model->setFilterCaseSensitivity(Qt::CaseInsensitive);

    ui->reagent_table->setModel(recipelist_proxy_model);
#if QT_VERSION >= 0x050000
    ui->reagent_table->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
#else
    ui->reagent_table->verticalHeader()->setResizeMode(QHeaderView::ResizeToContents);
#endif

    ui->splitter->setSizes({300,700});
    ui->recipelists_selector->addItem(tr("All recipe lists"),QVariant::fromValue<RecipeList*>(nullptr));

    connect(ui->reagent_table->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(reagentlist_selection_changed(QItemSelection/*,QItemSelection*/)));
    connect(ui->search_filter, SIGNAL(textChanged(QString)), recipelist_proxy_model, SLOT(setFilterString(QString)));

    ui->menuWindow->addAction(ui->dockWidget->toggleViewAction());

    load_settings();

    load_saved_recipelists();
}

MainWindow::~MainWindow()
{
    delete ui;
}

Reagent::IngredientData get_tag_list(const QString& text)
{
    Reagent::IngredientData result;

    result.name = text;

    // Check if the name has tags in it
    int tags_start = text.lastIndexOf("@");
    if(tags_start != -1) {
        // Split the name and the tags
        result.name = text.left(tags_start).trimmed();
        result.tags = text.mid(tags_start+1).split(",", QString::SkipEmptyParts);
    }

    result.tags.replaceInStrings(" ","");


    return result;
}

Reagent::ReagentStep get_reagent_step(const QString& text)
{
    QString text_clean;
    QString text_html;
    Reagent::ReagentStep result;

    QRegExp regex("<([^>]+)>");
    int pos = 0;
    int prev_pos = 0;

    while ((pos = regex.indexIn(text, pos)) != -1) {
        Reagent::IngredientData ing = get_tag_list(regex.cap(1));
        result.ingredients.append(ing);

        text_clean += text.mid(prev_pos, pos-prev_pos);
        text_clean += ing.name;

        text_html += text.mid(prev_pos, pos-prev_pos);
        text_html += QString("<a href=\"%0\">%0</a>").arg(ing.name);


        pos += regex.matchedLength();
        prev_pos = pos;
    }

    text_clean += text.mid(prev_pos);
    text_html += text.mid(prev_pos);

    // If we didn't find any Ingredients in <>, add one from the whole text
    if(result.ingredients.isEmpty()) {
        Reagent::IngredientData ing = get_tag_list(text);
        result.ingredients.append(ing);
        text_clean = ing.name;
        text_html = ing.name;
    }

    result.text_plain = text_clean;
    result.text_html = text_html;

    return result;
}

void MainWindow::load_recipelist(QString filename)
{
    RecipeList rl;
    rl.filename = filename;

    YAML::Node list;
    try {
        list = YAML::LoadFile(filename.toStdString());
    } catch(YAML::Exception& e) {
        QMessageBox::critical(this, tr("Error loading recipe list"), tr("There was an error during loading of:\n%0\n\n%1").arg(filename,QString::fromStdString(e.what())));
        return;
    }

    for(auto &i: recipeLists) {
        if(filename == i.filename){
            QMessageBox::warning(this,tr("Recipe list already added"), tr("The '%0' recipe list contained in %1 is already present").arg(i.name, i.filename));
            return;
        }
    }

    try {

        rl.name = QString::fromUtf8(list["name"].as<std::string>().c_str());
        recipeLists.append(rl);

        for(YAML::const_iterator it=list["recipes"].begin();it!=list["recipes"].end();++it) {
            YAML::Node entry = *it;
            Reagent r;

            if(list["name"]) {
                r.recipelist = QString::fromUtf8(list["name"].as<std::string>().c_str());
            } else {
                r.recipelist = tr("Unnamed recipe list");
            }

            r.name = QString::fromUtf8(entry["name"].as<std::string>().c_str());
            r.fromFile = filename;

            for(YAML::const_iterator i_it=entry["ingredients"].begin();i_it!=entry["ingredients"].end();++i_it) {
                QString text = QString::fromUtf8(i_it->as<std::string>().c_str());


                if(i_it->Tag() == "!step") {
                    // Custom instruction text
                    Reagent::ReagentStep step = get_reagent_step(text);
                    step.type = Reagent::Instruction;
                    r.ingredients.append(step);
                } else {
                    // Normal reagent
                    Reagent::ReagentStep step=get_reagent_step(text);
                    step.type = Reagent::Ingredient;

                    r.ingredients.append(step);
                }

            }

            for(YAML::const_iterator p_it=entry.begin();p_it!=entry.end();++p_it) {
                if(p_it->first.as<std::string>() == "ingredients")
                    continue;

                if(p_it->second.IsMap()) {
                    QMap<QString, QVariant> map;
                    for(YAML::const_iterator m_it=p_it->second.begin();m_it!=p_it->second.end();++m_it) {
                        map[QString::fromUtf8(m_it->first.as<std::string>().c_str())] = QString::fromUtf8(m_it->second.as<std::string>().c_str());
                    }
                    r.properties[QString::fromUtf8(p_it->first.as<std::string>().c_str())] = map;

                } else if(p_it->second.IsSequence()){
                    QList<QVariant> sequence;
                    for(YAML::const_iterator m_it=p_it->second.begin();m_it!=p_it->second.end();++m_it) {
                        YAML::Node sequence_entry = *m_it;
                        sequence.append(QString::fromUtf8(sequence_entry.as<std::string>().c_str()));
                    }
                    r.properties[QString::fromUtf8(p_it->first.as<std::string>().c_str())] = sequence;

                } else {
                    r.properties[QString::fromUtf8(p_it->first.as<std::string>().c_str())] = QString::fromUtf8(p_it->second.as<std::string>().c_str());
                }

            }

            reagents.append(r);
        }
    } catch(YAML::Exception& e) {
        QMessageBox::critical(this, tr("Error loading recipe list"), tr("There was an error during loading of:\n%0\n\n%1").arg(filename,QString::fromStdString(e.what())));
        return;
    }

    reload_tag_cloud();
    reload_recipe_list();
    reload_recipelist_selector();
}

void MainWindow::reload_tag_cloud()
{
    static const float size_min = 7.5f;
    static const float size_max = 18.0f;
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
        // Scale the sizes from size_min to size_max logarithmically
        float size = ( log(*it) / log(max) ) * (size_max - size_min) + size_min;
        links.append(QString("<a href=\"%0\"><span style=\"font-size:%1pt\">%0</span></a>").arg(it.key()).arg(static_cast<int>(size)));
    }

    ui->tag_browser->setHtml(links.join(" "));
    QTextCursor cursor = ui->tag_browser->textCursor();
    cursor.setPosition(0);
    ui->tag_browser->setTextCursor(cursor);
}

void MainWindow::reload_recipelist_selector()
{
    ui->recipelists_selector->clear();

    ui->recipelists_selector->addItem(tr("All recipe lists") + QString(" (%0)").arg(reagents.size()),QVariant::fromValue<RecipeList*>(nullptr));

    for(auto &i: recipeLists) {
        int count = 0;
        for(Reagent &r: reagents) {
            if( r.fromFile == i.filename && r.recipelist == i.name) {
                ++count;
            }
        }
        ui->recipelists_selector->addItem(i.name+QString(" (%0)").arg(count), QVariant::fromValue(&i));
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

    if(reagent->properties.contains("heat_to")) {
        QLabel* l = new QLabel(QString(R"(<img src=":/icons/heat.png"/> %0)").arg(reagent->properties["heat_to"].toString()));
        l->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Minimum);
        l->setFrameShape(QLabel::StyledPanel);
        l->setStyleSheet("background-color: #FFDCDC;");
        l->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        layout->addWidget(l);
    }

    if(reagent->properties.contains("reaction_message")) {
        QLabel* l = new QLabel(QString(R"(<img src=":/icons/reaction.png"/> %0)").arg(reagent->properties["reaction_message"].toString()));
        l->setWordWrap(true);
        l->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Minimum);
        l->setAlignment(Qt::AlignTop);
        l->setFrameShape(QLabel::StyledPanel);
        l->setStyleSheet("background-color: cornsilk;");
        l->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        layout->addWidget(l);
    }

    layout->addWidget(ingredient_table);

    ingredient_table->setHorizontalHeaderLabels({tr("Ingredient")});
    ingredient_table->horizontalHeader()->setStretchLastSection(true);
#if QT_VERSION >= 0x050000
    ingredient_table->horizontalHeader()->setSectionsClickable(false);
#else
    ingredient_table->horizontalHeader()->setClickable(false);
#endif

    ingredient_table->setShowGrid(false);
    ingredient_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    ingredient_table->setSelectionMode(QAbstractItemView::SingleSelection);
    ingredient_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ingredient_table->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    ingredient_table->verticalHeader()->setVisible(false);

    TextDelegate *tg = new TextDelegate(ingredient_table);
    connect(tg, SIGNAL(anchor_clicked(QString, QModelIndex)), this, SLOT(reagentlist_select(QString,QModelIndex)));
    ingredient_table->setItemDelegate(tg);

    for(auto i: reagent->ingredients) {
        if(i.type != Reagent::Ingredient) {
            // Skip the non-reagent steps
            continue;
        }

        QTableWidgetItem *newItem = new QTableWidgetItem(i.text_html);
        newItem->setData(Qt::UserRole+1, QVariant::fromValue(i));
        ingredient_table->insertRow(ingredient_table->rowCount());
        ingredient_table->setItem(ingredient_table->rowCount()-1, 0, newItem);
    }

    w->setLayout(layout);

    connect(ingredient_table->horizontalHeader(), SIGNAL(sectionResized(int, int, int)), ingredient_table, SLOT(resizeRowsToContents()));
    connect(ingredient_table->horizontalHeader(), SIGNAL(geometriesChanged()), ingredient_table, SLOT(resizeRowsToContents()));

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
    reagent_table->horizontalHeader()->setStretchLastSection(true);
#if QT_VERSION >= 0x050000
    reagent_table->horizontalHeader()->setSectionsClickable(false);
    reagent_table->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
#else
    reagent_table->horizontalHeader()->setClickable(false);
    reagent_table->verticalHeader()->setResizeMode(QHeaderView::ResizeToContents);
#endif
    reagent_table->setShowGrid(false);
    reagent_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    reagent_table->setSelectionMode(QAbstractItemView::SingleSelection);
    reagent_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    reagent_table->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    reagent_table->verticalHeader()->setVisible(false);
    connect(reagent_table, SIGNAL(cellDoubleClicked(int,int)), this, SLOT(ingredientlist_selection_doubleclicked(int,int)));


    for(auto &i: reagents) {
        if(i.contains_ingredient(reagent->name)) {
            QTableWidgetItem *newItem = new QTableWidgetItem(i.name);
            newItem->setData(Qt::UserRole+1, QVariant::fromValue(Reagent::ReagentStep{i.name, i.name, {{i.name, i.properties["tags"].toStringList()}}, Reagent::StepTypes::Ingredient}));
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

    connect(reagent_table->horizontalHeader(), SIGNAL(sectionResized(int, int, int)), reagent_table, SLOT(resizeRowsToContents()));
    connect(reagent_table->horizontalHeader(), SIGNAL(geometriesChanged()), reagent_table, SLOT(resizeRowsToContents()));

    return w;
}

bool ReactionListLessThan(const MainWindow::ReactionStep &s1, const MainWindow::ReactionStep &s2)
{
    return s1.total_depth < s2.total_depth;
}

bool ReactionListGreaterThan(const MainWindow::ReactionStep &s1, const MainWindow::ReactionStep &s2)
{
    return s1.total_depth > s2.total_depth;
}

MainWindow::ReactionStep MainWindow::gather_reactions(Reagent reagent)
{
    QList<ReactionStep> reagents_list;
    ReactionStep step;
    int depth=0;
    int sub_max_depth=0;

    step = {reagent.name, {}, ReactionStepTypes::StepReagent, depth};

    if(!reagent.ingredients.isEmpty()) {
        depth++;
        step.type = ReactionStepTypes::StepIntermediateResult;
        for(auto ingredient: reagent.ingredients) {
            if(ingredient.type == Reagent::Ingredient) {
                Reagent r;
                for(auto i: reagents) {
                    if((i.name.compare(ingredient.text_plain, Qt::CaseInsensitive) == 0 ||
                       (i.name.compare(ingredient.ingredients[0].name, Qt::CaseInsensitive) == 0)) &&
                        i.matches_tags(ingredient.ingredients[0].tags)) {
                        r = i;
                        break;
                    }
                }

                if(!r.name.isEmpty()) {
                    ReactionStep new_step = gather_reactions(r);
                    if(sub_max_depth < new_step.total_depth) {
                        sub_max_depth = new_step.total_depth;
                    }
                    reagents_list.append(new_step);
                } else {
                    reagents_list.append({ingredient.ingredients[0].name, {}, ReactionStepTypes::StepReagent, 0});
                }
            } else {
                reagents_list.append({ingredient.text_plain, {}, ReactionStepTypes::StepInstruction, 0});
            }
        }

        step.entry.setValue(reagents_list);
        step.total_depth = depth + sub_max_depth;
    }

    return step;
}

void MainWindow::fill_directions(QTableWidget* table, ReactionStep step, bool sort, int current_depth)
{
    if(directions_style == DirectionsStyle::Normal) {
        if(!step.entry.isNull()) {
            auto steps = step.entry.value<QList<ReactionStep>>();
            if(sort) {
                qStableSort(steps.begin(),steps.end(), ReactionListGreaterThan);
            }

            for(auto i: steps) {
                fill_directions(table, i, current_depth + 1);
            }
        }
    }

    QColor color = StepColors[step.type];
    QString text = QString((current_depth)*indentation_level, ' ');

    switch(step.type) {
    case ReactionStepTypes::StepIntermediateResult:
        if(directions_style == DirectionsStyle::Normal) {
            text += QString(tr("Makes: %0")).arg(step.reagent_name);
        } else {
            text += QString("%0:").arg(step.reagent_name);
        }
        break;
    case ReactionStepTypes::StepInstruction:
        text += QString(tr("Step: %0")).arg(step.reagent_name);
        break;
    case ReactionStepTypes::StepReagent:
    default:
        text += step.reagent_name;
        break;
    }

    QTableWidgetItem *newItem = new QTableWidgetItem(text);
    newItem->setBackgroundColor(color);
    table->insertRow(table->rowCount());
    table->setItem(table->rowCount()-1, 0, newItem);

    if(directions_style == DirectionsStyle::Inverted) {
        if(!step.entry.isNull()) {
            auto steps = step.entry.value<QList<ReactionStep>>();
            if(sort) {
                qStableSort(steps.begin(),steps.end(), ReactionListLessThan);
            }
            for(auto i: steps) {
                fill_directions(table, i, current_depth + 1);
            }
        }
    }

}

void MainWindow::fill_directions(QTreeWidgetItem* item, ReactionStep step, bool sort, int current_depth)
{
    QColor color = StepColors[step.type];
    QString text;

    switch(step.type) {
    case ReactionStepTypes::StepIntermediateResult:
        text += QString("%0").arg(step.reagent_name);
        break;
    case ReactionStepTypes::StepInstruction:
        text += QString(tr("Step: %0")).arg(step.reagent_name);
        break;
    case ReactionStepTypes::StepReagent:
    default:
        text += step.reagent_name;
        break;
    }

    QTreeWidgetItem *newItem = new QTreeWidgetItem({text});
    newItem->setBackgroundColor(0, color);
    newItem->setToolTip(0, text);
    item->addChild(newItem);

    if(!step.entry.isNull()) {
        auto steps = step.entry.value<QList<ReactionStep>>();
        if(sort) {
            qStableSort(steps.begin(),steps.end(), ReactionListGreaterThan);
        }
        for(auto i: steps) {
            fill_directions(newItem, i, current_depth + 1);
        }
    }


}

QWidget *MainWindow::create_table_directions(const Reagent* reagent, QWidget* parent)
{
    QTableWidget* directions_table = new QTableWidget(0,1,parent);

    directions_table->setHorizontalHeaderLabels({tr("Step")});
    directions_table->horizontalHeader()->setStretchLastSection(true);

#if QT_VERSION >= 0x050000
    directions_table->horizontalHeader()->setSectionsClickable(false);
    directions_table->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    directions_table->verticalHeader()->setSectionsClickable(false);
#else
    directions_table->horizontalHeader()->setClickable(false);
    directions_table->verticalHeader()->setResizeMode(QHeaderView::ResizeToContents);
    directions_table->verticalHeader()->setClickable(false);
#endif
    directions_table->setShowGrid(false);
    directions_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    directions_table->setSelectionMode(QAbstractItemView::SingleSelection);
    directions_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    directions_table->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::MinimumExpanding);
    directions_table->setShowGrid(true);
    directions_table->setWordWrap(true);
    directions_table->setCornerButtonEnabled(false);

    auto reaction_list = gather_reactions(*reagent);
    bool sort = true;
    if(reagent->properties.contains("unsorted") && reagent->properties["unsorted"].toBool() == true) {
        sort = false;
    }
    fill_directions(directions_table, reaction_list, sort);

    connect(directions_table->horizontalHeader(), SIGNAL(sectionResized(int, int, int)), directions_table, SLOT(resizeRowsToContents()));
    connect(directions_table->horizontalHeader(), SIGNAL(geometriesChanged()), directions_table, SLOT(resizeRowsToContents()));

    return directions_table;
}

QWidget *MainWindow::create_tree_directions(const Reagent* reagent, QWidget* parent)
{
    QTreeWidget* directions_tree = new QTreeWidget(parent);

    directions_tree->setColumnCount(1);
    directions_tree->setHeaderLabel(tr("Step"));
#if QT_VERSION >= 0x050000
    directions_tree->header()->setSectionsClickable(false);
#else
    directions_tree->header()->setClickable(false);
#endif
    directions_tree->setRootIsDecorated(false);
    directions_tree->setAnimated(tree_animated);
    directions_tree->setSelectionBehavior(QAbstractItemView::SelectRows);
    directions_tree->setSelectionMode(QAbstractItemView::SingleSelection);
    directions_tree->setEditTriggers(QAbstractItemView::NoEditTriggers);
    directions_tree->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::MinimumExpanding);
    directions_tree->setWordWrap(true);

    auto reaction_list = gather_reactions(*reagent);
    bool sort = true;
    if(reagent->properties.contains("unsorted") && reagent->properties["unsorted"].toBool() == true) {
        sort = false;
    }
    fill_directions(directions_tree->invisibleRootItem(), reaction_list, sort);

    directions_tree->expandAll();

    return directions_tree;
}

QWidget *MainWindow::create_directions_tab(const Reagent* reagent)
{
    QWidget* w = new QWidget();
    QGridLayout* layout = new QGridLayout(w);

    if(directions_style == DirectionsStyle::Tree) {
        layout->addWidget(create_tree_directions(reagent, w));
    } else {
        layout->addWidget(create_table_directions(reagent, w));
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

    QLabel *l = new QLabel(reagent->recipelist);
    l->setWordWrap(true);
    l->setSizePolicy(QSizePolicy::MinimumExpanding,QSizePolicy::Minimum);
    l->setAlignment(Qt::AlignTop);

    layout->addRow(tr("Recipe list"), l);

    if(reagent->properties.contains("info")) {
        QMap<QString, QVariant> info = reagent->properties["info"].toMap();
        for(auto it = info.constBegin(); it != info.constEnd(); ++it) {
            QLabel *l = new QLabel(it.value().toString());
            l->setWordWrap(true);
            l->setSizePolicy(QSizePolicy::MinimumExpanding,QSizePolicy::Minimum);
            l->setAlignment(Qt::AlignTop);

            layout->addRow(QApplication::translate("InfoStrings", it.key().toLatin1()), l);
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

    ui->main_tabWidget->setUpdatesEnabled(false);
    ui->main_tabWidget->clear();
    Reagent* r = selected.indexes()[0].data(Qt::UserRole+1).value<Reagent*>();
    Q_ASSERT(r);

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

    ui->main_tabWidget->setUpdatesEnabled(true);
}

void MainWindow::ingredientlist_selection_doubleclicked(int x, int y)
{
    QTableWidget* ingredient_table = static_cast<QTableWidget*>(sender());

    QModelIndex index = ingredient_table->model()->index(x,y);
    Reagent::ReagentStep reagent = index.data(Qt::UserRole+1).value<Reagent::ReagentStep>();

    reagentlist_select(reagent.ingredients[0].name, index);
}

void MainWindow::reagentlist_select(const QString &reagent, const QModelIndex& model_index)
{
    // Find the selected reagent by name in all recipe lists, case insensitive
    QList<QStandardItem*> found_reagents = recipelist_model->findItems(reagent, Qt::MatchFixedString);
    QList<QStandardItem*> current_list_reagents;

    // Filter reagents matching tags
    for(QStandardItem* i: found_reagents) {
        Reagent* r = i->data(Qt::UserRole+1).value<Reagent*>();

        Q_ASSERT(r);

        if(model_index.data(Qt::UserRole+1).canConvert<Reagent::ReagentStep>()) {
            Reagent::ReagentStep ingredient = model_index.data(Qt::UserRole+1).value<Reagent::ReagentStep>();
            for(auto ing: ingredient.ingredients) {
                if(ing.name == reagent )
                    if( r->matches_tags(ing.tags)) {
                        current_list_reagents.append(i);
                    }
            }
        } else {
            if(r->name == reagent) {
                current_list_reagents.append(i);
            }
        }
    }

    if(!current_list_reagents.empty()) {
        // Try the current list first
        // We use the first reagent that matches the name and tags
        QModelIndex index = recipelist_proxy_model->mapFromSource(current_list_reagents[0]->index());

        if(!index.isValid()) {
            // Not in current recipe list, try all
            ui->search_filter->clear(); // Clear the filtering, so we have all the available reagents to select
            ui->recipelists_selector->setCurrentIndex(0); // Set the recipe list filter to all
            recipelist_proxy_model->setRecipeList(nullptr);
            index = recipelist_proxy_model->mapFromSource(current_list_reagents[0]->index());
        }

        // The index must be valid, because we found the reagent in the master recipe list
        Q_ASSERT(index.isValid());

        ui->reagent_table->selectionModel()->reset();
        ui->reagent_table->selectionModel()->select(index, QItemSelectionModel::Select);
        ui->reagent_table->viewport()->update();
        ui->reagent_table->repaint();
        ui->reagent_table->scrollTo(index);
    } else {
        QMessageBox::information(this,tr("Item not found"), tr("Item %0 was not found in any of your recipe lists").arg(reagent));
    }
}

void MainWindow::load_settings()
{
    restoreGeometry(getSetting<QByteArray>("General/geometry"));
    restoreState(getSetting<QByteArray>("General/windowState"));

    indentation_level = getSetting<int>("General/indentation_level", 3);
    tree_animated = getSetting<bool>("General/tree_animated", true);
    // QVariant.value<enum type>() doesn't seem to work. Cast it from int manually
    directions_style = static_cast<DirectionsStyle>(getSetting<quint32>("General/directions_style", DirectionsStyle::Normal));
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

    settings.setValue("General/indentation_level", indentation_level);
    settings.setValue("General/directions_style", directions_style);
    settings.setValue("General/tree_animated", tree_animated);

    settings.sync();

    settings_changed = false;

}

void MainWindow::closeEvent(QCloseEvent *event)
{
    setSetting("General/geometry", saveGeometry());
    setSetting("General/windowState", saveState());

    if (settings_changed) {
        QMessageBox::StandardButton save;
        save = QMessageBox::question(this, tr("Save before quitting?"), tr("Do you want to save modified settings before exiting?"),QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel);
        if(save == QMessageBox::Yes) {
            save_settings();
            setSetting("General/geometry", saveGeometry());
            setSetting("General/windowState", saveState());
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

    QMessageBox::about(this, tr("About Recipe Manager"), trUtf8("Recipe Manager\n© 2013-2014 by mysha (mysha@mysha.cu.cc)\n"
                                                            "Version " VERSION_STRING "\n"
                                                            "\n"
                                                            "Donations in bitcoins are appreciated.\n\n"
                                                            "BTC donation address: %0").arg("1Gzk3F4C4FiMVjTHCCkRuRwqZoCKujtBXd"));
}

void MainWindow::on_tag_browser_anchorClicked(const QUrl &arg1)
{
    ui->recipelists_selector->setCurrentIndex(0);
    ui->search_filter->setText(QString("tag:%0").arg(arg1.toString()));
}

void MainWindow::on_action_Options_triggered()
{
    OptionsDialog* options = new OptionsDialog(indentation_level, directions_style, tree_animated, this);
    QObject::connect(options, SIGNAL(indentation_changed(int)), this, SLOT(set_indentation(int)));
    QObject::connect(options, SIGNAL(tree_animated_changed(bool)), this, SLOT(set_tree_animated(bool)));
    QObject::connect(options, SIGNAL(directions_style_changed(MainWindow::DirectionsStyle)), this, SLOT(set_directions_style(MainWindow::DirectionsStyle)));
    options->exec();
}

void MainWindow::set_indentation(int level)
{
    settings_changed = true;
    indentation_level = level;
    if(directions_style != DirectionsStyle::Tree) {
        reagentlist_selection_changed(ui->reagent_table->selectionModel()->selection());
    }
}

void MainWindow::set_tree_animated(bool animated)
{
    settings_changed = true;
    tree_animated = animated;
    if(directions_style == DirectionsStyle::Tree) {
        reagentlist_selection_changed(ui->reagent_table->selectionModel()->selection());
    }
}

void MainWindow::set_directions_style(MainWindow::DirectionsStyle style)
{
    settings_changed = true;
    directions_style = style;
    reagentlist_selection_changed(ui->reagent_table->selectionModel()->selection());
}
