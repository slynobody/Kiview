// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2024 Danilo Agostini <kyre334@gmail.com>
import QtQuick
import org.kde.kiview
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.ki18n

// The text of the info dialog: the keyboard shortcuts and the links.
Column {
    id: content

    readonly property int headerTextSize: 14
    readonly property int shortcutTextSize: 12

    // One line of the list: the keys of an action, from KiviewApp, and what the action does.
    component ShortcutLine: Text {
        color: Kirigami.Theme.textColor
        font.pointSize: content.shortcutTextSize
        horizontalAlignment: Text.AlignHCenter
        width: content.width
        wrapMode: Text.Wrap
    }

    Text {
        id: shortcutsText
        text: KI18n.i18n("Shortcuts")
        color: Kirigami.Theme.textColor
        font.pointSize: content.headerTextSize + 2
        horizontalAlignment: Text.AlignHCenter
        width: content.width
        bottomPadding: 5
        font.bold: true
    }

    Text {
        id: generalText
        text: KI18n.i18n("General")
        color: Kirigami.Theme.textColor
        font.pointSize: content.headerTextSize
        horizontalAlignment: Text.AlignHCenter
        width: content.width
        padding: 5
        font.underline: true
    }
    ShortcutLine {
        text: KI18n.i18nc("@info %1 is a list of keys", "%1: Move between files.", KiviewApp.keysText(["Left", "Right"]))
    }
    ShortcutLine {
        text: KI18n.i18nc("@info %1 is a list of keys", "%1: Open the file in its application.", KiviewApp.keysText(KiviewApp.shortcuts.open_in_application))
    }
    ShortcutLine {
        text: KI18n.i18nc("@info %1 is a list of keys", "%1: Go back to the folder or the archive.", KiviewApp.keysText(KiviewApp.shortcuts.go_up))
    }
    ShortcutLine {
        text: KI18n.i18nc("@info %1 is a list of keys", "%1: Close Kiview.", KiviewApp.keysText(KiviewApp.shortcuts.close))
    }
    Text {
        id: documentText
        text: KI18n.i18n("Documents")
        color: Kirigami.Theme.textColor
        font.pointSize: content.headerTextSize
        horizontalAlignment: Text.AlignHCenter
        width: content.width
        padding: 5
        font.underline: true
    }
    ShortcutLine {
        text: KI18n.i18nc("@info %1 is a list of keys", "%1: Go to the previous or the next page.", KiviewApp.keysText(KiviewApp.shortcuts.previous_page.concat(KiviewApp.shortcuts.next_page)))
    }
    ShortcutLine {
        text: KI18n.i18nc("@info %1 is a list of keys", "%1: Scroll the document.", KiviewApp.keysText(KiviewApp.shortcuts.scroll_up.concat(KiviewApp.shortcuts.scroll_down)))
    }
    Text {
        id: videoplayerText
        text: KI18n.i18n("Video player")
        color: Kirigami.Theme.textColor
        font.pointSize: content.headerTextSize
        horizontalAlignment: Text.AlignHCenter
        width: content.width
        padding: 5
        font.underline: true
    }
    ShortcutLine {
        text: KI18n.i18nc("@info %1 is a list of keys", "%1: Move back or forward in the track.", KiviewApp.keysText(KiviewApp.shortcuts.seek_backward.concat(KiviewApp.shortcuts.seek_forward)))
    }
    ShortcutLine {
        text: KI18n.i18nc("@info %1 is a list of keys", "%1: Play or pause.", KiviewApp.keysText(KiviewApp.shortcuts.play_pause))
    }
    Text {
        id: archiveText
        text: KI18n.i18n("Folders and archives")
        color: Kirigami.Theme.textColor
        font.pointSize: content.headerTextSize
        horizontalAlignment: Text.AlignHCenter
        width: content.width
        padding: 5
        font.underline: true
    }
    ShortcutLine {
        text: KI18n.i18nc("@info %1 is a list of keys", "%1: Move through the list.", KiviewApp.keysText(KiviewApp.shortcuts.previous_element.concat(KiviewApp.shortcuts.next_element)))
    }

    RowLayout {
        spacing: 50
        anchors.horizontalCenter: parent.horizontalCenter

        Text {
            id: infoText
            topPadding: 10
            text: "<a href='https://github.com/Nyre221/Kiview'>" + KI18n.i18n("How it works") + "</a>"
            color: Kirigami.Theme.textColor
            font.pointSize: content.headerTextSize
            horizontalAlignment: Text.AlignRight

            font.underline: true
            linkColor: Kirigami.Theme.linkColor
            onLinkActivated: link => Qt.openUrlExternally(link)
        }

        Text {
            id: donateText
            topPadding: 10
            text: "<a href='https://www.paypal.com/donate/?hosted_button_id=J7QU55MMUP4G4'>" + KI18n.i18n("Support me") + "</a>"
            color: Kirigami.Theme.textColor
            font.pointSize: content.headerTextSize
            horizontalAlignment: Text.AlignHCenter
            font.underline: true
            linkColor: Kirigami.Theme.linkColor
            onLinkActivated: link => Qt.openUrlExternally(link)
        }
    }
}
