// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2024 danilo agostini <kyre334@gmail.com>
pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigami.delegates as KD
import org.kde.kiview
import org.kde.ki18n

Item {
    id: containerViewer
    anchors.fill: parent
    // Property to enable/disable unloading of the viewer when not in use.
    property bool isPersistent: false
    property list<Kirigami.Action> actions
    //these actions are read by viewersLoader and are shown in the "header bar" of the application.
    actions: [
        OpenAppButton {}
    ]

    Item {
        id: infoPanel
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: parent.width / 4

        Rectangle {
            id: infoPanelBackground
            anchors.fill: parent
            color: Kirigami.Theme.alternateBackgroundColor
        }

        Kirigami.Icon {
            id: fileIcon
            height: 200
            width: parent.width
            source: Manager.containerViewer.fileIcon
            anchors.top: parent.top
            anchors.topMargin: parent.height / 15
        }

        Column {
            id: infoArea
            anchors.top: fileIcon.bottom
            width: parent.width
            property int fontSizeText: 13
            property int fontSizeValue: 12
            property int separation: 15

            Column {
                id: sizeArea
                visible: (Manager.containerViewer.fileSize === "" ? false : true)
                width: parent.width
                topPadding: infoArea.separation

                Text {
                    id: sizeText
                    text: KI18n.i18n("Size")
                    font.pointSize: infoArea.fontSizeText
                    color: Kirigami.Theme.textColor
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    width: parent.width
                    font.bold: true
                }

                Text {
                    id: sizeValue
                    text: Manager.containerViewer.fileSize
                    font.pointSize: infoArea.fontSizeValue
                    color: Kirigami.Theme.textColor
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    width: parent.width
                    font.underline: true
                }
            }

            Column {
                id: lastModifiedArea
                visible: (Manager.containerViewer.fileLastModified === "" ? false : true)
                width: parent.width
                topPadding: infoArea.separation
                Text {
                    id: lastModifiedText
                    text: KI18n.i18n("Last Modified")
                    font.pointSize: infoArea.fontSizeText
                    color: Kirigami.Theme.textColor
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    width: parent.width
                    font.bold: true
                }

                Text {
                    id: lastModifiedValue
                    text: Manager.containerViewer.fileLastModified
                    font.pointSize: infoArea.fontSizeValue
                    color: Kirigami.Theme.textColor
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    width: parent.width
                    font.underline: true
                }
            }
            Column {
                id: fileCountArea
                visible: (Manager.containerViewer.fileContentCount === "" ? false : true)
                width: parent.width
                topPadding: infoArea.separation
                Text {
                    id: countText
                    text: KI18n.i18n("File Count")
                    font.pointSize: infoArea.fontSizeText
                    color: Kirigami.Theme.textColor
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    width: parent.width
                    font.bold: true
                }

                Text {
                    id: countValue
                    text: Manager.containerViewer.fileContentCount
                    font.pointSize: infoArea.fontSizeValue
                    color: Kirigami.Theme.textColor
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    width: parent.width
                    font.underline: true
                }
            }
        }
    }

    Kirigami.Separator {
        id: lineSeparator
        anchors.left: infoPanel.right
        anchors.bottom: parent.bottom
        anchors.top: parent.top
    }

    Rectangle {
        id: listBackground
        anchors.fill: contentList
        color: Kirigami.Theme.backgroundColor
        anchors.topMargin: 0
    }

    ListView {
        id: contentList
        anchors.left: lineSeparator.right
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        model: Manager.containerViewer.viewerElements

        // To avoid conflicts, shortcuts must be disabled when the viewer is not visible
        Shortcut {
            enabled: contentList.visible
            sequences: KiviewApp.shortcuts.previous_element
            onActivated: if (contentList.currentIndex > 0)
                contentList.currentIndex = contentList.currentIndex - 1
        }
        Shortcut {
            enabled: contentList.visible
            sequences: KiviewApp.shortcuts.next_element
            onActivated: if (contentList.count > contentList.currentIndex + 1)
                contentList.currentIndex = contentList.currentIndex + 1
        }

        delegate: ItemDelegate {
            id: elementDelegate
            required property elementItem modelData
            required property int index
            text: modelData.path
            icon.name: modelData.iconName
            width: contentList.width
            highlighted: contentList.currentIndex === index
            // Activation follows the setting of the system, for a single or a double click.
            onClicked: {
                contentList.currentIndex = index;
                if (Qt.styleHints.singleClickActivation) {
                    Manager.openElement(index);
                }
            }
            onDoubleClicked: {
                if (!Qt.styleHints.singleClickActivation) {
                    Manager.openElement(index);
                }
            }

            // Shows the entry name as plain text, as it is written in the archive.
            contentItem: RowLayout {
                spacing: Kirigami.Units.smallSpacing

                Kirigami.Icon {
                    source: elementDelegate.icon.name
                    fallback: elementDelegate.modelData.genericIconName
                    selected: elementDelegate.highlighted || elementDelegate.down
                    Layout.preferredWidth: elementDelegate.icon.width
                    Layout.preferredHeight: elementDelegate.icon.height
                }

                KD.TitleSubtitle {
                    title: elementDelegate.text
                    textFormat: Text.PlainText
                    selected: elementDelegate.highlighted || elementDelegate.down
                    Layout.fillWidth: true
                }
            }

            Kirigami.Icon {
                id: symlinkIcon
                visible: elementDelegate.modelData.fileType === ElementType.Symlink
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                anchors.margins: 4
                source: "link"
                rotation: 45
            }
        }

        Text {
            id: errorMessage
            anchors.verticalCenter: parent.verticalCenter
            anchors.horizontalCenter: parent.horizontalCenter
            wrapMode: Text.WrapAnywhere
            width: parent.width * 0.9
            visible: (Manager.containerViewer.errorMessageText === "" ? false : true)
            text: Manager.containerViewer.errorMessageText
            color: Kirigami.Theme.textColor
            font.pointSize: 15
            horizontalAlignment: Text.AlignHCenter
        }

        Kirigami.LoadingPlaceholder {
            id: loadingIndicator
            anchors.verticalCenter: parent.verticalCenter
            anchors.horizontalCenter: parent.horizontalCenter
            visible: Manager.containerViewer.loadingBarVisible
        }
    }
}
