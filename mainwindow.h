#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpSocket>
#include "filemessage.h"
#include <QFile>
#include <QElapsedTimer>
#include <QTimer>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    QString selectedFilePath;
    QTcpSocket *socket;
    int mode = 0;
    int pairPartnerId = -1;
    int myId = -1;

    long long current_total_file_size;
    long long current_file_size;
    QString current_file_name;

    fileMessage *currentFileMessage = nullptr;
    QFile* current_file = nullptr;

    qint64 bytesReceivedThisSecond = 0;
    QElapsedTimer speedTimer;

    QTimer *updateTimer = nullptr;

    int flushIndex = 0;

    bool sendingFile = false;
    QMetaObject::Connection uploadConn;

    bool hardSend = false;

private slots:
    void on_fileDialogButton_clicked();
    void on_connectButton_clicked();
    void on_RemoveFileButton_clicked();
    void on_pairButton_clicked();
    void on_sendButton_clicked();
    void sendFile();
    void sendMessage(const QString messageToSend);
    void sendFileChunk();
    void scrollToBottom();

};
#endif // MAINWINDOW_H
