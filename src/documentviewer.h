// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2024 danilo agostini <kyre334@gmail.com>
#ifndef DOCUMENTVIEWER_H
#define DOCUMENTVIEWER_H
#include <QFileInfo>
#include <QObject>
#include <QProcess>
#include <QProperty>
#include <QTemporaryDir>
#include <QTimer>
#include <QUrl>
#include <QtQmlIntegration/qqmlintegration.h>

#include <memory>
class DocumentViewer : public QObject
{
    Q_OBJECT
    QML_ANONYMOUS
    Q_PROPERTY(QUrl viewerDocument MEMBER m_viewerDocument NOTIFY viewerDocumentChanged FINAL)
    Q_PROPERTY(bool messagePageVisible MEMBER m_messagePageVisible NOTIFY messagePageVisibleChanged FINAL)
    Q_PROPERTY(QString messageHeaderText MEMBER m_messageHeaderText NOTIFY messageHeaderTextChanged FINAL)
    Q_PROPERTY(QString messageDescriptionText MEMBER m_messageDescriptionText NOTIFY messageDescriptionTextChanged FINAL)
    Q_PROPERTY(bool reloadButtonVisible MEMBER m_reloadButtonVisible NOTIFY reloadButtonVisibleChanged FINAL)
    Q_PROPERTY(bool loadingBarVisible MEMBER m_loadingBarVisible NOTIFY loadingBarVisibleChanged FINAL)
    Q_PROPERTY(bool helpPageLinkVisible MEMBER m_helpPageLinkVisible NOTIFY helpPageLinkVisibleChanged FINAL)

public:
    enum LoadingError { NoError, Timeout, UnknownError, FileNotFound, MissingLibreoffice, ErrorConverting };
    Q_ENUM(LoadingError)

    explicit DocumentViewer(QObject *parent = nullptr);
    void loadFile(const QFileInfo &fileInfo, const QString &tempDir = QString());

    QString currentFilePath;
    void loadingFailed(LoadingError errorCode, QString customError = QString());
    void stopViewer();
    Q_INVOKABLE void reloadDocument();
    bool isActiveViewer = false;

Q_SIGNALS:
    void viewerDocumentChanged();
    void messageHeaderTextChanged();

    void messagePageVisibleChanged();

    void messageDescriptionTextChanged();

    void reloadButtonVisibleChanged();

    void loadingBarVisibleChanged();

    void helpPageLinkVisibleChanged();

private:
    void convertDocument(const QString &path);
    void conversionFinished(QProcess *process, const QString &path, int exitCode, QProcess::ExitStatus exitStatus);
    void closeActiveConversionProcess();

    QString m_tempDir;
    QProcess *m_process = nullptr;
    QTimer m_conversionTimeout;
    std::unique_ptr<QTemporaryDir> m_conversionDir;
    std::unique_ptr<QTemporaryDir> m_shownConversionDir;
    bool m_profileReady = false;

    QString getLibreofficeCommand();
    QString libreofficeCommand;
    bool m_libreofficeLookedUp = false;
    // How LibreOffice is started.
    enum class LibreofficeKind {
        Native, // the libreoffice command
        Flatpak, // the org.libreoffice.LibreOffice Flatpak, with flatpak run
        HostFlatpak, // the Flatpak of the host, with flatpak-spawn from inside the Kiview Flatpak
    };
    LibreofficeKind m_libreofficeKind = LibreofficeKind::Native;
    // Kiview runs inside a Flatpak sandbox.
    bool m_inFlatpak = false;
    // The directory for the conversions and the LibreOffice profile, which LibreOffice can write to.
    QString workDirectory() const;
    void showLoadingScreen();
    Q_OBJECT_BINDABLE_PROPERTY(DocumentViewer, QUrl, m_viewerDocument, &DocumentViewer::viewerDocumentChanged)
    Q_OBJECT_BINDABLE_PROPERTY(DocumentViewer, QString, m_messageHeaderText, &DocumentViewer::messageHeaderTextChanged)
    Q_OBJECT_BINDABLE_PROPERTY(DocumentViewer, bool, m_messagePageVisible, &DocumentViewer::messagePageVisibleChanged)
    Q_OBJECT_BINDABLE_PROPERTY(DocumentViewer, QString, m_messageDescriptionText, &DocumentViewer::messageDescriptionTextChanged)
    Q_OBJECT_BINDABLE_PROPERTY(DocumentViewer, bool, m_reloadButtonVisible, &DocumentViewer::reloadButtonVisibleChanged)
    Q_OBJECT_BINDABLE_PROPERTY(DocumentViewer, bool, m_loadingBarVisible, &DocumentViewer::loadingBarVisibleChanged)
    Q_OBJECT_BINDABLE_PROPERTY(DocumentViewer, bool, m_helpPageLinkVisible, &DocumentViewer::helpPageLinkVisibleChanged)
};

#endif // DOCUMENTVIEWER_H
