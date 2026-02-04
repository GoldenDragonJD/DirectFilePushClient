#include "encryptionwindow.h"
#include "ui_encryptionwindow.h"
#include "mainwindow.h"

encryptionwindow::encryptionwindow(QWidget *parent, bool recieving)
    : QDialog(parent)
    , ui(new Ui::encryptionwindow)
{
    ui->setupUi(this);
    ui->OkButton->setText(recieving? "Check": "Send");
    recievingEncryption = recieving;

    mainWindow = dynamic_cast<MainWindow*>(parent);
}

encryptionwindow::~encryptionwindow()
{
    delete ui;
}

void encryptionwindow::on_OkButton_clicked()
{
    if (ui->keyInput->text().length() == 0)
    {
        changeStatus("Key must have at least one character.");
        return;
    }

    if (!recievingEncryption)
    {
        challenge = mainWindow->sendEncryptionRequest();
        ui->OkButton->setText("Waiting");
        ui->OkButton->setEnabled(false);
        return;
    }

    mainWindow->sendEncryptionTest();
    ui->OkButton->setText("Waiting");
    ui->OkButton->setEnabled(false);
}

void encryptionwindow::on_Cancel_clicked()
{
    reject();
}

QString encryptionwindow::getEncrytionKey()
{
    return ui->keyInput->text();
}

QByteArray encryptionwindow::getChallenge()
{
    return challenge;
}

bool encryptionwindow::getRecievingEncryption()
{
    return recievingEncryption;
}

void encryptionwindow::changeStatus(QString status)
{
    ui->errorLable->setText(status);
}

void encryptionwindow::resetButtonUI()
{
    ui->OkButton->setText(recievingEncryption? "Check": "Send");
    ui->OkButton->setEnabled(true);
}
