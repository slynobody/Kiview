// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2024 danilo agostini <kyre334@gmail.com>
#ifndef VIDEOVIEWER_H
#define VIDEOVIEWER_H

#include <QFileInfo>
#include <QObject>
#include <QProperty>
#include <QUrl>
#include <QtQmlIntegration/qqmlintegration.h>

class VideoViewer : public QObject
{
    Q_OBJECT
    QML_ANONYMOUS
    Q_PROPERTY(QUrl viewerMedia MEMBER m_viewerMedia NOTIFY viewerMediaChanged FINAL)

public:
    explicit VideoViewer(QObject *parent = nullptr);
    void loadFile(const QFileInfo &fileInfo);

Q_SIGNALS:
    void viewerMediaChanged();

private:
    Q_OBJECT_BINDABLE_PROPERTY(VideoViewer, QUrl, m_viewerMedia, &VideoViewer::viewerMediaChanged)
};

#endif // VIDEOVIEWER_H
