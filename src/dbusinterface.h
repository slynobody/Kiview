// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2024 danilo agostini <kyre334@gmail.com>
#ifndef DBUSINTERFACE_H
#define DBUSINTERFACE_H
// Qt
#include <QDBusContext>
#include <QDBusError>
#include <QObject>
#include <QProperty>
#include <QQmlEngine>
#include <QTimer>
// Kde
#include <KDBusService>
// Other
#include <contentmanager.h>

class DBusInterface : public QObject, protected QDBusContext
{
    Q_OBJECT
    QML_NAMED_ELEMENT(BusInterface)
    QML_SINGLETON
    Q_CLASSINFO("D-Bus Interface", "org.kde.kiview")
    Q_PROPERTY(bool isDaemon MEMBER m_isDaemon NOTIFY isDaemonChanged FINAL)
    // linkedAppkey is the key/ID of the application that has control over Kiview.
    // Applications must use it to determine whether they should ignore the "keyPressesForwarded" signal or not.
    Q_PROPERTY(QString linkedAppKey MEMBER m_linkedAppKey NOTIFY linkedAppKeyChanged FINAL)

public:
    explicit DBusInterface(ContentManager *contentManager, QObject *parent = nullptr);
    // Returns the instance that main() creates, which stays owned by C++.
    static DBusInterface *create(QQmlEngine *, QJSEngine *);
    // This method is called by the applications to set the data regarding the files to display
    // and the application key.
    Q_SCRIPTABLE QString setFileContext(const QString appKey, const QString rootDirectory, const QStringList filesList, const QString selectedFile);

    Q_INVOKABLE void SendKeyPresses(const int &key, const int &modifiers);

    void startService();

Q_SIGNALS:
    void isDaemonChanged();
    // The keys used to scroll through files/perform certain actions
    // are sent via D-Bus signal (which apps subscribe to) to the underlying application (dolphin, etc).
    // This way the underlying application can manage how to react to these keys being pressed and tell kiview what to do.
    // This also has the advantage of maintaining the file sorting and having a more transparent integration.
    // Kiview is more like an extension of the program that calls it, rather than a stand-alone application.
    void keyPressesForwarded(int key, int modifiers);

    void linkedAppKeyChanged();

private:
    static inline DBusInterface *s_instance = nullptr;
    // The dbus interface requires `m_contentManager` in order to load the data received via dbus.
    ContentManager *m_contentManager;
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(DBusInterface, bool, m_isDaemon, false, &DBusInterface::isDaemonChanged)
    bool scrollingEnabled = true;
    QTimer *enableScrollingTimer = new QTimer(this);
    void enableScrolling();
    Q_OBJECT_BINDABLE_PROPERTY(DBusInterface, QString, m_linkedAppKey, &DBusInterface::linkedAppKeyChanged)
};

#endif // DBUSINTERFACE_H
