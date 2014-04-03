#include "tcpserver.h"
#include "ui_tcpserver.h"
#include <QTcpServer>
#include <QTcpSocket>
#include <QMessageBox>
#include <QFileDialog>
#include <QDebug>
#include <QFile>

TcpServer::TcpServer(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::TcpServer)
{
    ui->setupUi(this);

    setFixedSize(350,180);//固定对话框大小

    //创建QTcpServer对象进行信号和槽的关联
    tcpPort=6666;
    tcpServer=new QTcpServer(this);
    connect(tcpServer,SIGNAL(newConnection()),this,SLOT(sendMessage()));

    initServer();
}

TcpServer::~TcpServer()
{
    delete ui;
}

void TcpServer::initServer()
{
	//初始化变量
	payloadSize=64*1024;
	TotalBytes=0;
	bytesWritten=0;
	bytesToWrite=0;

	//设置按钮状态
	ui->serverStatusLabel->setText(tr("选择要传送的文件："));
	ui->progressBar->reset();
	ui->serverOpenBtn->setEnabled(true);
	ui->serverSendBtn->setEnabled(false);

    tcpServer->close();//关闭服务器
}

//sendMessage()槽
void TcpServer::sendMessage()
{
	ui->serverSendBtn->setEnabled(false);
	clientConnection=tcpServer->nextPendingConnection();
	connect(clientConnection,SIGNAL(bytesWritten(qint64)),this,SLOT(updateClientProgress(qint64)));

    ui->serverStatusLabel->setText(tr("开始传送文件 %1！").arg(thefileName));

	localFile=new QFile(fileName);
	if(!localFile->open((QFile::ReadOnly)))
	{
		QMessageBox::warning(this,tr("应用程序"),tr("无法读取文件%1：\n%2")
							.arg(fileName).arg(localFile->errorString()));
		return;
	}
	TotalBytes=localFile->size();
	QDataStream sendOut(&outBlock,QIODevice::WriteOnly);
	sendOut.setVersion(QDataStream::Qt_4_7);
	time.start();//启动计时
	QString currentFile=fileName.right(fileName.size()-fileName.lastIndexOf('/')-1);
	sendOut<<qint64(0)<<qint64(0)<<currentFile;
    TotalBytes+=outBlock.size();
	sendOut.device()->seek(0);
	sendOut<<TotalBytes<<qint64((outBlock.size()-sizeof(qint64)*2));
    bytesToWrite=TotalBytes-clientConnection->write(outBlock);
	outBlock.resize(0);
}

//updateClientProgress()槽
void TcpServer::updateClientProgress(qint64 numBytes)
{
	qApp->processEvents();//该函数用于传送大文件是使界面不会冻结
	bytesWritten+=(int)numBytes;
	if(bytesToWrite>0)
	{
		outBlock=localFile->read(qMin(bytesToWrite,payloadSize));
		bytesToWrite-=(int)clientConnection->write(outBlock);
		outBlock.resize(0);
	}
	else
	{
        localFile->close();
	}

	ui->progressBar->setMaximum(TotalBytes);
	ui->progressBar->setValue(bytesWritten);

	float useTime=time.elapsed();//time.elapsed()函数用来获取耗费的时间
	double speed=bytesWritten/useTime;
	ui->serverStatusLabel->setText(tr("已发送%1MB(%2MB/s)"
									"\n共%3MB已用时：%4秒\n估计剩余时间：%5秒")
									.arg(bytesWritten/(1024*1024))
                                    .arg(speed*1000/(1024*1024),0,'f',2)
									.arg(TotalBytes/(1024*1024))
                                    .arg(useTime/1000,0,'f',0)
                                    .arg(TotalBytes/speed/1000-useTime/1000,0,'f',0));
	if (bytesWritten==TotalBytes)
	{
		localFile->close();
		tcpServer->close();
		ui->serverStatusLabel->setText(tr("文件传送成功！！"));
	}
}

//打开
void TcpServer::on_serverOpenBtn_clicked()
{
	fileName=QFileDialog::getOpenFileName(this);
	if (!fileName.isEmpty())
	{
        thefileName=fileName.right(fileName.size()-fileName.lastIndexOf('/')-1);
		ui->serverStatusLabel->setText(tr("要传送的文件为：%1").arg(fileName));
		
		//选择了要发送的文件后更新按钮状态
		ui->serverSendBtn->setEnabled(true);
        ui->serverOpenBtn->setEnabled(false);
	}
}


//发送
void TcpServer::on_serverSendBtn_clicked()
{
	if(!tcpServer->listen(QHostAddress::Any,tcpPort))//开始监听
	{
        qDebug()<<tcpServer->errorString();
		close();
		return;
	}
	ui->serverStatusLabel->setText(tr("请等待对方接收…………"));
    emit sendFileName(thefileName);//发送sendFileName()信号
}


//关闭
void TcpServer::on_serverCloseBtn_clicked()
{
	if(tcpServer->isListening())
	{
		tcpServer->close();
		if(localFile->isOpen())
		{
			localFile->close();
		}
        clientConnection->abort();
	}
	close();
}


//拒绝接收文件
//这个函数在主界面Widget类中当接收到接收端的拒绝接收文件的UDP信息是被调用
void TcpServer::refused()
{
	tcpServer->close();
	ui->serverStatusLabel->setText(tr("对方拒绝接收文件！！！"));
}

//关闭事件的处理函数
void TcpServer::closeEvent(QCloseEvent *)
{
	on_serverCloseBtn_clicked();//调用关闭按钮单击信号槽
}
