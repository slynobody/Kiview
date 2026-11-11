// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2024 Danilo Agostini <kyre334@gmail.com>
import QtQuick
import org.kde.kiview
import org.kde.kirigami as Kirigami
import org.kde.ki18n

Kirigami.Dialog {
    id: shortcutsDialog
    title: KI18n.i18n("Info")
    width: parent.width * 0.75
    height: parent.height * 0.8
    padding: 10

    customFooterActions: [
        Kirigami.Action {
            icon.name: "configure-shortcuts"
            text: KI18n.i18nc("@action:button", "Configure Keyboard Shortcuts…")
            onTriggered: {
                shortcutsDialog.close();
                KiviewApp.shortcutsEditorAction();
            }
        }
    ]

    InfoDialogContent {
        width: shortcutsDialog.width
    }

    Shortcut {
        enabled: shortcutsDialog.visible
        sequence: "Space"
        onActivated: shortcutsDialog.close()
    }
    // The dialog is modal, so the shortcuts of the window do not reach Main.qml while it is open.
    Shortcut {
        enabled: shortcutsDialog.visible
        sequences: KiviewApp.shortcuts.close
        onActivated: {
            shortcutsDialog.close();
            shortcutsDialog.parent.Window.window.close();
        }
    }
}
