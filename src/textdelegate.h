#ifndef TEXTDELEGATE_H
#define TEXTDELEGATE_H

#include <QStyledItemDelegate>

class TextDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit TextDelegate(QObject *parent = 0);
    ~TextDelegate() = default;

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;

signals:
    void anchor_clicked(QString, QModelIndex);

protected:
    bool editorEvent (QEvent * event, QAbstractItemModel * model, const QStyleOptionViewItem & option, const QModelIndex & index );
};

#endif // TEXTDELEGATE_H
