#ifndef CHATWIDGET_H
#define CHATWIDGET_H

#include <QWidget>

class QPlainTextEdit;
class QListWidget;
class QLineEdit;

class ChatWidget: public QWidget
{
    Q_OBJECT

    QListWidget *mpUsersList;
    QPlainTextEdit *mpFeed;
    QLineEdit *mpEnterLine;
public:
    ChatWidget(QWidget *pParent = nullptr);
};

#endif // CHATWIDGET_H
