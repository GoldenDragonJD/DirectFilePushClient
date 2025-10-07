#include "addipwindow.h"
#include "ui_addipwindow.h"
#include <QFile>
#include "mainwindow.h"

addIpWindow::addIpWindow(MainWindow *parent)
    : QDialog(parent)
    , ui(new Ui::addIpWindow)
{
    ui->setupUi(this);
}

addIpWindow::~addIpWindow()
{
    delete ui;
}

void addIpWindow::on_AddButton_clicked()
{
    accept();
}

QString addIpWindow::getIpAddress() const {
    return ui->IpEnter->text();
}

QString addIpWindow::getPort() const {
    return ui->PortEnter->text();
}

void addIpWindow::on_CancelButton_clicked()
{
    reject();
}
