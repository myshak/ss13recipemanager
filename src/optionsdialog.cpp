#include "optionsdialog.h"
#include "ui_optionsdialog.h"

OptionsDialog::OptionsDialog(int indentation, bool inverted, QWidget *parent):
    QDialog(parent),
    ui(new Ui::Options)
{
    ui->setupUi(this);

    ui->indentation_level->setValue(indentation);
    ui->inverted->setChecked(inverted);
}

OptionsDialog::~OptionsDialog()
{
    delete ui;
}

void OptionsDialog::accept()
{
    emit indentation_changed(ui->indentation_level->value());
    emit inverted_changed(ui->inverted->isChecked());

    QDialog::accept();
}


