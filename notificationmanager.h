#ifndef NOTIFICATIONMANAGER_H
#define NOTIFICATIONMANAGER_H

#include <QObject>
#include <QSystemTrayIcon>

class NotificationManager: public QObject
{
    Q_OBJECT
public:
    explicit NotificationManager(QObject *parent = nullptr);
    ~NotificationManager() override;

    void notifyIfNotFocused(const QString &title, const QString &message, int timeoutMs = 5000);
    static bool appIsActive();

    void shutdown();  // add this

private:
    QSystemTrayIcon m_tray;
};


#endif // NOTIFICATIONMANAGER_H
