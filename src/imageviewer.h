// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2024 danilo agostini <kyre334@gmail.com>
#ifndef IMAGEVIEWER_H
#define IMAGEVIEWER_H
#include <QFileInfo>
#include <QObject>
#include <QProperty>
#include <QUrl>
#include <QtQmlIntegration/qqmlintegration.h>
class ImageViewer : public QObject
{
    Q_OBJECT
    QML_ANONYMOUS
    Q_PROPERTY(QUrl viewerImage MEMBER m_viewerImage NOTIFY viewerImageChanged FINAL)
    // The format of viewerImage supports animation, like GIF or animated WebP.
    Q_PROPERTY(bool viewerImageAnimated MEMBER m_viewerImageAnimated NOTIFY viewerImageAnimatedChanged FINAL)
    Q_PROPERTY(QString errorMessageText MEMBER m_errorMessageText NOTIFY errorMessageTextChanged FINAL)

public:
    explicit ImageViewer(QObject *parent = nullptr);
    void loadFile(const QFileInfo &fileInfo, const QString &tempDir = QString());
    QString currentFilePath;
    bool isActiveViewer = false;
    void stopViewer();

    void extractPreview(QString path, QString outputDir);

    // The largest Krita preview that is extracted, in bytes.
    static constexpr qint64 maximumPreviewSize = 256 * 1024 * 1024;

Q_SIGNALS:
    void viewerImageChanged();
    void viewerImageAnimatedChanged();

    void errorMessageTextChanged();

private:
    // The extracted Krita preview that is shown, removed when the next one is shown.
    QString m_previewFile;
    Q_OBJECT_BINDABLE_PROPERTY(ImageViewer, QUrl, m_viewerImage, &ImageViewer::viewerImageChanged)
    Q_OBJECT_BINDABLE_PROPERTY(ImageViewer, bool, m_viewerImageAnimated, &ImageViewer::viewerImageAnimatedChanged)
    Q_OBJECT_BINDABLE_PROPERTY(ImageViewer, QString, m_errorMessageText, &ImageViewer::errorMessageTextChanged)
};

#endif // IMAGEVIEWER_H
