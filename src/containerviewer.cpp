// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2024 danilo agostini <kyre334@gmail.com>
#include "containerviewer.h"
#include "guithread.h"
#include "kiview_debug.h"
#include <klocalizedstring.h>

// Kde
#include <K7Zip>
#include <KArchiveFile>
#include <KCompressionDevice>
#include <KFormat>
#include <KTar>
#include <KZip>

// Qt
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMimeDatabase>
#include <QThreadPool>

#include <functional>

namespace
{
// The most that the listing of a compressed tar may decompress.
constexpr qint64 maximumDecompressedSize = 1024LL * 1024 * 1024;

// Reads a compressed tar for KTar, which only reads and skips forward to list it. Being sequential,
// a skip goes through readData() too, so every decompressed byte is counted, and the reading stops
// past the limit or once the archive is no longer shown.
class BoundedReader : public QIODevice
{
public:
    BoundedReader(QIODevice *source, std::function<bool()> cancelled)
        : m_source(source)
        , m_cancelled(std::move(cancelled))
    {
    }

    bool isSequential() const override
    {
        return true;
    }

    bool tooBig() const
    {
        return m_tooBig;
    }

protected:
    qint64 readData(char *data, qint64 maxSize) override
    {
        if (m_cancelled()) {
            return -1;
        }
        if (m_read + maxSize > maximumDecompressedSize) {
            m_tooBig = true;
            return -1;
        }
        const qint64 n = m_source->read(data, maxSize);
        if (n > 0) {
            m_read += n;
        }
        return n;
    }

    qint64 writeData(const char *, qint64) override
    {
        return -1;
    }

private:
    QIODevice *m_source;
    std::function<bool()> m_cancelled;
    qint64 m_read = 0;
    bool m_tooBig = false;
};

enum class ArchiveType {
    None,
    Zip,
    SevenZip,
    Tar,
};

// The type of archive of a MIME type. A tar archive has a MIME type of its own for each compression,
// the same for .tar.gz and .tgz, and the formats built on zip inherit application/zip.
ArchiveType archiveType(const QMimeType &type)
{
    if (type.inherits(QStringLiteral("application/zip"))) {
        return ArchiveType::Zip;
    }
    if (type.inherits(QStringLiteral("application/x-7z-compressed"))) {
        return ArchiveType::SevenZip;
    }
    static const QStringList tarTypes{
        QStringLiteral("application/x-tar"),
        QStringLiteral("application/x-compressed-tar"),
        QStringLiteral("application/x-bzip2-compressed-tar"),
        QStringLiteral("application/x-xz-compressed-tar"),
        QStringLiteral("application/x-zstd-compressed-tar"),
        QStringLiteral("application/x-lzma-compressed-tar"),
    };
    for (const QString &tarType : tarTypes) {
        if (type.inherits(tarType)) {
            return ArchiveType::Tar;
        }
    }
    return ArchiveType::None;
}

// Returns the archive at path, after the type that its name gives, or nullptr for another type.
std::unique_ptr<KArchive> createArchive(const QString &path)
{
    switch (archiveType(QMimeDatabase().mimeTypeForFile(path, QMimeDatabase::MatchExtension))) {
    case ArchiveType::Zip:
        return std::make_unique<KZip>(path);
    case ArchiveType::SevenZip:
        return std::make_unique<K7Zip>(path);
    case ArchiveType::Tar:
        return std::make_unique<KTar>(path);
    case ArchiveType::None:
        break;
    }
    return nullptr;
}

// Sets the icons of an element from the type that its name gives, without reading the file.
void setIcons(ElementItem &element, const QMimeDatabase &database)
{
    if (element.fileType == ElementItem::Dir) {
        element.iconName = QStringLiteral("folder");
        element.genericIconName = element.iconName;
        return;
    }
    const QMimeType type = database.mimeTypeForFile(element.path, QMimeDatabase::MatchExtension);
    element.iconName = type.iconName();
    element.genericIconName = type.genericIconName();
}
}

ContainerViewer::ContainerViewer(QObject *parent)
    : QObject{parent}
{
}

void ContainerViewer::loadFile(const QFileInfo &fileInfo, bool isArchive, const QString &tempDir)
{
    resetView();
    m_isArchive = isArchive;
    if (!tempDir.isEmpty()) {
        m_tempDir = tempDir;
    }
    // it is used to discard the thread if not longer needed
    isActiveViewer = true;
    ++m_generation;
    currentFilePath = fileInfo.absoluteFilePath();

    // gets last modified time
    const QDateTime lastModified = fileInfo.lastModified();
    QString stringLastModified = QLocale().toString(lastModified.date(), QLocale::ShortFormat);
    // sets the info in the interface
    m_fileLastModified = stringLastModified;

    if (isArchive) {
        m_fileIcon = QStringLiteral("application-zip");
        // Runs the function in another thread to avoid freezing the interface
        // The worker gets its own QFileInfo: one shared with this thread caches what it reads.
        QThreadPool::globalInstance()->start([this, path = currentFilePath, generation = quint64(m_generation)]() {
            loadArchive(QFileInfo(path), generation);
        });
    } else {
        m_fileIcon = QStringLiteral("folder-blue");
        // Listing reads the metadata of every entry, which is slow on a large or remote folder.
        QThreadPool::globalInstance()->start([this, path = currentFilePath]() {
            loadDir(path);
        });
    }
}

void ContainerViewer::loadDir(const QString &directoryPath)
{
    // the list containing files and directories that is passed to Qml.
    QList<ElementItem> elementsList;
    const QMimeDatabase mimeDatabase;
    QDir directory = QDir(directoryPath);
    // searches for files and folders in the given folder
    directory.setFilter(QDir::Dirs | QDir::Files | QDir::NoDot | QDir::NoDotDot);
    directory.setSorting(QDir::Name | QDir::DirsFirst);
    for (const QFileInfo &entry : directory.entryInfoList()) {
        if (entry.isHidden()) {
            // excludes hidden folders from the list.
            continue;
        }

        // the object that will contain the information about the element and will be accessed by qml.
        ElementItem element;
        element.path = entry.fileName();

        if (entry.isDir()) {
            element.fileType = ElementItem::Dir;
        } else if (entry.isSymLink()) {
            element.fileType = ElementItem::Symlink;
        } else {
            element.fileType = ElementItem::File;
        }
        setIcons(element, mimeDatabase);

        elementsList.append(element);
    }

    applyIfCurrent(this, directoryPath, [this, elementsList]() {
        m_loadingBarVisible = false;
        // sets the elements in the interface
        m_viewerElements = elementsList;
        m_fileContentCount = QString::number(elementsList.count());
    });
}

void ContainerViewer::loadArchive(const QFileInfo &fileInfo, quint64 generation)
{
    QString path = fileInfo.absoluteFilePath();

    const qint64 size = fileInfo.size();
    applyIfCurrent(this, path, [this, formattedSize = KFormat().formatByteSize(size)]() {
        m_fileSize = formattedSize;
    });

    // avoid opening the archive if it is too big.
    if (size > 1073741824) { // 1gib
        // file too big
        qCInfo(KIVIEW_LOG) << "No preview of" << path << "- the archive is" << size << "bytes, more than the 1 GiB limit";
        applyIfCurrent(this, path, [this]() {
            loadingFailed(i18n("Preview not available: File too big"));
        });
        return;
    }

    auto fail = [this, &path](const QString &error) {
        applyIfCurrent(this, path, [this, error]() {
            loadingFailed(error);
        });
    };

    QStringList contentList;

    // A lambda is used to avoid writing the same code twice.
    auto readArchive = [&](auto &archiveEntry) {
        struct result {
            QString error;
            QStringList content;
        };
        // opens the archive.
        if (archiveEntry.open(QIODevice::ReadOnly)) {
            // Take the root folder from the archive and create a KArchiveDirectory object.
            const KArchiveDirectory *root = archiveEntry.directory();
            // returns the list of files in the archive.
            return result{QStringLiteral(), recursiveListEntries(root)};
        } else {
            qCWarning(KIVIEW_LOG) << "Could not open the archive" << path << ":" << archiveEntry.errorString();
            return result{i18n("The archive could not be loaded."), QStringList()};
        }
    };

    // Opened from a file name, KTar decompresses the whole stream into a temporary file first, so a
    // compressed tar is read through a device that decompresses it on the fly, with a limit.
    const QMimeType type = QMimeDatabase().mimeTypeForFile(path, QMimeDatabase::MatchExtension);
    const KCompressionDevice::CompressionType compression = KCompressionDevice::compressionTypeForMimeType(type.name());
    if (archiveType(type) == ArchiveType::Tar && compression != KCompressionDevice::None) {
        KCompressionDevice decompressed(path, compression);
        if (!decompressed.open(QIODevice::ReadOnly)) {
            qCWarning(KIVIEW_LOG) << "Could not decompress" << path << ":" << decompressed.errorString();
            fail(i18n("Error: %1", QStringLiteral("The archive could not be loaded")));
            return;
        }
        BoundedReader reader(&decompressed, [this, generation]() {
            return m_generation != generation;
        });
        KTar archive(&reader);
        auto result = readArchive(archive);
        if (m_generation != generation) {
            return;
        }
        if (reader.tooBig()) {
            qCInfo(KIVIEW_LOG) << "No preview of" << path << "- it decompresses to more than" << maximumDecompressedSize << "bytes";
            fail(i18n("Preview not available: File too big"));
            return;
        }
        if (!result.error.isEmpty()) {
            fail(result.error);
            return;
        }
        contentList = result.content;
        archive.close();
    } else if (std::unique_ptr<KArchive> archive = createArchive(path)) {
        auto result = readArchive(*archive);
        if (!result.error.isEmpty()) {
            fail(result.error);
            return;
        }
        contentList = result.content;
        archive->close();
    }

    // removes empty strings added to the list.
    contentList.removeAll(QString());

    // creates an element for each string in the list
    // this element is then passed to Qml
    QList<ElementItem> elementsList;
    const QMimeDatabase mimeDatabase;
    for (const QString &name : contentList) {
        ElementItem element;
        element.path = name;
        // The list only contains files.
        element.fileType = ElementItem::File;
        setIcons(element, mimeDatabase);
        elementsList.append(element);
    }

    // sets the text in the UI
    applyIfCurrent(this, path, [this, elementsList]() {
        m_fileContentCount = QString::number(elementsList.count());
        m_viewerElements = elementsList;
        m_loadingBarVisible = false;
    });
}

QStringList ContainerViewer::recursiveListEntries(const KArchiveDirectory *dir, const QString &path)
{
    QStringList fullContentList;
    QStringList entriesList = dir->entries();
    // recursively searches for files in the archive.
    for (const QString &item : std::as_const(entriesList)) {
        const KArchiveEntry *entry = dir->entry(item);

        if (entry->isFile()) {
            fullContentList.append(path + item);
        }
        // recursively searches for files in the folder.
        if (entry->isDirectory()) {
            fullContentList += recursiveListEntries((KArchiveDirectory *)entry, path + item + QStringLiteral("/"));
        }
    }
    fullContentList.sort();
    return fullContentList;
}


void ContainerViewer::resetView()
{
    m_errorMessageText = QString();
    m_viewerElements = QList<ElementItem>();
    m_fileLastModified = QString();
    m_fileContentCount = QString();
    m_fileSize = QString();
    m_loadingBarVisible = true;
}

void ContainerViewer::loadingFailed(const QString &errorMessage)
{
    m_loadingBarVisible = false;
    m_errorMessageText = errorMessage;
}

bool ContainerViewer::isArchive(const QMimeType &type)
{
    return archiveType(type) != ArchiveType::None;
}

void ContainerViewer::clearExtractedElements()
{
    m_extractedDirs.clear();
}

quint64 ContainerViewer::prepareElement(const QString &containerPath, bool isArchive, const QString &elementPath)
{
    const quint64 request = ++m_lastRequest;
    if (!isArchive) {
        QMetaObject::invokeMethod(
            this,
            [this, request, path = QDir(containerPath).filePath(elementPath)]() {
                if (request == m_lastRequest) {
                    Q_EMIT elementReady(request, path);
                }
            },
            Qt::QueuedConnection);
        return request;
    }
    QThreadPool::globalInstance()->start([this, request, containerPath, elementPath, tempDir = m_tempDir]() {
        extractElement(request, containerPath, elementPath, tempDir);
    });
    return request;
}

QList<ElementItem> ContainerViewer::elements() const
{
    return m_viewerElements;
}

bool ContainerViewer::isShowingArchive() const
{
    return m_isArchive;
}

void ContainerViewer::extractElement(quint64 request, const QString &archivePath, const QString &entryPath, const QString &tempDir)
{
    const std::unique_ptr<KArchive> archive = createArchive(archivePath);
    if (!archive || !archive->open(QIODevice::ReadOnly)) {
        qCWarning(KIVIEW_LOG) << "Could not open" << archivePath << "to extract" << entryPath;
        return;
    }
    const KArchiveEntry *entry = archive->directory()->entry(entryPath);
    if (!entry || !entry->isFile()) {
        qCWarning(KIVIEW_LOG) << "Could not find" << entryPath << "in" << archivePath;
        return;
    }
    const auto *file = static_cast<const KArchiveFile *>(entry);
    if (file->size() > maximumEntrySize) {
        qCWarning(KIVIEW_LOG) << "No preview of" << entryPath << "in" << archivePath << "- it has" << file->size() << "bytes, more than" << maximumEntrySize;
        return;
    }

    // A directory of its own, and the name of the entry, whose extension picks the viewer.
    auto directory = std::make_shared<QTemporaryDir>(tempDir + QStringLiteral("/entry-XXXXXX"));
    const QString extractedPath = directory->filePath(QFileInfo(entryPath).fileName());
    QFile extracted(extractedPath);
    const std::unique_ptr<QIODevice> input(file->createDevice());
    if (!directory->isValid() || !extracted.open(QIODevice::WriteOnly) || !input || !input->open(QIODevice::ReadOnly)) {
        qCWarning(KIVIEW_LOG) << "Could not extract" << entryPath << "from" << archivePath << "to" << tempDir;
        return;
    }
    // The size in the archive is only what the archive says, so the copy stops at the limit too.
    qint64 copied = 0;
    while (!input->atEnd() && copied <= maximumEntrySize) {
        const QByteArray chunk = input->read(64 * 1024);
        if (chunk.isEmpty() || extracted.write(chunk) != chunk.size()) {
            break;
        }
        copied += chunk.size();
    }
    extracted.close();
    if (copied > maximumEntrySize || !input->atEnd()) {
        qCWarning(KIVIEW_LOG) << "Could not extract" << entryPath << "from" << archivePath << "- it stopped after" << copied << "bytes";
        return;
    }

    // The viewer may show another file by now, so the request number decides.
    QMetaObject::invokeMethod(
        this,
        [this, request, directory, extractedPath]() {
            if (request == m_lastRequest) {
                m_extractedDirs.append(directory);
                Q_EMIT elementReady(request, extractedPath);
            }
        },
        Qt::QueuedConnection);
}

void ContainerViewer::stopViewer()
{
    isActiveViewer = false;
    ++m_generation;
}

