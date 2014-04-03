#include "widget.h"
#include "ui_widget.h"
#include "tcpserver.h"
#include "tcpclient.h"
#include <QFileDialog>
#include <QudpSocket>
#include <QHostInfo>
#include <QMessageBox>
#include <QScrollBar>
#include <QDateTime>
#include <QNetworkInterface>
#include <QProcess>
#include <QPushButton>
#include <QColorDialog>

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

    server=new TcpServer(this);//创建服务器类对象
    connect(server,SIGNAL(sendFileName(QString)),this,SLOT(getFileName(QString)));

    //关联信号和槽
    connect(ui->messageTextEdit,SIGNAL(currentCharFormatChanged(QTextCharFormat)),
            this,SLOT(currentFormatChanged(const QTextCharFormat)));
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
    {
            int row=ui->userTableWidget->currentRow();
            QString clientAddress=ui->userTableWidget->item(row,2)->text();//获取当前选中用户的IP
            out<<address<<clientAddress<<fileName;//将IP和文件名一起写入UDP数据报中
            break;
    }
        case Refuse:
            out<<serverAddress;
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
        {
                in>>userName>>localHostName>>ipAddress;
                QString clientAddress,fileName;
                in>>clientAddress>>fileName;
                hasPendingFlie(userName,ipAddress,clientAddress,fileName);//判断是否接收该文件
                break;
        }
            case Refuse:
                in>>userName>>localHostName;
                QString serverAddress;
                in>>serverAddress;
                QString ipAddress=getIP();
                //当发送过来拒绝信息时，要判断该程序是否是发送端
                if(ipAddress==serverAddress)
                {
                    server->refused();//若是，则执行服务器的refused()函数
                }
                break;
        }
    }
}

//hasPendingFile()函数
void Widget::hasPendingFlie(QString userName,QString serverAddress,
                                QString clientAddress,QString fileName)
{
    QString ipAddress=getIP();
    if(ipAddress==clientAddress)
    {
        //弹出提示框让用户判断是否接收文件
        int btn=QMessageBox::information(this,tr("接受文件"),tr("来自%1(%2)的文件：%3,是否接收？")
                                                .arg(userName).arg(serverAddress).arg(fileName),
                                                QMessageBox::Yes,QMessageBox::No);
        if(btn==QMessageBox::Yes)
        {
            //若接收，则创建客户端对象来传输文件
            QString name=QFileDialog::getSaveFileName(0,tr("保存文件"),fileName);
            if(!name.isEmpty())
            {
                TcpClient *client=new TcpClient(this);
                client->setFileName(name);
                client->setHostAddress(QHostAddress(serverAddress));
                client->show();
            }
        }
        else
        {
            //若拒绝接收，就发送拒绝信息的UDP广播
            sendMessage(Refuse,serverAddress);
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
    //从消息文本编辑器中获取用户输入的聊天信息
    QString msg=ui->messageTextEdit->toHtml();
    ui->messageTextEdit->clear();//将文本编辑器中的内容清空
    ui->messageTextEdit->setFocus();
    return msg;
}

//getFileName()槽
void Widget::getFileName(QString name)
{
    fileName=name;
    sendMessage(FileName);
}

//currentFormatChanged()槽的定义
void Widget::currentFormatChanged(const QTextCharFormat &format)
{
    ui->fontComboBox->setCurrentFont(format.font());
    //如果字体大小出错，使用12号字体
    if(format.fontPointSize()<9)
    {
        ui->sizeComboBox->setCurrentIndex(3);
    }
    else
    {
        ui->sizeComboBox->setCurrentIndex(ui->sizeComboBox
                ->findText(QString::number(format.fontPointSize())));
    }
    ui->boldToolBtn->setChecked(format.font().bold());
    ui->italicToolBtn->setChecked(format.font().italic());
    ui->underlineToolBtn->setChecked(format.font().underline());
    color=format.foreground().color();
}

//发送
void Widget::on_sendButton_clicked()
{
    sendMessage(Message);
}


//传输文件
void Widget::on_sendToolBtn_clicked()
{
    if(ui->userTableWidget->selectedItems().isEmpty())
    {
        //现在用户列表中选择一个用户来接收文件
        QMessageBox::warning(0,tr("选择用户"),
                            tr("请先从用户列表选择要传送的用户"),QMessageBox::Ok);
        return;
    }

    //弹出发送端界面
    server->show();
    server->initServer();
}

//更改字体样式
void Widget::on_fontComboBox_currentFontChanged(QFont f)
{
    ui->messageTextEdit->setCurrentFont(f);
    ui->messageTextEdit->setFocus();
}

//更改字体大小
void Widget::on_sizeComboBox_currentIndexChanged(QString size)
{
    ui->messageTextEdit->setFontPointSize(size.toDouble());
    ui->messageTextEdit->setFocus();
}

//字体加粗
void Widget::on_boldToolBtn_clicked(bool checked)
{
    if(checked)
    {
        ui->messageTextEdit->setFontWeight(QFont::Bold);
    }
    else
    {
        ui->messageTextEdit->setFontWeight(QFont::Normal);
    }
    ui->messageTextEdit->setFocus();
}

//字体倾斜
void Widget::on_italicToolBtn_clicked(bool checked)
{
    ui->messageTextEdit->setFontItalic(checked);
    ui->messageTextEdit->setFocus();
}

//字体下划线
void Widget::on_underlineToolBtn_clicked(bool checked)
{
    ui->messageTextEdit->setFontUnderline(checked);
    ui->messageTextEdit->setFocus();
}

//更改字体颜色
void Widget::on_colorToolBtn_clicked()
{
    color = QColorDialog::getColor(color, this);
    if (color.isValid())
    {
        ui->messageTextEdit->setTextColor(color);
        ui->messageTextEdit->setFocus();
    }
}

//保存聊天记录
void Widget::on_saveToolBtn_clicked()
{
    if(ui->messageBrowser->document()->isEmpty())
    {
        QMessageBox::warning(0,tr("警告"),tr("聊天记录为空，无法保存！"),QMessageBox::Ok);
    }
    else
    {
        QString fileName=QFileDialog::getSaveFileName(this,tr("保存聊天记录"),
                            tr("聊天记录"),tr("文本(*.txt);;All File(*.*"));
        if(!fileName.isEmpty())
        {
            saveFile(fileName);
        }
    }
}

//saveFile()函数的定义
bool Widget::saveFile(const QString &fileName)
{
    QFile file(fileName);
    if(!file.open(QFile::WriteOnly|QFile::Text))
    {
        QMessageBox::warning(this,tr("保存文件"),tr("无法保存文件 %1:\n%2")
                                    .arg(fileName)
                                    .arg(file.errorString()));
        return false;
    }
    QTextStream out(&file);
    out<<ui->messageBrowser->toPlainText();
    return true;
}

//删除聊天记录
void Widget::on_clearToolBtn_clicked()
{
    ui->messageBrowser->clear();
}

// 退出
void Widget::on_exitButton_clicked()
{
    close();
}

void Widget::closeEvent(QCloseEvent *e)
{
    sendMessage(ParticipantLeft);
    QWidget::closeEvent(e);
}