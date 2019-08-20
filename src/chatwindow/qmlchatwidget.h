#ifndef QMLCHATWIDGET_H
#define QMLCHATWIDGET_H

#include "chatwidgetinterface.h"
#include "onlineusersmodel.h"
#include <QQmlContext>

class QmlChatWidget : public ChatWidgetInterface
{
    Q_OBJECT

public:
    QmlChatWidget();
    QmlChatWidget(QmlChatWidget const&)             = delete;
    QmlChatWidget& operator=(QmlChatWidget const &) = delete;

    virtual void keyPressEvent(QKeyEvent * event) override;

public slots:

    virtual void configureOnLogin(const QString & userName) override;

    virtual void updateOnlineUsers(const QStringList & onlineUsers) override;
    virtual void updateConnectedUsers(const QStringList & connectedUsers) override;

    virtual void appendMessage(const QString & messageAuthor,
                               const QString & messageBody) override;

private:

    QString mUserName;

    OnlineUsersModel * mpUsers;
    QQmlContext      * mpChatContext;

private slots:
    void ConnectUserOnChangedState   (const QString userName);
    void DisconnectUserOnChangedState(const QString userName);

};

#endif // QMLCHATWIDGET_H
