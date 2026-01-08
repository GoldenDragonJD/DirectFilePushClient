#ifndef NOTIFICATIONMANAGER_H
#define NOTIFICATIONMANAGER_H

#include <QObject>
#include <QSystemTrayIcon>

class NotificationManager: public QObject
{
    Q_OBJECT
public:
    explicit NotificationManager(QObject *parent = nullptr);

    void notifyIfNotFocused(const QString &title, const QString &message, const int timeout = 5000);

    static bool appIsActive();

private:
    QSystemTrayIcon m_tray;
};

#endif // NOTIFICATIONMANAGER_H
