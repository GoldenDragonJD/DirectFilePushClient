#ifndef SETTINGSWINDOW_H
#define SETTINGSWINDOW_H

#include <QDialog>
#include <QFileDialog>

namespace Ui {
class settingsWindow;
}

class settingsWindow : public QDialog
{
    Q_OBJECT

public:
    explicit settingsWindow(QWidget *parent = nullptr);
    ~settingsWindow();

    QString folderDirectory;

private:
    Ui::settingsWindow *ui;

private slots:
    void on_folderButton_clicked();
    void on_defaultButton_clicked();
};

#endif // SETTINGSWINDOW_H
