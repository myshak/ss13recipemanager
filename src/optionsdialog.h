#ifndef OPTIONS_H
#define OPTIONS_H

#include <QDialog>
#include "mainwindow.h"

namespace Ui {
class Options;
}

class OptionsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit OptionsDialog(int indentation, MainWindow::DirectionsStyle style, bool tree_animated, QWidget *parent = 0);
    ~OptionsDialog();

signals:
    void indentation_changed(int level);
    void directions_style_changed(MainWindow::DirectionsStyle style);
    void tree_animated_changed(bool animated);

protected:
    void accept();

private:
    Ui::Options *ui;
};

#endif // OPTIONS_H
