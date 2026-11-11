// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2024 danilo agostini <kyre334@gmail.com>
import QtQuick
import org.kde.kirigami as Kirigami
import org.kde.ki18n

Item {
    id: fallbackPage
    anchors.fill: parent
    // Property to enable/disable unloading of the viewer when not in use.
    property bool isPersistent: false
    property list<Kirigami.Action> actions
    //these actions are read by viewersLoader and are shown in the "header bar" of the application.
    actions: [
        OpenAppButton {}
    ]

    Kirigami.Icon {
        id: messageIcon
        height: parent.height / 3
        width: parent.width / 3
        source: "unknown"
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: parent.height / 7
    }

    Text {
        id: headerText
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: messageIcon.bottom
        text: KI18n.i18n("File type not supported.")
        color: Kirigami.Theme.textColor
        font.bold: true
        font.pointSize: 13
    }
}
