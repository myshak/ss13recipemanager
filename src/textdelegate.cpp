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
    //FIXME: doesn't work properly under qt5
    QStyleOptionViewItemV4 option = options;
    initStyleOption(&option, index);

    painter->save();
    QTextDocument doc;
    doc.setHtml( index.data().toString() );
    QAbstractTextDocumentLayout::PaintContext context;
    doc.setPageSize( option.rect.size());
    painter->translate(option.rect.x(), option.rect.y());

    if(option.state & QStyle::State_Selected) {
        context.palette.setColor(QPalette::Text, option.palette.highlightedText().color());
        context.palette.setColor(QPalette::Link, option.palette.highlightedText().color());
        context.palette.setColor(QPalette::LinkVisited, option.palette.highlightedText().color());
    }

    doc.documentLayout()->draw(painter, context);
    painter->restore();
}

bool TextDelegate::editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index)
{
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
