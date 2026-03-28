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

    // 1. Safely cast parent to a MainWindow pointer
    MainWindow *main = qobject_cast<MainWindow*>(parent);

    if (main)
    {
        // 2. Initialize the string so it's not empty if the user just clicks OK
        folderDirectory = main->transfer_file_path;

        // 3. Set the UI text
        ui->dirPath->setText(folderDirectory);
    }
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
