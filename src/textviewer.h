// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2024 danilo agostini <kyre334@gmail.com>
#ifndef TEXTVIEWER_H
#define TEXTVIEWER_H
#include <QFileInfo>
#include <QObject>
#include <QProperty>
#include <QtQmlIntegration/qqmlintegration.h>
class TextViewer : public QObject
{
    Q_OBJECT
    QML_ANONYMOUS
    Q_PROPERTY(QString viewerText MEMBER m_viewerText NOTIFY viewerTextChanged FINAL)
    // The file is longer than maximumSize, and viewerText holds its beginning only.
    Q_PROPERTY(bool viewerTextTruncated MEMBER m_viewerTextTruncated NOTIFY viewerTextTruncatedChanged FINAL)
public:
    explicit TextViewer(QObject *parent = nullptr);

    // The number of bytes of a file that are shown.
    static constexpr qint64 maximumSize = 256 * 1024;

    void loadFile(const QFileInfo &fileInfo);
    void stopViewer();
    // it is used to discard the thread if not longer needed
    bool isActiveViewer = false;
    QString currentFilePath;

Q_SIGNALS:
    void viewerTextChanged();
    void viewerTextTruncatedChanged();

private:
    Q_OBJECT_BINDABLE_PROPERTY(TextViewer, QString, m_viewerText, &TextViewer::viewerTextChanged)
    Q_OBJECT_BINDABLE_PROPERTY(TextViewer, bool, m_viewerTextTruncated, &TextViewer::viewerTextTruncatedChanged)
};

#endif // TEXTVIEWER_H
