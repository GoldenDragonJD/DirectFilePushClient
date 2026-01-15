#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpSocket>
#include "filemessage.h"
#include <QFile>
#include <QElapsedTimer>
#include <QTimer>
#include "notificationmanager.h"
#include <QQueue>

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
    QString selectedFolderPath;
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
    bool sendingFolder = false;
    QMetaObject::Connection uploadConn;

    bool hardSend = false;

    qint64 CHUNK_SIZE = 1024*256;

    QString document_folder;
    QString cache_file_path;
    QString transfer_file_path;
    QString root_folder_name;

    QQueue<QString> fileQueue;

    NotificationManager *notificationManager = nullptr;

    void sendFile();
    void sendMessage(const QString messageToSend);
    void sendFileChunk();
    void scrollToBottom();
    void UpdateLastIp();
    void sendDirectories();

    quint16 filesToRecieve = 0;

    int lastProgress = -1;


private slots:
    void on_fileDialogButton_clicked();
    void on_folderDialogButton_clicked();
    void on_connectButton_clicked();
    void on_RemoveFileButton_clicked();
    void on_pairButton_clicked();
    void on_sendButton_clicked();
    void on_AddIpButton_clicked();

protected:
    void closeEvent(QCloseEvent *event) override;

};
#endif // MAINWINDOW_H
