#include "managedialog.h"
#include "ui_managedialog.h"

#include "mainwindow.h"
#include "recipelist.h"

#include <QFileDialog>

ManageDialog::ManageDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ManageDialog)
{
    ui->setupUi(this);

    MainWindow* mw =static_cast<MainWindow*>(parentWidget());
    for(auto &i: mw->recipeLists) {
        recipelists.append(i);
    }
    reload_recipelists_table();
}

ManageDialog::~ManageDialog()
{
    delete ui;
}

void ManageDialog::reload_recipelists_table()
{
    for(auto &i: recipelists) {
        QTableWidgetItem *rl = new QTableWidgetItem(i.name);
        rl->setData(Qt::UserRole+1, QVariant::fromValue(&(i)));
        ui->recipelist_table->insertRow(ui->recipelist_table->rowCount());
        ui->recipelist_table->setItem(ui->recipelist_table->rowCount()-1, 0, rl);
    }
}

void ManageDialog::on_remove_button_clicked()
{
    QTableWidgetItem* item = ui->recipelist_table->currentItem();
    if(item) {
        RecipeList* rl = item->data(Qt::UserRole+1).value<RecipeList*>();
        if(rl) {
            recipelists.removeAll(*rl);
            ui->recipelist_table->removeRow(item->row());
        }
    }
}

void ManageDialog::accept()
{
    QStringList rl;
    for(auto &i: recipelists) {
        rl.append(i.filename);
    }

    emit reload_recipelists(rl);

    QDialog::accept();
}
