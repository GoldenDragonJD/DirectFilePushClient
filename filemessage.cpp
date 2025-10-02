#include "filemessage.h"
#include "ui_filemessage.h"

fileMessage::fileMessage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::fileMessage)
{
    ui->setupUi(this);
}

fileMessage::~fileMessage()
{
    delete ui;
}

void fileMessage::setFileName(const QString &fileName)
{
    ui->fileName->setText(fileName);
}

void fileMessage::setProgress(const int &progresse)
{
    ui->transferProgressBar->setValue(progresse);
}

void fileMessage::setToAndFromClient(const QString &fromClient, const QString &toClient)
{
    ui->fromLable->setText("Client " + fromClient + " >>>");
    ui->toLable->setText(">>> Client " + toClient);
}

void fileMessage::setStatus(const QString &status)
{
    ui->statusLable->setText(status);
}
