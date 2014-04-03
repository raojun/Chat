#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QUdpSocket>
#include <QTextCharFormat>

class QUdpSocket;
class TcpServer;

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

    QString fileName;
    TcpServer *server;

    QColor color;

protected:
    void newParticipant(QString userName,QString localHostName,QString ipAddress);
    void participantLeft(QString userName,QString localHostName,QString time);
    void sendMessage(MessageType type,QString serverAddress="");

    bool saveFile(const QString &fileName);//保存聊天记录
    void closeEvent(QCloseEvent *);//关闭事件

    //用于在收到文件名UDP信息时判断是否接收该文件
    void hasPendingFlie(QString userName,QString serverAddress
                        ,QString clientAddress,QString fileName);

    QString getIP();
    QString getUserName();
    QString getMessage();

private slots:
    void processPendingDatagrams();
    void on_sendButton_clicked();
    void on_sendToolBtn_clicked();
    void getFileName(QString);//用于获取服务器类sendFileName()信号发送过来的文件名
    void on_fontComboBox_currentFontChanged(QFont f);
    void on_sizeComboBox_currentIndexChanged(QString );
    void on_boldToolBtn_clicked(bool checked);
    void on_italicToolBtn_clicked(bool checked);
    void on_underlineToolBtn_clicked(bool checked);
    void on_colorToolBtn_clicked();

    void currentFormatChanged(const QTextCharFormat &format);
    void on_saveToolBtn_clicked();
    void on_clearToolBtn_clicked();
    void on_exitButton_clicked();
};

#endif // WIDGET_H
