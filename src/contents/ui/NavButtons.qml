// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2024 danilo agostini <kyre334@gmail.com>
import QtQuick
import QtQuick.Controls
import org.kde.kiview
import org.kde.kirigami as Kirigami

// When these buttons are activated,
// a D-Bus signal is sent to notify applications that have invoked Kiview to change the selected file.
// Without such an application, they move through the files that Kiview was started with.
Item {
    id: navButtons
    anchors.fill: parent
    anchors.leftMargin: 5
    anchors.rightMargin: 5
    visible: !autoHidden && (BusInterface.isDaemon || Manager.fileCount > 1 || Manager.canGoUp)
    // Hides the buttons with the controls of the video viewer.
    property bool autoHidden: false
    // White icons over the black background of a video.
    property bool onDarkBackground: false

    Button {
        id: buttonBack
        enabled: Manager.currentIndex > 0
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        flat: true
        icon.color: navButtons.onDarkBackground ? "white" : "transparent"
        action: Kirigami.Action {
            icon.name: "arrow-left"
            onTriggered: BusInterface.SendKeyPresses(Qt.Key_Left, 0)
        }
    }

    Button {
        id: buttonForward
        enabled: Manager.currentIndex < Manager.fileCount - 1
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        flat: true
        icon.color: navButtons.onDarkBackground ? "white" : "transparent"
        action: Kirigami.Action {
            icon.name: "arrow-right"
            onTriggered: BusInterface.SendKeyPresses(Qt.Key_Right, 0)
        }
    }
}
