#include "textdelegate.h"

#include <QPainter>
#include <QTextDocument>
#include <QAbstractTextDocumentLayout>
#include <QTextBlock>
#include <QApplication>

#include <QDebug>

#include "reagent.h"

TextDelegate::TextDelegate(QObject *parent) :
    QStyledItemDelegate(parent)
{
}

void TextDelegate::paint( QPainter * painter, const QStyleOptionViewItem & options, const QModelIndex & index ) const
{
    QStyleOptionViewItemV4 option = options;
    initStyleOption(&option, index);

    painter->save();

    QAbstractTextDocumentLayout::PaintContext context;

    painter->translate(option.rect.x(), option.rect.y());

    context.palette = option.palette;
    context.clip = QRect{0, 0, option.rect.width(), option.rect.height()};

    QTextDocument doc;
    doc.setPageSize( option.rect.size());
    doc.setTextWidth(option.rect.width());

    if(option.state & QStyle::State_Selected) {
        context.palette.setColor(QPalette::Text, option.palette.color(QPalette::Active, QPalette::HighlightedText));
        doc.setDefaultStyleSheet(QString("a {color: %0; }").arg(option.palette.color(QPalette::Active, QPalette::HighlightedText).name()));
    }

    doc.setHtml( index.data().toString() );

    doc.documentLayout()->draw(painter, context);
    painter->restore();
}

bool TextDelegate::editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index)
{
    (void)model;

    if(event->type() == QEvent::MouseButtonDblClick) {

        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);

        QTextDocument doc;
        doc.setHtml( index.data().toString() );

        QString anchor = doc.documentLayout()->anchorAt(mouseEvent->pos() - option.rect.topLeft());

        if(!anchor.isEmpty()) {
            emit anchor_clicked(anchor, index);
            return true;
        } else {
            /*
             // The whole text is considered one block, so we can't list anchors this way
             for (QTextBlock it = doc.begin(); it != doc.end(); it = it.next()) {
                 if(it.charFormat().isAnchor()) {
                     emit anchor_clicked(it.charFormat().anchorHref(), index);
                     return true;
                 }
             }*/

            Reagent::ReagentStep reagent = index.data(Qt::UserRole+1).value<Reagent::ReagentStep>();

            emit anchor_clicked(reagent.ingredients[0].name, index);
            return true;
        }
    }

    return false;
}
