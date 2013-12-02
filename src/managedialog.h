#ifndef MANAGEDIALOG_H
#define MANAGEDIALOG_H

#include <QDialog>
#include <QList>
#include "recipelist.h"

namespace Ui {
class ManageDialog;
}

class ManageDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ManageDialog(QWidget *parent = 0);
    ~ManageDialog();

signals:
    void reload_recipelists(QStringList rl);

protected:
    void accept();

private slots:
    void on_remove_button_clicked();

private:
    Ui::ManageDialog *ui;

    QList<RecipeList> recipelists;

    void reload_recipelists_table();
};

#endif // MANAGEDIALOG_H
