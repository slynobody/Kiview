// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Méven Car <meven@kde.org>
#include "kiviewapplication.h"

#include <KLocalizedString>
#include <KStandardActions>
#include <KStandardShortcut>
#include <QAction>
#include <QLocale>

namespace
{
KiviewApplication *s_instance = nullptr;
}

KiviewApplication::KiviewApplication(QObject *parent)
    : AbstractKirigamiApplication(parent)
{
    setupActions();
    s_instance = this;
}

KiviewApplication *KiviewApplication::create(QQmlEngine *, QJSEngine *)
{
    QJSEngine::setObjectOwnership(s_instance, QJSEngine::CppOwnership);
    return s_instance;
}

void KiviewApplication::setupActions()
{
    // The actions of the base class are not set up, as Kiview has no menu or command bar to show them.
    KirigamiActionCollection *collection = mainCollection();
    const auto addAction = [this, collection](const QString &name, const QString &text, const QList<QKeySequence> &shortcuts) {
        auto *action = new QAction(text, this);
        collection->addAction(name, action);
        KirigamiActionCollection::setDefaultShortcuts(action, shortcuts);
        connect(action, &QAction::changed, this, &KiviewApplication::shortcutsChanged);
    };

    addAction(QStringLiteral("close"), i18nc("@action", "Close Kiview"), {QKeySequence(Qt::Key_Space), QKeySequence(Qt::Key_Q)});
    addAction(QStringLiteral("open_in_application"), i18nc("@action", "Open the file in its application"), {QKeySequence(Qt::Key_Return)});
    addAction(QStringLiteral("go_up"), i18nc("@action", "Go back to the folder or the archive"), {QKeySequence(Qt::ALT | Qt::Key_Up)});
    addAction(QStringLiteral("previous_page"), i18nc("@action", "Previous page"), QKeySequence::keyBindings(QKeySequence::MoveToPreviousPage));
    addAction(QStringLiteral("next_page"), i18nc("@action", "Next page"), QKeySequence::keyBindings(QKeySequence::MoveToNextPage));
    addAction(QStringLiteral("scroll_up"), i18nc("@action", "Scroll the document up"), {QKeySequence(Qt::Key_W)});
    addAction(QStringLiteral("scroll_down"), i18nc("@action", "Scroll the document down"), {QKeySequence(Qt::Key_S)});
    addAction(QStringLiteral("seek_backward"), i18nc("@action", "Move back in the track"), {QKeySequence(Qt::Key_S)});
    addAction(QStringLiteral("seek_forward"), i18nc("@action", "Move forward in the track"), {QKeySequence(Qt::Key_W)});
    addAction(QStringLiteral("skip_backward"), i18nc("@action", "Skip back 30 seconds"), {QKeySequence(Qt::CTRL | Qt::Key_Left)});
    addAction(QStringLiteral("skip_forward"), i18nc("@action", "Skip forward 30 seconds"), {QKeySequence(Qt::CTRL | Qt::Key_Right)});
    addAction(QStringLiteral("mute"), i18nc("@action", "Mute"), {QKeySequence(Qt::Key_M)});
    addAction(QStringLiteral("toggle_fullscreen"),
              i18nc("@action", "Full Screen"),
              QList<QKeySequence>{QKeySequence(Qt::Key_F)} + KStandardShortcut::fullScreen() + QList<QKeySequence>{QKeySequence(Qt::Key_F11)});
    addAction(QStringLiteral("play_pause"), i18nc("@action", "Play or pause"), {QKeySequence(Qt::Key_E), QKeySequence(Qt::Key_Escape)});
    addAction(QStringLiteral("previous_element"), i18nc("@action", "Previous element of the list"), {QKeySequence(Qt::Key_W)});
    addAction(QStringLiteral("next_element"), i18nc("@action", "Next element of the list"), {QKeySequence(Qt::Key_S)});

    QAction *keyBindings = KStandardActions::keyBindings(this, &AbstractKirigamiApplication::shortcutsEditorAction, this);
    collection->addAction(keyBindings->objectName(), keyBindings);
    connect(keyBindings, &QAction::changed, this, &KiviewApplication::shortcutsChanged);

    readSettings();
}

QVariantMap KiviewApplication::shortcuts() const
{
    QVariantMap shortcuts;
    const QList<QAction *> actions = mainCollection()->actions();
    for (const QAction *action : actions) {
        // A QVariantList, as QML makes a JavaScript array of it, which has concat().
        QVariantList keys;
        for (const QKeySequence &shortcut : action->shortcuts()) {
            keys << shortcut.toString(QKeySequence::PortableText);
        }
        shortcuts.insert(action->objectName(), keys);
    }
    return shortcuts;
}

QString KiviewApplication::keysText(const QStringList &sequences) const
{
    QStringList texts;
    for (const QString &sequence : sequences) {
        texts << QKeySequence(sequence, QKeySequence::PortableText).toString(QKeySequence::NativeText);
    }
    return QLocale().createSeparatedList(texts);
}
