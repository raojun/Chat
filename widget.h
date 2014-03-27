#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QUdpSocket>

namespace Ui {
class Widget;
}

//定义枚举类型，用来区分不同的广播类型
enum MessageType {
	Message,//消息
	NewParticipant,//新用户加入
	ParticipantLeft,//用户退出
	FileName,//文件名
	Refuse //拒绝接收文件
};

class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(QWidget *parent = 0);
    ~Widget();

private:
    Ui::Widget *ui;
    QUdpSocket * udpSocket;
    qint16 port;

protected:
	void newParticipant(QString userName,QString localHostName,QString ipAddress);
	void participantLeft(QString userName,QString localHostName,QString time);
    void sendMessage(MessageType type,QString serverAddress="");

	QString getIP();
	QString getUserName();
	QString getMessage();

private slots:
	void processPendingDatagrams();
    void on_sendButton_clicked();
};

#endif // WIDGET_H
