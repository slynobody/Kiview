// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2024 danilo agostini <kyre334@gmail.com>
#ifndef CONTAINERVIEWER_H
#define CONTAINERVIEWER_H

#include <KArchiveDirectory>
#include <QDir>
#include <QFileInfo>
#include <QMimeType>
#include <QObject>
#include <QProperty>
#include <QTemporaryDir>
#include <QtQmlIntegration/qqmlintegration.h>
#include <atomic>
#include <memory>

class ElementItem
{
    // Gadget class to pass file information to Qml more easily.
    Q_GADGET
    QML_VALUE_TYPE(elementItem)
    Q_PROPERTY(QString path MEMBER path)
    Q_PROPERTY(FileTypes fileType MEMBER fileType)
    Q_PROPERTY(QString iconName MEMBER iconName)
    Q_PROPERTY(QString genericIconName MEMBER genericIconName)
public:
    enum FileTypes { File, Dir, Symlink };
    Q_ENUM(FileTypes)

    bool operator==(const ElementItem &other) const = default;

    QString path;
    FileTypes fileType = File;
    // The icon of the type of the element, and the icon to show when the theme has no such icon.
    QString iconName;
    QString genericIconName;
};
Q_DECLARE_METATYPE(ElementItem)

// Gives QML access to the enums of ElementItem, as ElementType.Dir for example.
namespace ElementItemForeign
{
Q_NAMESPACE
QML_NAMED_ELEMENT(ElementType)
QML_FOREIGN_NAMESPACE(ElementItem)
}

class ContainerViewer : public QObject
{
    Q_OBJECT
    QML_ANONYMOUS
    Q_PROPERTY(QList<ElementItem> viewerElements MEMBER m_viewerElements NOTIFY viewerElementsChanged FINAL)
    Q_PROPERTY(QString fileIcon MEMBER m_fileIcon NOTIFY fileIconChanged FINAL)
    Q_PROPERTY(QString fileSize MEMBER m_fileSize NOTIFY fileSizeChanged FINAL)
    Q_PROPERTY(QString fileContentCount MEMBER m_fileContentCount NOTIFY fileContentCountChanged FINAL)
    Q_PROPERTY(QString fileLastModified MEMBER m_fileLastModified NOTIFY fileLastModifiedChanged FINAL)
    Q_PROPERTY(QString errorMessageText MEMBER m_errorMessageText NOTIFY errorMessageTextChanged FINAL)
    Q_PROPERTY(bool loadingBarVisible MEMBER m_loadingBarVisible NOTIFY loadingBarVisibleChanged FINAL)

public:
    explicit ContainerViewer(QObject *parent = nullptr);
    void loadFile(const QFileInfo &fileInfo, bool isArchive, const QString &tempDir = QString());
    // Prepares the element at elementPath of the folder or the archive at containerPath for a
    // preview, by extracting an archive entry first. Returns the number of the request, which
    // elementReady() gives back. Only the last request is answered.
    quint64 prepareElement(const QString &containerPath, bool isArchive, const QString &elementPath);
    // The elements that the viewer lists, and whether they are the entries of an archive.
    QList<ElementItem> elements() const;
    bool isShowingArchive() const;
    // Whether type is a type of archive that this viewer lists.
    static bool isArchive(const QMimeType &type);
    // Removes the extracted archive entries, which nothing shows any more.
    void clearExtractedElements();
    void loadDir(const QString &directoryPath);
    void loadArchive(const QFileInfo &fileInfo, quint64 generation);
    bool isActiveViewer = false;
    // Changes whenever the viewer leaves the archive it lists, which stops that listing.
    std::atomic<quint64> m_generation = 0;
    void stopViewer();

    QString currentFilePath;

    // The largest archive entry that is extracted for a preview, in bytes.
    static constexpr qint64 maximumEntrySize = 256 * 1024 * 1024;

Q_SIGNALS:
    // The element of request is ready for a preview at path.
    void elementReady(quint64 request, const QString &path);

    void fileIconChanged();

    void fileSizeChanged();

    void fileContentCountChanged();

    void fileLastModifiedChanged();

    void errorMessageTextChanged();

    void viewerElementsChanged();

    void loadingBarVisibleChanged();

private:
    void extractElement(quint64 request, const QString &archivePath, const QString &entryPath, const QString &tempDir);
    quint64 m_lastRequest = 0;
    bool m_isArchive = false;
    QString m_tempDir;
    // The directories of the extracted archive entries, kept while they may be shown again.
    QList<std::shared_ptr<QTemporaryDir>> m_extractedDirs;
    QStringList recursiveListEntries(const KArchiveDirectory *dir, const QString &path = QString());
    void resetView();
    void loadingFailed(const QString &errorMessage);
    Q_OBJECT_BINDABLE_PROPERTY(ContainerViewer, QString, m_fileIcon, &ContainerViewer::fileIconChanged)
    Q_OBJECT_BINDABLE_PROPERTY(ContainerViewer, QString, m_fileSize, &ContainerViewer::fileSizeChanged)
    Q_OBJECT_BINDABLE_PROPERTY(ContainerViewer, QString, m_fileContentCount, &ContainerViewer::fileContentCountChanged)
    Q_OBJECT_BINDABLE_PROPERTY(ContainerViewer, QString, m_fileLastModified, &ContainerViewer::fileLastModifiedChanged)
    Q_OBJECT_BINDABLE_PROPERTY(ContainerViewer, QString, m_errorMessageText, &ContainerViewer::errorMessageTextChanged)
    Q_OBJECT_BINDABLE_PROPERTY(ContainerViewer, QList<ElementItem>, m_viewerElements, &ContainerViewer::viewerElementsChanged)
    Q_OBJECT_BINDABLE_PROPERTY(ContainerViewer, bool, m_loadingBarVisible, &ContainerViewer::loadingBarVisibleChanged)
};

#endif // CONTAINERVIEWER_H
