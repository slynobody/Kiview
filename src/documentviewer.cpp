// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2024 danilo agostini <kyre334@gmail.com>
#include "documentviewer.h"
#include "kiview_document_debug.h"
#include <klocalizedstring.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>

// Qt
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QMimeDatabase>
#include <QProcess>
#include <QStandardPaths>
#include <QUrl>

#include <utility>

using namespace Qt::Literals::StringLiterals;
using namespace std::chrono_literals;

DocumentViewer::DocumentViewer(QObject *parent)
    : QObject{parent}
{
    m_conversionTimeout.setSingleShot(true);
    connect(&m_conversionTimeout, &QTimer::timeout, this, [this]() {
        qCWarning(KIVIEW_DOCUMENT_LOG) << "Converting" << currentFilePath << "took longer than" << m_conversionTimeout.interval()
                                       << "ms - stopping LibreOffice";
        closeActiveConversionProcess();
        loadingFailed(DocumentViewer::LoadingError::Timeout);
    });
}

void DocumentViewer::loadFile(const QFileInfo &fileInfo, const QString &tempDir)
{
    closeActiveConversionProcess();
    showLoadingScreen();
    currentFilePath = fileInfo.absoluteFilePath();
    // it is used to discard the conversion if not longer needed
    isActiveViewer = true;
    if (!tempDir.isEmpty()) {
        m_tempDir = tempDir;
    }

    // A PDF is shown as it is, and other documents are converted to PDF first.
    if (QMimeDatabase().mimeTypeForFile(fileInfo).inherits(QStringLiteral("application/pdf"))) {
        m_viewerDocument = QUrl::fromLocalFile(currentFilePath);
        m_messagePageVisible = false;
        m_loadingBarVisible = false;
    } else {
        convertDocument(currentFilePath);
    }
}

void DocumentViewer::convertDocument(const QString &path)
{
    // The lookup can run flatpak info, so it waits for the first document to convert.
    if (!m_libreofficeLookedUp) {
        libreofficeCommand = getLibreofficeCommand();
        m_libreofficeLookedUp = true;
    }
    // # shows a message and stops the function if libreoffice was not found.
    if (libreofficeCommand.isEmpty()) {
        loadingFailed(DocumentViewer::LoadingError::MissingLibreoffice);
        qCWarning(KIVIEW_DOCUMENT_LOG) << "Cannot convert" << path << "- neither libreoffice nor the org.libreoffice.LibreOffice Flatpak was found";
        return;
    }

    // A directory for each conversion, so that the PDF of another document with the same name is
    // never shown instead of this one.
    const QString workDir = workDirectory();
    m_conversionDir.reset();
    if (!workDir.isEmpty()) {
        m_conversionDir = std::make_unique<QTemporaryDir>(workDir + u"/conversion-XXXXXX"_s);
    }
    if (!m_conversionDir || !m_conversionDir->isValid()) {
        qCWarning(KIVIEW_DOCUMENT_LOG) << "Cannot convert" << path << "- no directory to convert it into under" << workDir;
        loadingFailed(DocumentViewer::LoadingError::UnknownError);
        return;
    }

    QStringList arguments;
    QString document = path;
    switch (m_libreofficeKind) {
    case LibreofficeKind::Native:
        break;
    case LibreofficeKind::Flatpak:
        // flatpak run becomes bwrap, and LibreOffice only ends with it when asked to. LibreOffice gets
        // write access to the work directory for this run only.
        arguments << u"run"_s << u"--die-with-parent"_s << u"--filesystem=%1"_s.arg(workDir) << u"org.libreoffice.LibreOffice"_s;
        break;
    case LibreofficeKind::HostFlatpak:
        // flatpak-spawn passes its signals to the process on the host, and --watch-bus ends that
        // process when flatpak-spawn ends. The work directory is shared with the LibreOffice sandbox.
        arguments << u"--host"_s << u"--watch-bus"_s << u"flatpak"_s << u"run"_s << u"--die-with-parent"_s
                  << u"--filesystem=xdg-run/app/%1"_s.arg(qEnvironmentVariable("FLATPAK_ID")) << u"org.libreoffice.LibreOffice"_s;
        // The files of the host outside the home directory are under /run/host in this sandbox.
        if (document.startsWith(u"/run/host/"_s)) {
            document = document.mid(9);
        }
        break;
    }
    // A profile of its own, so that the settings and macros of the user do not apply to a file that
    // is only previewed, and a LibreOffice the user has open does not take the request.
    arguments << u"-env:UserInstallation=%1"_s.arg(QUrl::fromLocalFile(workDir + u"/libreoffice-profile"_s).toString());
    arguments << u"--headless"_s << u"--nolockcheck"_s << u"--norestore"_s << u"--convert-to"_s << u"pdf"_s << u"--outdir"_s << m_conversionDir->path()
              << document;

    m_process = new QProcess(this);
    m_process->setProgram(libreofficeCommand);
    m_process->setArguments(arguments);
    // The native command is a script that starts soffice.bin, so the whole group is stopped.
    m_process->setChildProcessModifier([]() {
        ::setpgid(0, 0);
    });
    connect(m_process, &QProcess::finished, this, [this, process = m_process, path](int exitCode, QProcess::ExitStatus exitStatus) {
        conversionFinished(process, path, exitCode, exitStatus);
    });
    connect(m_process, &QProcess::errorOccurred, this, [this, process = m_process](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart && process == m_process) {
            qCWarning(KIVIEW_DOCUMENT_LOG) << "Could not start" << process->program() << "to convert" << currentFilePath << ":" << process->errorString();
            closeActiveConversionProcess();
            loadingFailed(DocumentViewer::LoadingError::MissingLibreoffice);
        }
    });

    // The first conversion also creates the profile, which takes longer.
    m_conversionTimeout.start(m_profileReady ? 10s : 60s);
    qCDebug(KIVIEW_DOCUMENT_LOG) << "Converting" << path << "with" << libreofficeCommand << arguments;
    m_process->start();
}

void DocumentViewer::conversionFinished(QProcess *process, const QString &path, int exitCode, QProcess::ExitStatus exitStatus)
{
    if (process != m_process) {
        return;
    }
    m_conversionTimeout.stop();
    m_process = nullptr;
    process->deleteLater();

    if (path != currentFilePath || !isActiveViewer) {
        return;
    }

    const QStringList pdfFiles = QDir(m_conversionDir->path()).entryList({u"*.pdf"_s}, QDir::Files);
    if (exitStatus == QProcess::NormalExit && exitCode == 0 && !pdfFiles.isEmpty()) {
        m_profileReady = true;
        qCDebug(KIVIEW_DOCUMENT_LOG) << "Converted" << path << "to" << pdfFiles.first() << "in" << m_conversionDir->path();
        // The directory of the PDF shown before goes now.
        m_shownConversionDir = std::move(m_conversionDir);
        loadFile(QFileInfo(m_shownConversionDir->filePath(pdfFiles.first())));
        return;
    }

    QString standardError = QString::fromUtf8(process->readAllStandardError());
    // removes the warning part of the text (always present with libreoffice)
    // and the newline character because otherwise the html link that is
    // inserted after it doesn't work.
    const int pos = standardError.indexOf(QStringLiteral("Error"));
    if (pos >= 0) {
        qCWarning(KIVIEW_DOCUMENT_LOG) << "LibreOffice could not convert" << path << ", exit code" << exitCode << ":" << standardError.trimmed();
        loadingFailed(DocumentViewer::LoadingError::ErrorConverting, standardError.mid(pos).trimmed());
    } else if (exitStatus == QProcess::NormalExit && exitCode == 0) {
        // LibreOffice wrote its PDF where this process cannot see it, e.g. outside a Flatpak sandbox.
        qCWarning(KIVIEW_DOCUMENT_LOG) << "LibreOffice converted" << path << "but wrote no PDF to" << m_conversionDir->path();
        loadingFailed(DocumentViewer::LoadingError::FileNotFound);
    } else {
        qCWarning(KIVIEW_DOCUMENT_LOG) << "LibreOffice ended with" << exitCode << exitStatus << "converting" << path << ":" << standardError.trimmed();
        loadingFailed(DocumentViewer::LoadingError::UnknownError);
    }
}

void DocumentViewer::stopViewer()
{
    isActiveViewer = false;
    closeActiveConversionProcess();
}

void DocumentViewer::showLoadingScreen()
{
    m_messagePageVisible = true;
    m_messageHeaderText = QString();
    m_messageDescriptionText = QString();
    m_loadingBarVisible = true;
    m_reloadButtonVisible = false;
    m_helpPageLinkVisible = false;
}

void DocumentViewer::reloadDocument()
{
    loadFile(QFileInfo(currentFilePath));
}

void DocumentViewer::closeActiveConversionProcess()
{
    m_conversionTimeout.stop();
    QProcess *process = std::exchange(m_process, nullptr);
    if (!process) {
        return;
    }
    process->disconnect(this);
    if (process->state() == QProcess::NotRunning) {
        process->deleteLater();
        return;
    }

    // On SIGTERM, oosplash also ends the soffice.bin it started, and bwrap ends the sandbox.
    process->terminate();
    connect(process, &QProcess::finished, process, &QObject::deleteLater);
    // QProcess::kill() would leave soffice.bin running, so a LibreOffice that ignored SIGTERM is
    // killed with its whole process group, see convertDocument().
    const pid_t group = pid_t(process->processId());
    QTimer::singleShot(3s, process, [group]() {
        ::kill(-group, SIGKILL);
    });
}

QString DocumentViewer::getLibreofficeCommand()
{
    // checks which version of libreoffice is installed.
    QString command;

    m_inFlatpak = QFile::exists(u"/.flatpak-info"_s);

    if (!QStandardPaths::findExecutable(QStringLiteral("libreoffice")).isEmpty()) {
        // check if libreoffice is installed
        command = QStringLiteral("libreoffice");
        m_libreofficeKind = LibreofficeKind::Native;
    } else if (!m_inFlatpak && !QStandardPaths::findExecutable(QStringLiteral("flatpak")).isEmpty()) {
        // check if libreoffice is installed with flatpak
        QProcess process;
        process.start(QStringLiteral("flatpak"), {QStringLiteral("info"), QStringLiteral("org.libreoffice.LibreOffice")});
        process.waitForFinished();
        if (process.exitCode() == EXIT_SUCCESS) {
            m_libreofficeKind = LibreofficeKind::Flatpak;
            command = QStringLiteral("flatpak");
        }
    } else if (m_inFlatpak && !QStandardPaths::findExecutable(QStringLiteral("flatpak-spawn")).isEmpty()) {
        // check if the host has the libreoffice Flatpak
        QProcess process;
        process.start(QStringLiteral("flatpak-spawn"), {u"--host"_s, u"flatpak"_s, u"info"_s, u"org.libreoffice.LibreOffice"_s});
        process.waitForFinished();
        if (process.exitStatus() == QProcess::NormalExit && process.exitCode() == EXIT_SUCCESS) {
            m_libreofficeKind = LibreofficeKind::HostFlatpak;
            command = QStringLiteral("flatpak-spawn");
        }
    }
    return command;
}

QString DocumentViewer::workDirectory() const
{
    if (m_libreofficeKind == LibreofficeKind::HostFlatpak) {
        // Flatpak shares $XDG_RUNTIME_DIR/app/<application id> between the host and the sandbox.
        return QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation) + u"/app/"_s + qEnvironmentVariable("FLATPAK_ID");
    }
    return m_tempDir;
}

void DocumentViewer::loadingFailed(LoadingError error, QString customError)
{
    // code 1: file took too long to load
    // code 2: general error
    // code 3: flatpak error
    // code 4: libreoffice missing
    // code 999: libreoffice error message
    m_loadingBarVisible = false;
    m_messageHeaderText = i18n("Preview not available.");

    if (error == Timeout) {
        m_messageDescriptionText = i18n("The document took too long to load.");
        m_reloadButtonVisible = true;
    } else if (error == UnknownError) {
        // general message and reload option
        m_messageDescriptionText = i18n("Something went wrong.");
        m_reloadButtonVisible = true;
    } else if (error == FileNotFound) {
        // new pdf not found/flatpak error
        m_helpPageLinkVisible = true;
        m_messageDescriptionText = i18n("File not found: Flatpak may not be configured correctly:");
    } else if (error == MissingLibreoffice) {
        // The appstream link opens the software center on the LibreOffice page.
        const QString installLink =
            u"<a href=\"appstream://org.libreoffice.LibreOffice\">%1</a>"_s.arg(i18nc("@action link to the software center", "Install LibreOffice"));
        const QString reason = m_inFlatpak ? i18n("Office documents are previewed with the LibreOffice Flatpak, which is not installed on this system.")
                                           : i18n("Office documents are previewed with LibreOffice, which is not installed.");
        m_messageDescriptionText = reason + u"<br>"_s + installLink;
    } else if (error == ErrorConverting) {
        // libre error/general error: source could not be loaded
        m_helpPageLinkVisible = true;
        m_messageDescriptionText = customError + QStringLiteral(":");
    }
}

