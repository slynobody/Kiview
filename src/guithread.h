// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Méven Car <meven@kde.org>
#ifndef GUITHREAD_H
#define GUITHREAD_H
#include <QMetaObject>
#include <QString>

/**
 * Runs @p apply in the thread of @p viewer, unless the viewer is no longer showing @p path by then.
 * A worker in the thread pool hands its result over this way, because QML reads the properties of
 * the viewer from the GUI thread.
 */
template<typename Viewer, typename Function>
void applyIfCurrent(Viewer *viewer, const QString &path, Function apply)
{
    QMetaObject::invokeMethod(
        viewer,
        [viewer, path, apply = std::move(apply)]() {
            if (viewer->isActiveViewer && viewer->currentFilePath == path) {
                apply();
            }
        },
        Qt::QueuedConnection);
}

#endif // GUITHREAD_H
