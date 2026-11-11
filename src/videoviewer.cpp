// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2024 danilo agostini <kyre334@gmail.com>
#include "videoviewer.h"
#include <QUrl>

VideoViewer::VideoViewer(QObject *parent)
    : QObject{parent}
{
}

void VideoViewer::loadFile(const QFileInfo &fileInfo)
{
    m_viewerMedia = QUrl::fromLocalFile(fileInfo.absoluteFilePath());
}

