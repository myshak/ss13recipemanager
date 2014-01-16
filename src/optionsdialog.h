#ifndef OPTIONS_H
#define OPTIONS_H

#include <QDialog>

namespace Ui {
class Options;
}

class OptionsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit OptionsDialog(int indentation, bool inverted, QWidget *parent = 0);
    ~OptionsDialog();

signals:
    void indentation_changed(int level);
    void inverted_changed(bool inv);

protected:
    void accept();

private:
    Ui::Options *ui;
};

#endif // OPTIONS_H
