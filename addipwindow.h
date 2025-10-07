#ifndef ADDIPWINDOW_H
#define ADDIPWINDOW_H

#include <QDialog>
#include "mainwindow.h"

namespace Ui {
class addIpWindow;
}

class addIpWindow : public QDialog
{
    Q_OBJECT

public:
    explicit addIpWindow(MainWindow *parent = nullptr);
    ~addIpWindow();

    QString getIpAddress() const;
    QString getPort() const;

private:
    Ui::addIpWindow *ui;

private slots:
    void on_AddButton_clicked();
    void on_CancelButton_clicked();
};

#endif // ADDIPWINDOW_H
