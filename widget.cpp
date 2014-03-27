#include "widget.h"
#include "ui_widget.h"
#include <QudpSocket>
#include <QHostInfo>
#include <QMessageBox>
#include <QScrollBar>
#include <QDateTime>
#include <QNetworkInterface>
#include <QProcess>
#include <QPushButton>

Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget)
{
    ui->setupUi(this);

    udpSocket=new QUdpSocket(this);//创建UDP套接字并初始化
    port=45454;
    udpSocket->bind(port,QUdpSocket::ShareAddress|QUdpSocket::ReuseAddressHint);
    //当有数据报告来时，就会触发槽
    connect(udpSocket,SIGNAL(readyRead()),this,SLOT(processPendingDatagrams()));

    //退出
    connect(ui->exitButton,SIGNAL(clicked()),this,SLOT(close()));
    sendMessage(NewParticipant);//调用sendMessage()函数来广播用户登录信息
}

Widget::~Widget()
{
    delete ui;
}

void Widget::sendMessage(MessageType type,QString serverAddress)
{
    QByteArray data;
    QDataStream out(&data,QIODevice::WriteOnly);
    QString localHostName=QHostInfo::localHostName();
    QString address=getIP();
    out<<type<<getUserName()<<localHostName;

    switch(type)
    {
        case Message:
            if (ui->messageTextEdit->toPlainText()=="")//判断消息是否为空
            {
                QMessageBox::warning(0,tr("警告"),tr("发送内容不能为空"),QMessageBox::Ok);
                return;
            }
            out<<address<<getMessage();//想发送的数据中写入本机IP和输入的消息文本
            ui->messageBrowser->verticalScrollBar()->setValue(ui->messageBrowser->verticalScrollBar()->maximum());
            break;
        case NewParticipant:
            out<<address;//新用户加入，只需向数据中写入IP地址
            break;
        case ParticipantLeft:
            break;//离开用户不做任何处理
        case FileName:
            break;
        case Refuse:
            break;
    }

    //使用writeDatagram()函数进行UDP广播
    udpSocket->writeDatagram(data,data.length(),QHostAddress::Broadcast,port);
}

//processPendingDatagrams()槽的实现
void Widget::processPendingDatagrams()
{
    while(udpSocket->hasPendingDatagrams())
    {
        QByteArray datagram;
        datagram.resize(udpSocket->pendingDatagramSize());
        udpSocket->readDatagram(datagram.data(),datagram.size());
        QDataStream in(&datagram,QIODevice::ReadOnly);
        int messageType;
        in>>messageType;//获取消息类型
        QString userName,localHostName,ipAddress,message;
        QString time=QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
        switch(messageType)
        {
            case Message://普通信息，就获取用户名，主机名，IP和消息数据
                in>>userName>>localHostName>>ipAddress>>message;
                ui->messageBrowser->setTextColor(Qt::blue);
                ui->messageBrowser->setCurrentFont(QFont("Times New Roman",12));
                ui->messageBrowser->append("["+userName+"]"+time);
                ui->messageBrowser->append(message);
                break;
            case NewParticipant:
                in>>userName>>localHostName>>ipAddress;
                newParticipant(userName,localHostName,ipAddress);//进行新用户登录的处理
                break;
            case ParticipantLeft:
                in>>userName>>localHostName;
                participantLeft(userName,localHostName,time);//进行用户离开的处理
                break;
            case FileName:
                break;
            case Refuse:
                break;
        }
    }
}

//新用户的加入
void Widget::newParticipant(QString userName,QString localHostName,QString ipAddress)
{
    //使用主机名来判断该用户是否已经加入到用户列表中
    bool isEmpty=ui->userTableWidget
                    ->findItems(localHostName,Qt::MatchExactly).isEmpty();
    if(isEmpty)
    {
        QTableWidgetItem *user=new QTableWidgetItem(userName);
        QTableWidgetItem *host=new QTableWidgetItem(localHostName);
        QTableWidgetItem *ip=new QTableWidgetItem(ipAddress);

        //如果没有加入，则向界面右侧的用户列表中添加新用户的信息
        ui->userTableWidget->insertRow(0);
        ui->userTableWidget->setItem(0,0,user);
        ui->userTableWidget->setItem(0,1,host);
        ui->userTableWidget->setItem(0,2,ip);

        //在信息游览器中显示用户的加入
        ui->messageBrowser->setTextColor(Qt::gray);
        ui->messageBrowser->setCurrentFont(QFont("Times New Roman",10));
        ui->messageBrowser->append(tr("%1在线上！").arg(userName));
        ui->userNumLabel->setText(tr("在线人数：%1").arg(ui->userTableWidget->rowCount()));

        //再次调用sendMessage()来发送新用户登录信息
        //这是因为已经在线的各个端点也要告知刚加入的端点他们自己的用户信息
        //不这样做，在新加入的端点的用户列表中就无法显示其他已经在线的端点的信息
        sendMessage(NewParticipant);
    }
}

//用户离开participantLeft()函数
void Widget::participantLeft(QString userName,QString localHostName,QString time)
{
    int rowNum=ui->userTableWidget->findItems(localHostName,Qt::MatchExactly).first()->row();
    ui->userTableWidget->removeRow(rowNum);
    ui->messageBrowser->setTextColor(Qt::gray);
    ui->messageBrowser->setCurrentFont(QFont("Times New Roman",10));
    ui->messageBrowser->append(tr("%1于%2离开！").arg(userName).arg(time));
    ui->userNumLabel->setText(tr("在线人数：%1").arg(ui->userTableWidget->rowCount()));
}

//获取IP地址
QString Widget::getIP()
{
    QList<QHostAddress> list=QNetworkInterface::allAddresses();
    foreach(QHostAddress address,list)
    {
        if(address.protocol()==QAbstractSocket::IPv4Protocol)
        {
            return address.toString();
        }
    }
    return 0;
}

//获取用户名
QString Widget::getUserName()
{
    QStringList envVariables;
    envVariables<<"USERNAME.*"<<"USER.*"<<"USERDOMAIN.*"
                <<"HOSTNAME.*"<<"DOMAINNAME.*";
    QStringList environment=QProcess::systemEnvironment();
    foreach(QString string,envVariables)
    {
        int index=environment.indexOf(QRegExp(string));
        if(index!=-1)
        {
            QStringList stringList=environment.at(index).split('=');
            if(stringList.size()==2)
            {
                return stringList.at(1);
                break;
            }
        }
    }
    return "unknown";
}

//获取用户输入的聊天信息
QString Widget::getMessage()
{
    QString msg=ui->messageTextEdit->toHtml();//从消息文本编辑器中获取用户输入的聊天信息
    ui->messageTextEdit->clear();//将文本编辑器中的内容清空
    ui->messageTextEdit->setFocus();
    return msg;
}

//发送


void Widget::on_sendButton_clicked()
{
    sendMessage(Message);
}
