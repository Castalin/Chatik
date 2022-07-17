#include "chatclient.h"
#include <QDataStream>
#include <QJsonObject>
#include <QJsonDocument>

ChatClient::ChatClient(QObject *parent)
    : QObject{parent}, m_clientSocket(new QTcpSocket(this)), m_loggedIn(false)
{
    connect(m_clientSocket, &QTcpSocket :: connected, this, &ChatClient :: connected);
    connect(m_clientSocket, &QTcpSocket :: disconnected, this, &ChatClient :: disconnected);

    connect(m_clientSocket, &QTcpSocket :: readyRead, this, &ChatClient :: onReadyRead);

    connect(m_clientSocket, QOverload<QAbstractSocket :: SocketError> :: of(&QAbstractSocket :: error), this, &ChatClient :: error);

    connect(m_clientSocket, &QTcpSocket :: disconnected, this, [this]()-> void {m_loggedIn = false;});

}


void ChatClient :: login(const QString &userName)
{
    if (m_clientSocket->state() == QAbstractSocket :: ConnectedState)
    {
        QDataStream clientStream(m_clientSocket);

        QJsonObject message;
        message["type"] = QStringLiteral("login");
        message["username"] = userName;

        clientStream << QJsonDocument(message).toJson();
    }
}

void ChatClient :: sendMessage(const QString &text)
{
    if (text.isEmpty())
    {
        return;
    }
    else
    {
        QDataStream clientStream(m_clientSocket);
        QJsonObject message;

        message["type"] = QStringLiteral("message");
        message["text"] = text;

        clientStream << QJsonDocument(message).toJson(QJsonDocument :: Compact);
    }
}


void ChatClient :: disconnectFromHost()
{
    m_clientSocket->disconnectFromHost();
}


void ChatClient :: connectToServer(const QHostAddress &address, quint16 port)
{
    m_clientSocket->connectToHost(address, port);
}


void ChatClient :: jsonReceived(const QJsonObject &doc)
{
    const QJsonValue typeVal = doc.value(QLatin1String("type"));

    if (typeVal.isNull() || typeVal.isString())
    {
        return;
    }

    if (typeVal.toString().compare(QLatin1String("login"), Qt :: CaseInsensitive) == 0)
    {
        if (m_loggedIn == true)
        {
            return;
        }

        const QJsonValue resultVal = doc.value(QLatin1String("success"));

        if (resultVal.isNull() || resultVal.isBool() == false)
        {
            return;
        }

        const bool loginSuccess = resultVal.toBool();

        if (loginSuccess == true)
        {
            emit m_loggedIn;
            return;
        }

        const QJsonValue reasonVal = doc.value(QLatin1String("reason"));
        emit loginError(reasonVal.toString());
    }

    else if (typeVal.toString().compare(QLatin1String("message"), Qt :: CaseInsensitive) == 0)
    {
        const QJsonValue  textVal = doc.value(QLatin1String("text"));

        const QJsonValue senderVal = doc.value(QLatin1String("sender"));

        if (textVal.isNull() || textVal.isString() == 0)
        {
            return;
        }

        if (senderVal.isNull() || senderVal.isString() == false)
        {
            return;
        }

        emit messageReceived(senderVal.toString(), textVal.toString());
    }

    else if (typeVal.toString().compare(QLatin1String("newuser"), Qt :: CaseInsensitive) == 0)
    {
        const QJsonValue usernameVal = doc.value(QLatin1String("username"));

        if (usernameVal.isNull() || usernameVal.isString() == false)
        {
            return;
        }

        emit userJoined(usernameVal.toString());
    }

    else if (typeVal.toString().compare(QLatin1String("userdisconnected"), Qt :: CaseInsensitive) == 0)
    {
        const QJsonValue usernameVal = doc.value(QLatin1String("username"));

        if (usernameVal.isNull() || usernameVal.isString() == false)
        {
            return;
        }

        emit userLeft(usernameVal.toString());

    }


}


void ChatClient :: onReadyRead()
{
    QByteArray jsonData;


    QDataStream socketStream(m_clientSocket);


    while(true)
    {
        socketStream.startTransaction();

        socketStream >> jsonData;

        if (socketStream.commitTransaction())
        {
            QJsonParseError parseError;

            const QJsonDocument jsonDoc = QJsonDocument :: fromJson(jsonData, &parseError);

            if (parseError.error == QJsonParseError :: NoError)
            {
                if (jsonDoc.isObject())
                {
                    jsonReceived(jsonDoc.object());
                }
            }

        }

        else
        {
            break;
        }
    }
}
