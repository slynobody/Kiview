// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2024 danilo agostini <kyre334@gmail.com>
import QtQuick
import QtQuick.Controls
import org.kde.kiview
import org.kde.kirigami as Kirigami
import org.kde.ki18n
import org.kde.syntaxhighlighting

Item {
    anchors.fill: parent
    // Property to enable/disable unloading of the viewer when not in use.
    property bool isPersistent: false
    property list<Kirigami.Action> actions
    //these actions are read by viewersLoader and are shown in the "header bar" of the application.
    actions: [
        OpenAppButton {}
    ]

    Kirigami.InlineMessage {
        id: truncatedMessage
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        visible: Manager.textViewer.viewerTextTruncated
        type: Kirigami.MessageType.Information
        text: KI18n.i18nc("@info", "Only the beginning of this file is shown.")
    }

    ScrollView {
        anchors.top: truncatedMessage.visible ? truncatedMessage.bottom : parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom

        // When a key is pressed, return focus to the parent: This is done because focusPolicy,etc,
        // are not respected when opening the context menu (right-click menu).
        Keys.onPressed: event => {
            parent.focus = true;
        }
        TextArea {
            id: textArea
            // Avoids taking focus: The arrow keys are used to scroll through files
            // and TextArea uses the same keys to navigate through the text.
            // This causes conflicts and makes scrolling through files not work.
            activeFocusOnPress: false
            activeFocusOnTab: false
            focusPolicy: Qt.NoFocus
            text: Manager.textViewer.viewerText
            readOnly: true
            font: Kirigami.Theme.fixedWidthFont
        }

        SyntaxHighlighter {
            textEdit: textArea
            definition: Repository.definitionForFileName(Manager.fileName)
            // The default theme that matches the brightness of the color scheme.
            theme: Repository.defaultTheme(Kirigami.ColorUtils.brightnessForColor(Kirigami.Theme.backgroundColor) === Kirigami.ColorUtils.Dark ? Repository.DarkTheme : Repository.LightTheme)
        }
    }
}
