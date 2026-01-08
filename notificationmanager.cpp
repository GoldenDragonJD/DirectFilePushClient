#include "notificationmanager.h"
#include <QIcon>
#include <QGuiApplication>

NotificationManager::NotificationManager(QObject *parent) : QObject(parent)
{
    m_tray.setIcon(QIcon(":/packaging/Icons/com.GoldenDragonJD.DirectFilePushClient.png"));
    m_tray.setToolTip("DirectFilePushClient");
    m_tray.setVisible(true);
}

bool NotificationManager::appIsActive()
{
    return QGuiApplication::applicationState() == Qt::ApplicationActive;
}

void NotificationManager::notifyIfNotFocused(const QString &title, const QString &message, int timeoutMs)
{
    if (appIsActive())
        return;

    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        // Fallback could be a log or in-app queue; don't hard fail.
        return;
    }

    m_tray.showMessage(title, message, QSystemTrayIcon::Information, timeoutMs);
}
