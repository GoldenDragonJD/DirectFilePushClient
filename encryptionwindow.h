#ifndef ENCRYPTIONWINDOW_H
#define ENCRYPTIONWINDOW_H

#pragma once

#include <QDialog>

class MainWindow;

namespace Ui {
class encryptionwindow;
}

class encryptionwindow : public QDialog
{
    Q_OBJECT

public:
    explicit encryptionwindow(QWidget *parent = nullptr, bool = false);
    ~encryptionwindow();

    void printError(QString);
    QString getEncrytionKey();
    QByteArray getChallenge();
    bool getRecievingEncryption();
    uint fails = 0;

    void changeStatus(QString);
    void resetButtonUI();
    void sendAccept();
    void sendReject();
    QByteArray challenge;

private:
    Ui::encryptionwindow *ui;
    MainWindow *mainWindow;

    bool recievingEncryption;

private slots:
    void on_OkButton_clicked();
    void on_Cancel_clicked();
};

#endif // ENCRYPTIONWINDOW_H
