// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2024 danilo agostini <kyre334@gmail.com>
pragma ComponentBehavior: Bound

import QtCore
import QtQuick
import QtQuick.Pdf
import QtQuick.Controls
import org.kde.kiview
import org.kde.kirigami as Kirigami
import org.kde.ki18n

Item {
    id: documentViewer
    anchors.fill: parent
    // Property to enable/disable unloading of the viewer when not in use.
    // Avoids unloading DocumentViewer since it tends to crash if continuously created and destroyed.
    // This property is also used to prevent DocumentViewer from being unloaded after being preloaded.
    property bool isPersistent: true
    Settings {
        id: settings
        property int thumbnailsLayoutType: DocumentViewerUi.ThumbnailsLayoutTypes.Grid
        property int currentViewMode: PdfDocumentView.ViewModes.Automatic
    }
    enum ThumbnailsLayoutTypes {
        List,
        Grid
    }
    property list<Kirigami.Action> actions
    //these actions are read by viewersLoader and are shown in the "header bar" of the application.
    actions: [
        Kirigami.Action {
            id: viewModeButton
            Component.onCompleted: viewModeButton.updateButtonState()
            // The number of values in the enum.
            // This is done to cycle through all the values and keep the code simple.
            // The number is static because in Qml there is no way of knowing the size/length of an enum.
            property int enumLenght: 3
            onTriggered: {
                docView.currentViewMode = (docView.currentViewMode + 1) % viewModeButton.enumLenght;
                viewModeButton.updateButtonState();
            }
            // updates icon,tooltips,etc
            function updateButtonState(): void {
                switch (docView.currentViewMode) {
                case PdfDocumentView.ViewModes.Automatic:
                    viewModeButton.icon.name = "view-fullscreen";
                    viewModeButton.tooltip = KI18n.i18nc("DocumentViewer automatic view mode (fit to width/height)", "Automatic mode");
                    break;
                case PdfDocumentView.ViewModes.FitToWidth:
                    viewModeButton.icon.name = "object-width-symbolic";
                    viewModeButton.tooltip = KI18n.i18nc("DocumentViewer fit to width view mode", "Fit to width");
                    break;
                case PdfDocumentView.ViewModes.FitToHeight:
                    viewModeButton.icon.name = "object-height-symbolic";
                    viewModeButton.tooltip = KI18n.i18nc("DocumentViewer fit to height view mode", "Fit to height");
                    break;
                }
            }
        },
        OpenAppButton {}
    ]
    property string htmlLink
    htmlLink: "<a href='https://invent.kde.org/danagost/Kiview/-/tree/master/extraStuff/Libreoffice%20troubleshooting'>" + KI18n.i18nc("@action link to the help page about LibreOffice problems", "Troubleshooting help") + "</a>"

    PdfDocumentView {
        // Shows the document
        id: docView
        visible: !messagePage.visible
        anchors {
            right: parent.right
            left: sidebar.right
            top: parent.top
            bottom: parent.bottom
        }
        pdfDocument: doc

        currentViewMode: settings.currentViewMode
        // Saves the settings of the current view mode
        Component.onDestruction: settings.currentViewMode = docView.currentViewMode

        PdfDocument {
            id: doc
            source: Manager.documentViewer.viewerDocument
            onSourceChanged: {
                docView.currentIndex = 0;
                docView.setRenderScale(1);
            }
        }

        // To avoid conflicts, shortcuts must be disabled when the viewer is not visible
        Shortcut {
            enabled: docView.visible
            sequences: KiviewApp.shortcuts.previous_page
            onActivated: docView.goToPage(docView.hoveredIndex - 1)
        }

        Shortcut {
            enabled: docView.visible
            sequences: KiviewApp.shortcuts.next_page
            onActivated: docView.goToPage(docView.hoveredIndex + 1)
        }

        Shortcut {
            enabled: docView.visible
            sequences: KiviewApp.shortcuts.scroll_up
            onActivated: docView.scrollUp()
        }

        Shortcut {
            enabled: docView.visible
            sequences: KiviewApp.shortcuts.scroll_down
            onActivated: docView.scrollDown()
        }
        MouseArea {
            anchors.fill: parent
            anchors.rightMargin: 20
            onWheel: wheel => {
                if (wheel.modifiers & Qt.ControlModifier) {
                    if (wheel.angleDelta.y > 0) {
                        docView.setRenderScale(docView.renderScale + 0.1);
                    } else {
                        docView.setRenderScale(docView.renderScale - 0.1);
                    }
                    wheel.accepted = true;
                } else {
                    wheel.accepted = false;
                }
            }
        }
    }

    Item {
        id: sidebar
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        visible: !messagePage.visible
        width: parent.width * 0.18
        // To change the layout type
        property int thumbnailsLayoutType: settings.thumbnailsLayoutType

        Rectangle {
            id: sidebarBackground
            anchors.fill: sidebar
            color: Kirigami.Theme.alternateBackgroundColor
        }

        Kirigami.Separator {
            id: lineSeparator
            anchors.left: sidebar.right
            anchors.bottom: parent.bottom
            anchors.top: parent.top
        }

        // Loads the thumbnails
        GridView {
            id: thumbnailsLayout
            anchors.fill: parent
            anchors.margins: 10
            model: doc.pageModel
            // The width of the cell changes according to the type of layout:
            // This eliminates the need to use ListView{} for displaying thumbnails in rows.
            cellWidth: sidebar.thumbnailsLayoutType == DocumentViewerUi.ThumbnailsLayoutTypes.Grid ? width / 2 : width
            // Note: cellHeight is set from inside the delegate
            // How tall should the thumbnail be
            // Note: The image maintains the same aspect ratio
            property int portraitImageHeight: sidebar.thumbnailsLayoutType == DocumentViewerUi.ThumbnailsLayoutTypes.Grid ? cellWidth * 1.3 : cellWidth * 1.2
            delegate: thumbnailsDelegate
        }

        // Saves the layout settings
        Component.onDestruction: settings.thumbnailsLayoutType = sidebar.thumbnailsLayoutType
    }

    // Changes the layout in the sidebar when clicked
    Rectangle {
        anchors.left: sidebar.right
        anchors.top: sidebar.top
        visible: !messagePage.visible
        width: 30
        height: 30
        color: Kirigami.Theme.alternateBackgroundColor

        Button {
            id: buttonThumbnailLayout
            anchors.fill: parent
            flat: true
            // The number of values ​​in the enums:
            // This is necessary because there is no way in qml to get the size of the enums
            property int enumLenght: 2
            // Changes the layout
            onClicked: sidebar.thumbnailsLayoutType = (sidebar.thumbnailsLayoutType + 1) % buttonThumbnailLayout.enumLenght
            // Updates the icon
            icon.name: sidebar.thumbnailsLayoutType == DocumentViewerUi.ThumbnailsLayoutTypes.List ? "view-list-details-symbolic" : "show-grid"
        }
    }

    Item {
        id: messagePage
        anchors.fill: parent
        visible: Manager.documentViewer.messagePageVisible

        Kirigami.Icon {
            id: messageIcon
            height: parent.height / 3
            width: parent.width / 3
            source: "libreoffice-master-document"
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            anchors.topMargin: parent.height / 7
        }

        Kirigami.LoadingPlaceholder {
            id: loadingIndicator
            visible: Manager.documentViewer.loadingBarVisible
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: messageIcon.bottom
        }

        Text {
            id: headerText
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: messageIcon.bottom
            text: Manager.documentViewer.messageHeaderText
            color: Kirigami.Theme.textColor
            font.bold: true
            font.pointSize: 13
        }

        Text {
            id: descriptionText
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: headerText.bottom
            width: parent.width * 0.8
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
            text: (Manager.documentViewer.helpPageLinkVisible ? Manager.documentViewer.messageDescriptionText + " " + documentViewer.htmlLink : Manager.documentViewer.messageDescriptionText)
            color: Kirigami.Theme.textColor
            linkColor: Kirigami.Theme.linkColor
            font.pointSize: 12
            onLinkActivated: link => Qt.openUrlExternally(link)
        }

        Button {
            id: relaodButton
            visible: Manager.documentViewer.reloadButtonVisible
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.topMargin: 20
            anchors.top: descriptionText.bottom
            text: KI18n.i18n("Reload")
            onPressed: Manager.documentViewer.reloadDocument()

            contentItem: Text {
                font.pointSize: 10
                text: relaodButton.text
                color: Kirigami.Theme.textColor
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
        }
    }

    Component {
        id: thumbnailsDelegate
        Item {
            // Inspired from: https://doc.qt.io/qt-6/qtpdf-multipage-example.html
            // This Item has been modified to work with both GridView and ListView.
            id: thumbnailsItem
            required property int index
            required property string label
            required property size pointSize
            width: image.width
            height: image.height + (pageNumber.height)
            // The height of the GridView cell is equal to the height of the image + the page number:
            // This allows for no gaps between thumbnails when in landscape mode
            onHeightChanged: thumbnailsLayout.cellHeight = thumbnailsItem.height

            Rectangle {
                id: paper
                // For some reason the rectangle is sometimes a pixel larger than the image
                // and this causes an unintended border with pdfs that already have a background and is not white in color.
                // Removing a pixel fixes the problem.
                width: image.width - 1
                height: image.height - 1
                x: (thumbnailsLayout.cellWidth - thumbnailsItem.width) / 2
                PdfPageImage {
                    id: image
                    document: doc
                    currentFrame: thumbnailsItem.index
                    asynchronous: true
                    fillMode: Image.PreserveAspectFit
                    property bool landscape: thumbnailsItem.pointSize.width > thumbnailsItem.pointSize.height
                    width: landscape ? thumbnailsLayout.cellWidth - 6 : height * thumbnailsItem.pointSize.width / thumbnailsItem.pointSize.height
                    height: landscape ? width * thumbnailsItem.pointSize.height / thumbnailsItem.pointSize.width : thumbnailsLayout.portraitImageHeight - pageNumber.height
                    sourceSize.width: width
                    sourceSize.height: height
                }
            }

            Text {
                id: pageNumber
                // Centers the text below the thumbnail
                anchors {
                    top: paper.bottom
                }
                x: paper.x + (paper.width / 2) - (width / 2)
                verticalAlignment: Text.AlignVCenter
                text: thumbnailsItem.label
                color: Kirigami.Theme.textColor
            }
            TapHandler {
                onTapped: docView.goToPage(thumbnailsItem.index)
            }
        }
    }
}
