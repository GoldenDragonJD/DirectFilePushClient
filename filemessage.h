#ifndef FILEMESSAGE_H
#define FILEMESSAGE_H

#include <QWidget>

namespace Ui {
class fileMessage;
}

class fileMessage : public QWidget
{
    Q_OBJECT

public:
    explicit fileMessage(QWidget *parent = nullptr);
    ~fileMessage();

    void setProgress(const int &progress);
    void setToAndFromClient(const QString &fromClient, const QString &toClient);
    void setStatus(const QString &status);
    void setFileName(const QString &filename);

private:
    Ui::fileMessage *ui;
};

#endif // FILEMESSAGE_H
