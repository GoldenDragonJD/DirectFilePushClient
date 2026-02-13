#include "notificationmanager.h"
#include <QIcon>
#include <QGuiApplication>

NotificationManager::NotificationManager(QObject *parent) : QObject(parent), m_tray(this)
{
    m_tray.setIcon(QIcon(":/packaging/icons/com.GoldenDragonJD.DirectFilePushClient.png"));
    m_tray.setToolTip("DirectFilePushClient");
    m_tray.setVisible(true);
}

NotificationManager::~NotificationManager()
{
    shutdown();
}

void NotificationManager::shutdown()
{
    // Be extra explicit for Plasma
    if (m_tray.isVisible()) {
        m_tray.hide();
        m_tray.setVisible(false);
    }
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
