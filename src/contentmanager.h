// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2024 danilo agostini <kyre334@gmail.com>
#ifndef CONTENTMANAGER_H
#define CONTENTMANAGER_H
#include "containerviewer.h"
#include <QDebug>
#include <QFileInfo>
#include <QObject>
#include <QProperty>
#include <QQmlEngine>
#include <QQuickItem>
#include <QTemporaryDir>
#include <documentviewer.h>
#include <imageviewer.h>
#include <textviewer.h>
#include <videoviewer.h>
class ContentManager : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(Manager)
    QML_SINGLETON

    Q_DISABLE_COPY(ContentManager)

    Q_PROPERTY(TextViewer *textViewer MEMBER m_textViewer CONSTANT FINAL)
    Q_PROPERTY(ImageViewer *imageViewer MEMBER m_imageViewer CONSTANT FINAL)
    Q_PROPERTY(DocumentViewer *documentViewer MEMBER m_documentViewer CONSTANT FINAL)
    Q_PROPERTY(VideoViewer *videoViewer MEMBER m_videoViewer CONSTANT FINAL)
    Q_PROPERTY(ContainerViewer *containerViewer MEMBER m_containerViewer CONSTANT FINAL)
    Q_PROPERTY(Viewer currentViewer MEMBER m_currentViewer NOTIFY currentViewerChanged FINAL)
    Q_PROPERTY(QString fileName MEMBER m_fileName NOTIFY fileNameChanged FINAL)
    Q_PROPERTY(QString windowTitle MEMBER m_windowTitle NOTIFY windowTitleChanged FINAL)
    // Number of files that the arrow keys move through.
    Q_PROPERTY(int fileCount READ fileCount NOTIFY fileCountChanged FINAL)
    // The shown file is an element of a folder or an archive, which goUp() shows again.
    Q_PROPERTY(bool canGoUp READ canGoUp NOTIFY canGoUpChanged FINAL)
    // Position of the current file in the file list, -1 for the root folder or a file outside the list.
    Q_PROPERTY(int currentIndex MEMBER m_currentIndex NOTIFY currentIndexChanged FINAL)

public:
    // The viewer that shows the current file.
    enum class Viewer {
        NoViewer,
        Text,
        Image,
        Document,
        Video,
        Container,
        Fallback
    };
    Q_ENUM(Viewer)

    // Without a default constructor, QML gets the instance from create().
    explicit ContentManager(QObject *parent);
    ~ContentManager() override;
    // Returns the instance that main() creates, which stays owned by C++.
    static ContentManager *create(QQmlEngine *, QJSEngine *);

    void loadData(const QString &rootDirectory, const QStringList &filesList, const QString &selectedFile);
    void loadFile(const QFileInfo &fileInfo);
    // Shows the element at index of the folder or the archive that the container viewer lists.
    Q_INVOKABLE void openElement(int index);
    // Shows the folder or the archive that the shown element came from.
    Q_INVOKABLE void goUp();
    bool canGoUp() const;
    // Loads the file that is offset positions away from the current one in the file list,
    // stopping at the first and the last file.
    void loadAdjacentFile(int offset);
    int fileCount() const;
    Q_INVOKABLE void openApp();
    Q_INVOKABLE void closing();

Q_SIGNALS:
    void currentViewerChanged();

    void fileNameChanged();

    void windowTitleChanged();

    void fileCountChanged();

    void canGoUpChanged();

    void currentIndexChanged();
    // Shows the application. In daemon mode the application is hidden instead of closed
    // (hidden, not minimized) and must be unhided when needed.
    void showKiview();

    // Quit application from QML: For some reason "QApplication::quit();"
    // (C++) does not trigger the Close event and so I can't reject it.
    // The same goes for "Qt.quit()" (Qml), that's why root.close() is used instead.
    void closeQml();

private:
    static inline ContentManager *s_instance = nullptr;
    TextViewer *m_textViewer = new TextViewer(this);
    DocumentViewer *m_documentViewer = new DocumentViewer(this);
    VideoViewer *m_videoViewer = new VideoViewer(this);
    ImageViewer *m_imageViewer = new ImageViewer(this);
    ContainerViewer *m_containerViewer = new ContainerViewer(this);

    QTemporaryDir tempDir;
    void stopViewers();
    // Shows a file without changing the folders and archives that goUp() returns to.
    void showFile(const QFileInfo &fileInfo);
    // A folder or an archive that an element was opened from: the arrow keys move through its
    // elements, and goUp() shows it again.
    struct Level {
        QFileInfo container;
        bool isArchive = false;
        QList<ElementItem> elements;
        int index = 0;
    };
    // The levels from the top, the one of the shown element last.
    QList<Level> m_levels;
    void setLevels(const QList<Level> &levels);
    // The element that the container viewer prepares: a new level, or another index of the last level.
    void prepareElement(const Level &level, bool newLevel);
    quint64 m_pendingRequest = 0;
    Level m_pendingLevel;
    bool m_pendingNewLevel = false;
    QFileInfo currentFileInfo;
    QFileInfo rootDirInfo;

    QStringList m_files;
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(ContentManager, Viewer, m_currentViewer, Viewer::NoViewer, &ContentManager::currentViewerChanged)
    Q_OBJECT_BINDABLE_PROPERTY(ContentManager, QString, m_fileName, &ContentManager::fileNameChanged)
    Q_OBJECT_BINDABLE_PROPERTY(ContentManager, QString, m_windowTitle, &ContentManager::windowTitleChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(ContentManager, int, m_currentIndex, -1, &ContentManager::currentIndexChanged)
};

#endif // CONTENTMANAGER_H
