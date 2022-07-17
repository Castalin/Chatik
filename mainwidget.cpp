#include "mainwidget.h"
#include "ui_mainwidget.h"
#include <QInputDialog>
#include <QHostAddress>
#include <QMessageBox>

MainWidget::MainWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::MainWidget), m_chatClient(new ChatClient(this)), m_chatModel(new QStandardItemModel(this))
{
    ui->setupUi(this);
    m_chatModel->insertColumn(0);

    ui->chatView->setModel(m_chatModel);

    connect(m_chatClient, &ChatClient :: connected, this, &MainWidget :: connectedToServer);
    connect(m_chatClient, &ChatClient :: disconnected, this, &MainWidget :: disconnectedFromServer);
    connect(m_chatClient, &ChatClient :: loggedIn, this, &MainWidget :: loggedIn);
    connect(m_chatClient, &ChatClient :: loginError, this, &MainWidget :: loginFailed);
    connect(m_chatClient, &ChatClient :: messageReceived, this, &MainWidget :: messageReceived);
    connect(m_chatClient, &ChatClient :: error, this, &MainWidget :: error);
    connect(m_chatClient, &ChatClient :: userJoined, this, &MainWidget :: userJoined);
    connect(m_chatClient, &ChatClient :: userLeft, this, &MainWidget :: userLeft);

    connect(ui->conn_btn, &QPushButton :: clicked, this, &MainWidget :: attemptConnection);
    connect(ui->send_btn, &QPushButton :: clicked, this, &MainWidget :: sendMessage);
    connect(ui->messageEdit, &QLineEdit :: returnPressed, this, &MainWidget :: sendMessage);

}

MainWidget::~MainWidget()
{
    delete ui;
}


void MainWidget :: attemptConnection()
{
    const QString hostAddress = QInputDialog :: getText(this, tr("Chose Server"), tr("Server Address"), QLineEdit :: Normal, QStringLiteral("127.0.0.1"));

    if (hostAddress.isEmpty())
    {
        return;
    }

    ui->conn_btn->setEnabled(false);

    const QString port = QInputDialog :: getText(this, tr("Chose Port"), tr("Number port"), QLineEdit :: Normal, QStringLiteral("1967"));
    m_chatClient->connectToServer(QHostAddress(hostAddress), port.toInt());
}


void MainWidget :: connectedToServer()
{
    const QString newUsername = QInputDialog :: getText(this, tr("Chose username"), tr("Username"));

    if (newUsername.isEmpty())
    {
        return m_chatClient->disconnectFromHost();
    }

    attemptLogin(newUsername);
}

void MainWidget :: attemptLogin(const QString &userName)
{
    m_chatClient->login(userName);
}

void MainWidget :: loggedIn()
{
    ui->messageEdit->setEnabled(true);
    ui->send_btn->setEnabled(true);
    ui->chatView->setEnabled(true);

    m_lastUserName.clear();
}

void MainWidget :: loginFailed(const QString &reason)
{
    QMessageBox :: critical(this, tr("Error"), reason);

    connectedToServer();
}


void MainWidget :: messageReceived(const QString &sender, const QString &text)
{
    int newRow = m_chatModel->rowCount();

    if (m_lastUserName != sender)
    {
        m_lastUserName = sender;
        QFont boldFont;
        boldFont.setBold(true);
        m_chatModel->insertRows(newRow, 2);
        m_chatModel->setData(m_chatModel->index(newRow, 0), sender + " ");
        m_chatModel->setData(m_chatModel->index(newRow, 0), int(Qt :: AlignLeft | Qt :: AlignVCenter), Qt :: TextAlignmentRole);
        m_chatModel->setData(m_chatModel->index(newRow, 0), boldFont, Qt :: FontRole);
        ++newRow;
    }
    else
    {
        m_chatModel->insertRow(newRow);
    }

    m_chatModel->setData(m_chatModel->index(newRow, 0), text);
    m_chatModel->setData(m_chatModel->index(newRow, 0), int(Qt :: AlignLeft | Qt :: AlignVCenter), Qt :: TextAlignmentRole);

    ui->chatView->scrollToBottom();
}


void MainWidget :: sendMessage()
{
    m_chatClient->sendMessage(ui->messageEdit->text());

    const int newRow = m_chatModel->rowCount();

    m_chatModel->insertRow(newRow);

    m_chatModel->setData(m_chatModel->index(newRow, 0), int(Qt :: AlignRight | Qt :: AlignVCenter), Qt :: TextAlignmentRole);

    ui->messageEdit->clear();
    ui->chatView->scrollToBottom();
    m_lastUserName.clear();


}


void MainWidget :: disconnectedFromServer()
{
    QMessageBox :: warning(this, tr("Disconnected"), tr("The host terminated the connection"));

    ui->messageEdit->setEnabled(false);
    ui->send_btn->setEnabled(false);
    ui->chatView->setEnabled(false);
    ui->conn_btn->setEnabled(true);
    m_lastUserName.clear();
}


void MainWidget :: userJoined(const QString &username)
{
    const int newRow = m_chatModel->rowCount();

    m_chatModel->insertRow(newRow);
    m_chatModel->setData(m_chatModel->index(newRow, 0), tr("%1 Joined the Chat").arg(username));
    m_chatModel->setData(m_chatModel->index(newRow, 0), Qt :: AlignCenter, Qt :: TextAlignmentRole);
    m_chatModel->setData(m_chatModel->index(newRow, 0), QBrush(Qt :: blue), Qt :: ForegroundRole);
    ui->chatView->scrollToBottom();
    m_lastUserName.clear();
}

void MainWidget :: userLeft(const QString &username)
{
    const int newRow = m_chatModel->rowCount();

    m_chatModel->insertRow(newRow);
    m_chatModel->setData(m_chatModel->index(newRow, 0), tr("%1 Left the Chat").arg(username));
    m_chatModel->setData(m_chatModel->index(newRow, 0), Qt :: AlignCenter, Qt :: TextAlignmentRole);
    m_chatModel->setData(m_chatModel->index(newRow, 0), QBrush(Qt :: red), Qt :: ForegroundRole);

    ui->chatView->scrollToBottom();
    m_lastUserName.clear();

}


void MainWidget :: error(QAbstractSocket :: SocketError  socketError)
{
    switch (socketError)  {
    case QAbstractSocket :: RemoteHostClosedError :
    case QAbstractSocket :: ProxyConnectionClosedError :
        return;
    case QAbstractSocket :: ConnectionRefusedError :
        QMessageBox :: critical(this, tr("Error"), tr("The host refused the connection"));
        break;
    case QAbstractSocket :: ProxyConnectionRefusedError :
        QMessageBox :: critical(this, tr("Error"), tr("The proxy refused the connection"));
        break;
    case QAbstractSocket :: ProxyNotFoundError :
        QMessageBox :: critical(this, tr("Error"), tr("Could not find the proxy"));
        break;
    case QAbstractSocket :: HostNotFoundError :
        QMessageBox :: critical(this, tr("Error"), tr("Could not fine the server"));
        break;
    case QAbstractSocket :: SocketAccessError :
        QMessageBox :: critical(this, tr("Error"), tr("You don't have permissions to execute this operation"));
        break;
    case QAbstractSocket :: SocketResourceError :
        QMessageBox :: critical(this, tr("Error"), tr("Too many connections openeed"));
        break;
    case QAbstractSocket :: SocketTimeoutError :
        QMessageBox :: critical(this, tr("Error"), tr("Operation timed out"));
        break;
    case QAbstractSocket :: ProxyConnectionTimeoutError :
        QMessageBox :: critical(this, tr("Error"), tr(" Proxy timed out"));
        break;
    case QAbstractSocket :: NetworkError :
        QMessageBox :: critical(this, tr("Error"), tr("Unable to reach the network"));
        break;
    case QAbstractSocket :: UnknownSocketError :
        QMessageBox :: critical(this, tr("Error"), tr("An unknown error occurred"));
        break;
    case QAbstractSocket :: UnsupportedSocketOperationError :
        QMessageBox :: critical(this, tr("Error"), tr("Operation not suppurted"));
        break;
    case QAbstractSocket :: ProxyAuthenticationRequiredError :
        QMessageBox :: critical(this, tr("Error"), tr("Your proxy reqauires authentication"));
        break;
    case QAbstractSocket :: ProxyProtocolError :
        QMessageBox :: critical(this, tr("Error"), tr("Proxy communication failed"));
        break;
    case QAbstractSocket :: TemporaryError :
    case QAbstractSocket :: OperationError :
        QMessageBox :: warning(this, tr("Error"), tr("Operation failed, please try again"));
        return;
    default:
        Q_UNREACHABLE();
    }

    ui->conn_btn->setEnabled(true);
    ui->send_btn->setEnabled(false);
    ui->messageEdit->setEnabled(false);
    ui->chatView->setEnabled(false);
    m_lastUserName.clear();
}
