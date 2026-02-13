#include "settingswindow.h"
#include "ui_settingswindow.h"
#include <QFileDialog>
#include <QFile>
#include <QStandardPaths>
#include "mainwindow.h"

settingsWindow::settingsWindow(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::settingsWindow)
{
    ui->setupUi(this);

    MainWindow main = reinterpret_cast<MainWindow*>(parent);

    ui->dirPath->setText(main.transfer_file_path);
}

settingsWindow::~settingsWindow()
{
    delete ui;
}

void settingsWindow::on_folderButton_clicked()
{
    folderDirectory = QFileDialog::getExistingDirectory();
    ui->dirPath->setText(folderDirectory);
}

void settingsWindow::on_defaultButton_clicked()
{
    folderDirectory = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation) + "/Files-Received";
    ui->dirPath->setText(folderDirectory);
}
