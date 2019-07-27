#include "codeeditor.h"
#include<QDebug>
void CodeEditor::keyPressEvent(QKeyEvent *e)
{
        if((e->key() == Qt::Key_Enter || e->key() == Qt::Key_Return) &&// shift + enter
                ((e->modifiers() & Qt::ShiftModifier) == Qt::ShiftModifier))
        {
            e = new QKeyEvent(e->type(), e->key(), e->modifiers()&Qt::MetaModifier &Qt::KeypadModifier);
        }

        if(e->key() == Qt::Key_BraceLeft)
        {
            QPlainTextEdit::keyPressEvent(e);
            this->insertPlainText("}");
            this->moveCursor(QTextCursor::Left);
            this->verticalScrollBar();
            return;
        }

        if(e->key() == Qt::Key_BracketLeft)
        {
           QPlainTextEdit::keyPressEvent(e);
           this->insertPlainText("]");
           this->moveCursor(QTextCursor::Left);
           this->verticalScrollBar();
           return;
        }

        if((e->key() == Qt::Key_Plus && e->modifiers() & Qt::ControlModifier)//ctrl & +
                && currentZoom <= 150)// forbid to zoom so much
        {
            this->zoomIn(1);//increase zoom
            currentZoom += 1;
            setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);// reset text margin in accordance to linecouter change
            return;
        }

        if((e->key() == Qt::Key_Minus && e->modifiers() & Qt::ControlModifier)//ctrl & -
                && currentZoom >= 50)//forbid to zoom so much
        {
            this->zoomOut(1);//decrease zoom
            currentZoom -= 1;
            setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);// reset text margin in accordance to linecouter change
            return;
        }

        if(e->key() == Qt::Key_Z && e->modifiers() & Qt::ControlModifier)
        {
            QString before = QString::fromStdString(changesManager.changesHistory.back().before);
            QString after = QString::fromStdString(changesManager.changesHistory.back().after);

            qDebug()<< "begin = "<< changesManager.changesHistory.back().begin_change_pos;
            qDebug()<< "before = "<< before;
            qDebug()<< "after = "<< after;
            QString text = QString::fromStdString(this->changesManager.returnToPreviousState());
            this->document()->setPlainText(text);

        }
        QPlainTextEdit::keyPressEvent(e);
}
