// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2024 danilo agostini <kyre334@gmail.com>
#include "imageviewer.h"
#include "guithread.h"
#include "kiview_debug.h"
#include <QUrl>
#include <klocalizedstring.h>

// Kde
#include <KZip>

// Qt
#include <QDebug>
#include <QFile>
#include <QImageReader>
#include <QMimeDatabase>
#include <QTemporaryFile>
#include <QThreadPool>

#include <memory>

ImageViewer::ImageViewer(QObject *parent)
    : QObject{parent}
{
}

void ImageViewer::loadFile(const QFileInfo &fileInfo, const QString &tempDir)
{
    m_errorMessageText = QString();
    m_viewerImage = QUrl();
    m_viewerImageAnimated = false;
    // it is used to discard the thread if not longer needed
    isActiveViewer = true;
    currentFilePath = fileInfo.absoluteFilePath();
    // A Krita file shows the preview that it holds.
    if (QMimeDatabase().mimeTypeForFile(fileInfo).inherits(QStringLiteral("application/x-krita"))) {
        // Starts the function in another thread to avoid slowing down the interface.
        QThreadPool::globalInstance()->start([this, path = currentFilePath, tempDir]() {
            extractPreview(path, tempDir);
        });
    } else {
        // Reads the header only, before the URL changes, so that QML shows the right item.
        m_viewerImageAnimated = QImageReader(currentFilePath).supportsAnimation();
        m_viewerImage = QUrl::fromLocalFile(currentFilePath);
    }
}

void ImageViewer::extractPreview(QString path, QString outputDir)
{
    // opens the .kra file (zip archive) to extract the preview inside.
    KZip archive(path);
    // Open the archive
    if (!archive.open(QIODevice::ReadOnly)) {
        qCWarning(KIVIEW_LOG) << "Could not open" << path << "as a Krita archive:" << archive.errorString();
        applyIfCurrent(this, path, [this]() {
            m_errorMessageText = i18n("The file failed to load");
        });
        return;
    }

    // Take the root folder from the archive and create a KArchiveDirectory object.
    const KArchiveDirectory *root = archive.directory();

    // extracts the image.
    const KArchiveFile *preview = root->file(QStringLiteral("mergedimage.png"));
    if (!preview) {
        qCWarning(KIVIEW_LOG) << "Could not show" << path << "- the Krita archive has no mergedimage.png";
        applyIfCurrent(this, path, [this]() {
            m_errorMessageText = i18n("The file failed to load");
        });
        return;
    }
    auto fail = [this, &path]() {
        applyIfCurrent(this, path, [this]() {
            m_errorMessageText = i18n("The file failed to load");
        });
    };
    if (preview->size() > maximumPreviewSize) {
        qCWarning(KIVIEW_LOG) << "Could not show" << path << "- its preview has" << preview->size() << "bytes, more than" << maximumPreviewSize;
        fail();
        return;
    }

    // A file of its own for each preview: the image cache of QML holds an image by its URL, and two
    // extractions can run at the same time.
    QTemporaryFile extracted(outputDir + QStringLiteral("/krita-preview-XXXXXX.png"));
    extracted.setAutoRemove(false);
    const std::unique_ptr<QIODevice> input(preview->createDevice());
    if (!extracted.open() || !input || !input->open(QIODevice::ReadOnly)) {
        qCWarning(KIVIEW_LOG) << "Could not extract the preview of" << path << "to" << outputDir << ":" << extracted.errorString();
        extracted.remove();
        fail();
        return;
    }
    // The size in the archive is only what the archive says, so the copy stops at the limit too.
    qint64 copied = 0;
    while (!input->atEnd() && copied <= maximumPreviewSize) {
        const QByteArray chunk = input->read(64 * 1024);
        if (chunk.isEmpty() || extracted.write(chunk) != chunk.size()) {
            break;
        }
        copied += chunk.size();
    }
    if (copied > maximumPreviewSize || !input->atEnd()) {
        qCWarning(KIVIEW_LOG) << "Could not extract the preview of" << path << "- it stopped after" << copied << "bytes";
        extracted.remove();
        fail();
        return;
    }
    extracted.close();
    archive.close();

    applyIfCurrent(this, path, [this, extractedFile = extracted.fileName()]() {
        if (!m_previewFile.isEmpty()) {
            QFile::remove(m_previewFile);
        }
        m_previewFile = extractedFile;
        m_viewerImage = QUrl::fromLocalFile(extractedFile);
    });
}

void ImageViewer::stopViewer()
{
    isActiveViewer = false;
}

