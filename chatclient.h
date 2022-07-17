#ifndef CHATCLIENT_H
#define CHATCLIENT_H

#include <QObject>
#include <QAbstractSocket>
#include <QTcpSocket>

class ChatClient : public QObject
{
    Q_OBJECT
public:
    explicit ChatClient(QObject *parent = nullptr);

signals:
    void connected();
    void loggedIn();
    void loginError(const QString &reason);
    void disconnected();
    void messageReceived(const QString &sender, const QString &text);
    void error(QAbstractSocket :: SocketError socketError);
    void userJoined(const QString &username);
    void userLeft(const QString &username);

private:
    QTcpSocket *m_clientSocket;
    bool m_loggedIn;
    void jsonReceived(const QJsonObject &doc);

public slots:
    void connectToServer(const QHostAddress &address, quint16 port);
    void login(const QString &userName);
    void sendMessage(const QString &text);
    void disconnectFromHost();

private slots:
    void onReadyRead();
};


#endif // CHATCLIENT_H
