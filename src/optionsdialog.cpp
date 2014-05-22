#include "optionsdialog.h"
#include "ui_optionsdialog.h"

OptionsDialog::OptionsDialog(int indentation, MainWindow::DirectionsStyle style, QWidget *parent):
    QDialog(parent),
    ui(new Ui::Options)
{
    ui->setupUi(this);

    ui->indentation_level->setValue(indentation);

    switch(style) {
    case MainWindow::DirectionsStyle::Normal:
        ui->style_normal->setChecked(true);
        break;
    case MainWindow::DirectionsStyle::Inverted:
        ui->style_inverted->setChecked(true);
        break;
    case MainWindow::DirectionsStyle::Tree:
        ui->style_tree->setChecked(true);
        break;
    }
}

OptionsDialog::~OptionsDialog()
{
    delete ui;
}

void OptionsDialog::accept()
{
    emit indentation_changed(ui->indentation_level->value());

    MainWindow::DirectionsStyle style = MainWindow::DirectionsStyle::Normal;
    if(ui->style_normal->isChecked()) {
        style = MainWindow::DirectionsStyle::Normal;
    } else if(ui->style_inverted->isChecked()) {
        style = MainWindow::DirectionsStyle::Inverted;
    } else if(ui->style_tree->isChecked()) {
        style = MainWindow::DirectionsStyle::Tree;
    }

    emit directions_style_changed(style);

    QDialog::accept();
}


