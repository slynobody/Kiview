// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2024 danilo agostini <kyre334@gmail.com>
#include "contentmanager.h"

#include <KIO/JobUiDelegateFactory>
#include <KIO/OpenUrlJob>
#include <algorithm>

// Qt
#include "QApplication"
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QImageReader>
#include <QMimeDatabase>
#include <QThreadPool>
namespace
{
// Whether a type is text. text/x-shellscript does not inherit text/plain, and application/json does,
// so both tests are needed.
bool isText(const QMimeType &type)
{
    return type.name().startsWith(QStringLiteral("text/")) || type.inherits(QStringLiteral("text/plain"));
}

// PDF, and the office formats that LibreOffice converts to PDF for a preview.
bool isDocument(const QMimeType &type)
{
    static const QStringList documentTypes{
        QStringLiteral("application/pdf"),
        QStringLiteral("application/vnd.oasis.opendocument.text"),
        QStringLiteral("application/vnd.oasis.opendocument.spreadsheet"),
        QStringLiteral("application/vnd.oasis.opendocument.presentation"),
        QStringLiteral("application/msword"),
        QStringLiteral("application/vnd.ms-excel"),
        QStringLiteral("application/vnd.ms-powerpoint"),
        QStringLiteral("application/vnd.openxmlformats-officedocument.wordprocessingml.document"),
        QStringLiteral("application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"),
        QStringLiteral("application/vnd.openxmlformats-officedocument.presentationml.presentation"),
    };
    return std::any_of(documentTypes.cbegin(), documentTypes.cend(), [&type](const QString &documentType) {
        return type.inherits(documentType);
    });
}

// The types that the installed Qt image plugins read, the ones of kimageformats included, and Krita
// files, whose preview Kiview extracts.
bool isImage(const QMimeType &type)
{
    static const QSet<QString> imageTypes = [] {
        QSet<QString> result{QStringLiteral("application/x-krita")};
        const QList<QByteArray> types = QImageReader::supportedMimeTypes();
        for (const QByteArray &imageType : types) {
            result.insert(QString::fromLatin1(imageType));
        }
        return result;
    }();
    if (imageTypes.contains(type.name())) {
        return true;
    }
    const QStringList aliases = type.aliases();
    return std::any_of(aliases.cbegin(), aliases.cend(), [](const QString &alias) {
        return imageTypes.contains(alias);
    });
}

// Audio and video, which Qt Multimedia plays. Playlists are text, and are not played.
bool isMedia(const QMimeType &type)
{
    return (type.name().startsWith(QStringLiteral("audio/")) || type.name().startsWith(QStringLiteral("video/"))) && !isText(type);
}
}

ContentManager::ContentManager(QObject *parent)
    : QObject{parent}
{
    connect(m_containerViewer, &ContainerViewer::elementReady, this, [this](quint64 request, const QString &path) {
        if (request != m_pendingRequest) {
            return;
        }
        m_pendingRequest = 0;
        QList<Level> levels = m_levels;
        if (m_pendingNewLevel || levels.isEmpty()) {
            levels.append(m_pendingLevel);
        } else {
            levels.last() = m_pendingLevel;
        }
        setLevels(levels);
        showFile(QFileInfo(path));
    });
    s_instance = this;
}

ContentManager::~ContentManager()
{
    // The viewers are deleted with this object, so the threads that hand results to them end first.
    stopViewers();
    QThreadPool::globalInstance()->waitForDone();
}

ContentManager *ContentManager::create(QQmlEngine *, QJSEngine *)
{
    QJSEngine::setObjectOwnership(s_instance, QJSEngine::CppOwnership);
    return s_instance;
}

void ContentManager::loadData(const QString &rootDirectory, const QStringList &filesList, const QString &selectedFile)
{
    // Shows the kiview window.
    Q_EMIT showKiview();
    // Loads the data received via Dbus.
    // If the selected file matches the root folder, the current index number is set to 0 in the GUI.
    rootDirInfo = QFileInfo(rootDirectory);
    // The list of files to be displayed is the one received via DBus
    if (m_files != filesList) {
        m_files = filesList;
        Q_EMIT fileCountChanged();
    }
    QFileInfo fileInfo;
    // If no file has been selected (no selection) shows a preview of the root folder.
    if (selectedFile.isEmpty()) {
        // The file to preview is the "root" folder.
        fileInfo = rootDirInfo;
    } else {
        // A file has been selected:
        // shows the preview of the selected file.
        fileInfo = QFileInfo(selectedFile);
    }

    // loads the file
    loadFile(fileInfo);
}

void ContentManager::loadAdjacentFile(int offset)
{
    // Inside a folder or an archive, the arrow keys move through its elements.
    if (!m_levels.isEmpty()) {
        Level level = m_levels.last();
        const int index = std::clamp(level.index + offset, 0, int(level.elements.size()) - 1);
        if (index != level.index) {
            level.index = index;
            prepareElement(level, false);
        }
        return;
    }
    if (m_files.isEmpty()) {
        return;
    }
    // The root folder has index -1, so the first step forward from it shows the first file.
    const int currentIndex = m_currentIndex;
    const int newIndex = std::clamp(currentIndex + offset, 0, int(m_files.size()) - 1);
    if (newIndex == currentIndex) {
        return;
    }
    loadFile(QFileInfo(m_files.at(newIndex)));
}

void ContentManager::stopViewers()
{
    // It is used to close running threads,subprocesses, and other viewer-specific
    // things.
    if (m_documentViewer->isActiveViewer) {
        m_documentViewer->stopViewer();
    }
    if (m_imageViewer->isActiveViewer) {
        m_imageViewer->stopViewer();
    }
    if (m_containerViewer->isActiveViewer) {
        m_containerViewer->stopViewer();
    }
    if (m_textViewer->isActiveViewer) {
        m_textViewer->stopViewer();
    }
}

void ContentManager::loadFile(const QFileInfo &fileInfo)
{
    // A file opened from the outside starts again from the top.
    if (!m_levels.isEmpty()) {
        setLevels({});
        m_containerViewer->clearExtractedElements();
    }
    m_pendingRequest = 0;
    showFile(fileInfo);
}

void ContentManager::openElement(int index)
{
    const QList<ElementItem> elements = m_containerViewer->elements();
    if (index < 0 || index >= elements.size()) {
        return;
    }
    prepareElement(Level{currentFileInfo, m_containerViewer->isShowingArchive(), elements, index}, true);
}

void ContentManager::prepareElement(const Level &level, bool newLevel)
{
    m_pendingLevel = level;
    m_pendingNewLevel = newLevel;
    m_pendingRequest = m_containerViewer->prepareElement(level.container.absoluteFilePath(), level.isArchive, level.elements.at(level.index).path);
}

void ContentManager::goUp()
{
    if (m_levels.isEmpty()) {
        return;
    }
    m_pendingRequest = 0;
    QList<Level> levels = m_levels;
    const QFileInfo container = levels.takeLast().container;
    setLevels(levels);
    showFile(container);
}

bool ContentManager::canGoUp() const
{
    return !m_levels.isEmpty();
}

void ContentManager::setLevels(const QList<Level> &levels)
{
    const bool couldGoUp = canGoUp();
    const int oldFileCount = fileCount();
    m_levels = levels;
    if (couldGoUp != canGoUp()) {
        Q_EMIT canGoUpChanged();
    }
    if (oldFileCount != fileCount()) {
        Q_EMIT fileCountChanged();
    }
}

void ContentManager::showFile(const QFileInfo &fileInfo)
{
    currentFileInfo = fileInfo;
    // stops possible running subprocesses/threads.
    stopViewers();

    if (!m_levels.isEmpty()) {
        // The position in the folder or the archive that the file came from.
        m_currentIndex = m_levels.last().index;
    } else {
        m_currentIndex = fileInfo == rootDirInfo ? -1 : m_files.indexOf(currentFileInfo.absoluteFilePath());
    }
    // sets the index number of the file in the title bar, when there is a file list to count.
    m_windowTitle = fileCount() > 0 ? QStringLiteral("[%1/%2]").arg(m_currentIndex + 1).arg(fileCount()) : QString();
    // sets the file name as the title in the "header bar"
    m_fileName = currentFileInfo.fileName();

    // The type comes from the name, and from the start of the content when the name gives no type.
    // The order matters: Qt reads PDF as an image too, an OpenDocument file is a zip archive, and
    // SVG is XML, which is text.
    const QMimeType type = currentFileInfo.isDir() ? QMimeType() : QMimeDatabase().mimeTypeForFile(currentFileInfo);
    if (currentFileInfo.isDir()) {
        m_containerViewer->loadFile(currentFileInfo, false, tempDir.path());
        m_currentViewer = Viewer::Container;
    } else if (isDocument(type)) {
        m_documentViewer->loadFile(currentFileInfo, tempDir.path());
        m_currentViewer = Viewer::Document;
    } else if (isImage(type)) {
        m_imageViewer->loadFile(currentFileInfo, tempDir.path());
        m_currentViewer = Viewer::Image;
    } else if (isMedia(type)) {
        m_videoViewer->loadFile(currentFileInfo);
        m_currentViewer = Viewer::Video;
    } else if (ContainerViewer::isArchive(type)) {
        m_containerViewer->loadFile(currentFileInfo, true, tempDir.path());
        m_currentViewer = Viewer::Container;
    } else if (isText(type)) {
        m_textViewer->loadFile(currentFileInfo);
        m_currentViewer = Viewer::Text;
    } else {
        m_currentViewer = Viewer::Fallback;
    }
}

void ContentManager::openApp()
{
    // KIO picks the application that Dolphin picks, goes through the portal inside a Flatpak, and
    // asks before it runs an executable file.
    auto *job = new KIO::OpenUrlJob(QUrl::fromLocalFile(currentFileInfo.absoluteFilePath()));
    job->setUiDelegate(KIO::createDefaultJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, nullptr));
    // Closing Kiview would end the job, so Kiview closes once the application has started.
    connect(job, &KJob::result, this, [this](KJob *job) {
        if (!job->error()) {
            Q_EMIT closeQml();
        }
    });
    job->start();
}

void ContentManager::closing()
{
    // when the app is closing.
    stopViewers();
}

int ContentManager::fileCount() const
{
    return m_levels.isEmpty() ? int(m_files.size()) : int(m_levels.last().elements.size());
}
