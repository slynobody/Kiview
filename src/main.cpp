// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2024 danilo agostini <kyre334@gmail.com>
#include <KDBusService>
#include <KLocalizedQmlContext>
#include <KLocalizedString>
#include <QApplication>
#include <QCommandLineParser>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QUrl>
#include <QtLogging>
#include <contentmanager.h>
#include <dbusinterface.h>
#include <kiviewapplication.h>

int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);

    QApplication app(argc, argv);

    KLocalizedString::setApplicationDomain("kiview");
    QCoreApplication::setOrganizationName(QStringLiteral("KDE"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("kde.org"));
    QCoreApplication::setApplicationName(QStringLiteral("Kiview"));

    // ContentManager manages which viewer is active
    ContentManager manager(nullptr);
    // DBus interface
    // The dbus interface requires `manager` in order to load the data received via dbus.
    DBusInterface interface(&manager);
    // The actions and their shortcuts, read from the configuration.
    KiviewApplication kiviewApplication(nullptr);

    // Declared after the objects that QML uses, so that it and the QML objects are destroyed first.
    QQmlApplicationEngine engine;

    KLocalization::setupLocalizedContext(&engine);

    // Command line options.
    // Uses QCommandLineParser to add parameters and get the arguments from the command line.
    QCommandLineParser commandLineParser;

    commandLineParser.setApplicationDescription(
        i18nc("@info:shell", "Kiview - File previewer"));

    commandLineParser.addHelpOption();

    // Creates command line options
    QCommandLineOption filePathOption(QStringList() << QStringLiteral("s") << QStringLiteral("show-file"),
                                      i18nc("@info:shell", "Show a preview of the file (single-file mode)."),
                                      i18nc("@info:shell", "file"));

    QCommandLineOption daemonModeOption(QStringList() << QStringLiteral("d") << QStringLiteral("daemon"),
                                        i18nc("@info:shell",
                                              "Kiview starts in the background and waits to receive data via the D-Bus interface. "
                                              "In this mode, Kiview does not close when the window is closed."));

    // Adds command line options.
    commandLineParser.addOption(filePathOption);
    commandLineParser.addOption(daemonModeOption);
    commandLineParser.addPositionalArgument(QStringLiteral("files"),
                                            i18nc("@info:shell",
                                                  "Files to preview. A single file opens in single-file mode, "
                                                  "and the arrow keys move between several files."),
                                            i18nc("@info:shell", "[files...]"));

    // Processes command line arguments.
    commandLineParser.process(app);

    // Retrieves command line arguments.
    const bool isDaemon = commandLineParser.isSet(daemonModeOption);
    const QStringList positionalArgs = commandLineParser.positionalArguments();
    QString filePath = commandLineParser.value(filePathOption);
    if (filePath.isEmpty() && positionalArgs.size() == 1) {
        filePath = positionalArgs.constFirst();
    }
    const bool isFilePathPresent = !filePath.isEmpty();
    // Several files are loaded like a file list received through setFileContext,
    // with the folder of the first file as the root directory and the first file selected.
    QStringList filesList;
    if (!isFilePathPresent && positionalArgs.size() > 1) {
        for (const QString &path : positionalArgs) {
            const QFileInfo fileInfo(path);
            if (!fileInfo.exists()) {
                qCritical() << "Error: Invalid path:" << path;
                exit(1);
            }
            filesList.append(fileInfo.absoluteFilePath());
        }
        // The navigation finds the current file by its first position in the list.
        filesList.removeDuplicates();
    }

    // handles arguments provided via CLI.
    if (isDaemon) {
        // Kiview runs in the background and waits to receive data via DBus.
        // The service does not need to be registered if in single file mode.
        KDBusService service(KDBusService::Unique);
        interface.startService();
        qInfo("Started in daemon mode...");
    } else if (isFilePathPresent) {
        const QFileInfo fileInfo(filePath);
        // Loads the file if a valid path is provided.
        if (fileInfo.exists()) {
            // Kiview displays a preview of the file passed as a command line argument (single-file mode)
            // (No scrolling between files and no index number shown in the title bar.)
            qInfo() << "Loading file: " << fileInfo.absoluteFilePath();
            manager.loadFile(fileInfo);
        } else {
            qCritical() << "Error: Invalid path:" << filePath;
            exit(1);
        }
    } else if (!filesList.isEmpty()) {
        qInfo() << "Loading" << filesList.size() << "files, starting with" << filesList.constFirst();
        manager.loadData(QFileInfo(filesList.constFirst()).absolutePath(), filesList, filesList.constFirst());
    } else {
        // Closes Kiview if no arguments have been given.
        qCritical() << "Error: No arguments given.";
        // Displays the help message and exits
        commandLineParser.showHelp(1);
    }

    engine.loadFromModule("org.kde.kiview", "Main");

    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    QObject::connect(&app, &QCoreApplication::aboutToQuit, &manager, &ContentManager::closing);
    // sets the icon of the app
    app.setWindowIcon(QIcon::fromTheme(QStringLiteral("io.github.nyre221.kiview")));

    return app.exec();
}
