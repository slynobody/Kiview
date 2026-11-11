// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2024 danilo agostini <kyre334@gmail.com>
// Own header
#include "dbusinterface.h"
#include "kiview_debug.h"
// Qt
#include <QDBusConnection>
#include <QDBusConnectionInterface>

DBusInterface::DBusInterface(ContentManager *contentManager, QObject *parent)
    : QObject{parent}
    , m_contentManager(contentManager)
{
    s_instance = this;
    // debounce scrolling events
    // This prevents the QML interface from crashing when the user rapidly presses the navigation keys.
    enableScrollingTimer->setSingleShot(true);
    enableScrollingTimer->setInterval(100);
    connect(enableScrollingTimer, &QTimer::timeout, this, &DBusInterface::enableScrolling);
}

DBusInterface *DBusInterface::create(QQmlEngine *, QJSEngine *)
{
    QJSEngine::setObjectOwnership(s_instance, QJSEngine::CppOwnership);
    return s_instance;
}

// The service does not need to be registered if in single file mode.
void DBusInterface::startService()
{
    QDBusConnection::sessionBus().registerObject(QStringLiteral("/Interface"),
                                                 this,
                                                 QDBusConnection::ExportScriptableContents | QDBusConnection::ExportAdaptors
                                                     | QDBusConnection::ExportAllSignal);
    QDBusConnectionInterface *sessionInterface = QDBusConnection::sessionBus().interface();
    if (sessionInterface) {
        sessionInterface->registerService(QStringLiteral("org.kde.kiview"), QDBusConnectionInterface::QueueService);
    }
    m_isDaemon = true;
}

QString DBusInterface::setFileContext(const QString appKey, const QString rootDirectory, const QStringList filesList, const QString selectedFile)
{
    QString error;

    // Checks if all the necessary data is present.
    if (appKey.isEmpty()) {
        error = QStringLiteral("Error: The Application Key is missing.");
    } else if (rootDirectory.isEmpty()) {
        error = QStringLiteral(" Error: Root directory not set.");
    } else if (!QFileInfo::exists(rootDirectory)) {
        error = QStringLiteral("Error: The root directory is invalid.");
    } else if (filesList.isEmpty()) {
        error = QStringLiteral("Error: The file list is empty.");
    } else if (!selectedFile.isEmpty() && !QFileInfo::exists(selectedFile)) {
        // A file is selected but does not exist.
        error = QStringLiteral("Error: The selected file is invalid.");
    }

    if (error.isEmpty()) {
        m_linkedAppKey = appKey;
        m_contentManager->loadData(rootDirectory, filesList, selectedFile);
    } else {
        sendErrorReply(QDBusError::InvalidArgs, error);
        qCWarning(KIVIEW_LOG) << "setFileContext rejected for" << selectedFile << "in" << rootDirectory << "with" << filesList.size() << "files:" << error;
    }

    return error;
}

void DBusInterface::SendKeyPresses(const int &key, const int &modifiers)
{
    if (!scrollingEnabled) {
        return;
    }
    scrollingEnabled = false;
    enableScrollingTimer->start();
    // Without a linked application, Kiview moves through the files that it was started with. Inside a
    // folder or an archive, it moves through its elements, which the linked application does not know.
    if (m_linkedAppKey.value().isEmpty() || m_contentManager->canGoUp()) {
        if (key == Qt::Key_Left || key == Qt::Key_Up) {
            m_contentManager->loadAdjacentFile(-1);
        } else if (key == Qt::Key_Right || key == Qt::Key_Down) {
            m_contentManager->loadAdjacentFile(1);
        }
        return;
    }
    Q_EMIT keyPressesForwarded(key, modifiers);
}

void DBusInterface::enableScrolling()
{
    scrollingEnabled = true;
}

