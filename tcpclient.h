#ifndef TCPCLIENT_H
#define TCPCLIENT_H

#include <QDialog>
#include <QHostAddress>
#include <QFile>
#include <QTime>

class QTcpSocket;

namespace Ui {
class TcpClient;
}

class TcpClient : public QDialog
{
    Q_OBJECT

public:
    explicit TcpClient(QWidget *parent = 0);
    ~TcpClient();

    void setHostAddress(QHostAddress address);
    void setFileName(QString fileName);

protected:
	void closeEvent(QCloseEvent *);

private:
    Ui::TcpClient *ui;

    QTcpSocket *tcpClient;
    quint16 blockSize;
    QHostAddress hostAddress;
    quint16 tcpPort;

    qint64 TotalBytes;
    qint64 bytesReceived;
    qint64 bytesToReceive;
    qint64 fileNameSize;
    QString fileName;
    QFile *localFile;
    QByteArray inBlock;

    QTime time;

private slots:
	void newConnect();
	void readMessage();
    void displayError(QAbstractSocket::SocketError);
	void closeBtn();
	void cancleBtn();

};

#endif // TCPCLIENT_H
