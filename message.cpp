#include "message.h"
#include "ui_message.h"

message::message(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::message)
{
    ui->setupUi(this);
}

message::~message()
{
    delete ui;
}

void message::setId(const QString &text)
{
    ui->ClientID->setText(text);
}

void message::setText(const QString &text)
{
    ui->MessageBox->setText(text);
}
