#include "tcpclient.h"
#include "ui_tcpclient.h"
#include <QTcpSocket>
#include <QDebug>
#include <QMessageBox>

TcpClient::TcpClient(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::TcpClient)
{
    ui->setupUi(this);

    setFixedSize(350,180);

    TotalBytes=0;
    bytesReceived=0;
    fileNameSize=0;
    tcpClient=new QTcpSocket(this);

    connect(tcpClient,SIGNAL(readyRead()),this,SLOT(readMessage()));
    connect(tcpClient,SIGNAL(error(QAbstractSocket::SocketError)),
    		this,SLOT(displayError(QAbstractSocket::SocketError)));
    connect(ui->tcpClientCancleBtn,SIGNAL(clicked()),this,SLOT(cancleBtn()));
    connect(ui->tcpClientCloseBtn,SIGNAL(clicked()),this,SLOT(closeBtn()));
}

TcpClient::~TcpClient()
{
    delete ui;
}

//要在主界面中弹出文件对话框来选择发送过来文件的保存路径，
//所以要在客户端类中提供函数来获取该路径
void TcpClient::setFileName(QString fileName)
{
	localFile=new QFile(fileName);
}

//从主界面获取IP地址
void TcpClient::setHostAddress(QHostAddress address)
{
	hostAddress=address;
	newConnect();
}

//设置与服务器的连接
void TcpClient::newConnect()
{
	blockSize=0;
	tcpClient->abort();
	tcpClient->connectToHost(hostAddress,tcpPort);
	time.start();
}

//读取数据
void TcpClient::readMessage()
{
	QDataStream in(tcpClient);
	in.setVersion(QDataStream::Qt_4_7);

	float useTime=time.elapsed();
	if(bytesReceived<=sizeof(qint64)*2)
	{
        if((tcpClient->bytesAvailable()>=sizeof(qint64)*2)&&(fileNameSize==0))
		{
            in>>TotalBytes>>fileNameSize;
			bytesReceived+=sizeof(qint64)*2;
		}
        if((tcpClient->bytesAvailable()>=fileNameSize)&&(fileNameSize!=0))
		{
			in>>fileName;
			bytesReceived+=fileNameSize;
			if(!localFile->open(QFile::WriteOnly))
			{
				QMessageBox::warning(this,tr("应用程序"),tr("无法读取文件%1:\n%2.")
										.arg(fileName)
										.arg(localFile->errorString()));
				return;
			}	
		}
		else
		{
			return;
		}
	}	
	if(bytesReceived<TotalBytes)
	{
		bytesReceived+=tcpClient->bytesAvailable();
		inBlock=tcpClient->readAll();
		localFile->write(inBlock);
		inBlock.resize(0);
	}

	ui->progressBar->setMaximum(TotalBytes);
	ui->progressBar->setValue(bytesReceived);

	double speed=bytesReceived/useTime;
	ui->tcpClientStatusLabel->setText(tr("已接收 %1MB (%2MB/s) "
                                       	"\n共%3MB 已用时:%4秒\n估计剩余时间：%5秒")
                                    	.arg(bytesReceived / (1024*1024))
                                      	.arg(speed*1000/(1024*1024),0,'f',2)
                                     	.arg(TotalBytes / (1024 * 1024))
                                      	.arg(useTime/1000,0,'f',0)
                                      	.arg(TotalBytes/speed/1000 - useTime/1000,0,'f',0));

	if(bytesReceived==TotalBytes)
	{
		localFile->close();
		tcpClient->close();
		ui->tcpClientStatusLabel->setText(tr("接收文件%1完毕").arg(fileName));
	}
}

//displayError()槽,错误处理
void TcpClient::displayError(QAbstractSocket::SocketError socketError)
{
	switch(socketError)
	{
        case QAbstractSocket::RemoteHostClosedError:break;
		defaule:qDebug()<<tcpClient->errorString();
	}
}

//取消按钮
void TcpClient::cancleBtn()
{
	tcpClient->abort();
	if(localFile->isOpen())
	{
		localFile->close();
	}
}

//关闭按钮
void TcpClient::closeBtn()
{
	tcpClient->abort();
	if(localFile->isOpen())
	{
		localFile->close();
	}
	close();
}

//关闭事件处理函数
void TcpClient::closeEvent(QCloseEvent *)
{
	connect(ui->tcpClientCloseBtn,SIGNAL(clicked()),this,SLOT(closeBtn()));
}
