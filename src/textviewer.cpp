// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2024 danilo agostini <kyre334@gmail.com>
#include "textviewer.h"
#include "guithread.h"
#include "kiview_debug.h"

// Qt
#include <QDebug>
#include <QFile>
#include <QString>
#include <QStringDecoder>
#include <QThreadPool>

namespace
{
// Decodes the beginning of a text file: with the encoding of its byte order mark, as UTF-8 otherwise,
// and as Latin-1 when the bytes are not valid UTF-8.
QString decodeText(const QByteArray &data)
{
    if (const auto encoding = QStringConverter::encodingForData(data)) {
        return QStringDecoder(*encoding)(data);
    }
    // The decoder keeps a character that the size limit cut as its state, which is not an error.
    QStringDecoder utf8(QStringConverter::Utf8);
    QString text = utf8(data);
    if (utf8.hasError()) {
        text = QString::fromLatin1(data);
    }
    return text;
}
}

TextViewer::TextViewer(QObject *parent)
    : QObject{parent}
{
}

void TextViewer::loadFile(const QFileInfo &fileInfo)
{
    isActiveViewer = true;
    currentFilePath = fileInfo.absoluteFilePath();
    m_viewerText = QString();
    m_viewerTextTruncated = false;

    // Reads the file in another thread, so that a slow or large file does not freeze the interface.
    QThreadPool::globalInstance()->start([this, path = currentFilePath]() {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) {
            qCWarning(KIVIEW_LOG) << "Could not open" << path << "as text:" << file.errorString();
            return;
        }
        QByteArray data = file.read(maximumSize + 1);
        const bool truncated = data.size() > maximumSize;
        if (truncated) {
            data.truncate(maximumSize);
        }
        const QString text = decodeText(data);
        applyIfCurrent(this, path, [this, text, truncated]() {
            m_viewerText = text;
            m_viewerTextTruncated = truncated;
        });
    });
}

void TextViewer::stopViewer()
{
    isActiveViewer = false;
}
