// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2024 danilo agostini <kyre334@gmail.com>
import QtQuick
import org.kde.kiview
import org.kde.kirigami as Kirigami

Item {
    id: imageViewer
    anchors.fill: parent
    // Property to enable/disable unloading of the viewer when not in use.
    property bool isPersistent: false
    property list<Kirigami.Action> actions
    //these actions are read by viewersLoader and are shown in the "header bar" of the application.
    actions: [
        OpenAppButton {}
    ]

    Image {
        id: imageArea
        anchors.fill: parent
        visible: !Manager.imageViewer.viewerImageAnimated
        source: visible ? Manager.imageViewer.viewerImage : ""
        fillMode: Image.PreserveAspectFit
        asynchronous: true
        sourceSize: Qt.size(Screen.width * Screen.devicePixelRatio, Screen.height * Screen.devicePixelRatio)
        // implicitWidth and implicitHeight are the size of the decoded image.
        smooth: (imageArea.implicitHeight > 256 && imageArea.implicitWidth > 256)
    }

    // AnimatedImage decodes each frame at its full size, so it only shows the formats that animate.
    AnimatedImage {
        id: animatedImageArea
        anchors.fill: parent
        visible: Manager.imageViewer.viewerImageAnimated
        source: visible ? Manager.imageViewer.viewerImage : ""
        fillMode: Image.PreserveAspectFit
        // The frames of a long animation stay out of the image cache.
        cache: false
        smooth: (animatedImageArea.implicitHeight > 256 && animatedImageArea.implicitWidth > 256)
    }

    Text {
        id: errorMessage
        anchors.verticalCenter: parent.verticalCenter
        anchors.horizontalCenter: parent.horizontalCenter
        visible: (Manager.imageViewer.errorMessageText === "" ? false : true)
        text: Manager.imageViewer.errorMessageText
        color: Kirigami.Theme.textColor
        font.pointSize: 15
    }
}
