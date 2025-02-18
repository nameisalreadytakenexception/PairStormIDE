#ifndef ADDCOMMENTTEXTEDIT_H
#define ADDCOMMENTTEXTEDIT_H

#include <QFont>
#include <QString>
#include<QPushButton>
#include<QTextCursor>
#include<QRegularExpression>
#include<QTextDocument>
#include<QPlainTextEdit>

namespace Ui
{
class AddCommentTextEdit;
}

class AddCommentTextEdit : public QWidget
{
    Q_OBJECT

public:
    explicit AddCommentTextEdit(QWidget *parent = nullptr);
    ~AddCommentTextEdit();

    QString getText();
    void setText(const QString &text);
    QPushButton* getSendButton();
    QPushButton* getSetBoldButton();
    QPushButton* getSetItalicButton();
    QTextCursor getCursor();
    QPlainTextEdit* getTextEdit();
    void setCursor(const QTextCursor &cursor);

public
slots:
    void setBoldPressed();
    void setItalicPressed();
    void setSpecialSelect(QString sighns);
    void sendComment();
    void deleteComment();

signals:
    void setBySpecialSigns(QString sights);
    void emptyCommentWasSent();
    void notEmptyCommentWasSent();
    void commentWasDeleted();

private:
    Ui::AddCommentTextEdit *ui;
};

#endif // ADDCOMMENTTEXTEDIT_H
