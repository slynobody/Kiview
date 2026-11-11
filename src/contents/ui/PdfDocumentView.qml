// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2024 danilo agostini <kyre334@gmail.com>
pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Pdf
import QtQuick.Controls
import org.kde.kirigami as Kirigami

// This is an alternative to PdfMultiPageView:
// I created it because PdfMultiPageView is not fully functional at the moment and
// it is easier to write an alternative than to make the original one work.
ListView {
    id: pdfDocumentView
    enum ViewModes {
        FitToWidth,
        FitToHeight,
        Automatic
    }
    // The document to load
    property PdfDocument pdfDocument: null
    // Used to zoom in/out
    property real renderScale: 1
    // How the page should be displayed (fit to height/width)
    property int currentViewMode: PdfDocumentView.ViewModes.Automatic
    // Resets renderScale
    onCurrentViewModeChanged: setRenderScale(1)
    // Holds the index number of the currently displayed page
    property int hoveredIndex: 0
    // This fixes a bug where hoveredIndex is "-1" in case the user calls `goToPage()`
    // and immediately after changes the pdf to display while the page change animation is still playing.
    // if hoveredIndex is equal to "-1" the `goToPage()` function does not work because
    // of how the previous/next page is selected by the external code that calls the function:
    // docView.goToPage(docView.hoveredIndex + 1)
    // hoveredIndex never becomes "0" because it is not updated since "contentY" does not change/update.
    onHoveredIndexChanged: if (hoveredIndex == -1) {
        hoveredIndex = 0;
    }

    property int scrollSteps: 20
    // When in fitToWidth mode, it prevents the page from overlapping the scrollbar.
    // This also prevents the horizontal scrollbar from appearing without zooming.
    property int fitWidthSideMargin: verticalScrollbar.width * 2
    ScrollBar.vertical: ScrollBar {
        id: verticalScrollbar
        parent: parent
    }
    ScrollBar.horizontal: ScrollBar {
        parent: parent
    }
    // Animates the index change animation by duration and not by speed
    highlightMoveDuration: 500
    highlightMoveVelocity: -1
    // Allows scrolling sideways:
    // Required because the page can be zoomed in
    flickableDirection: Flickable.HorizontalAndVerticalFlick
    // Gets the index of the current page
    // This applies when changing current index, scrolling the mouse wheel, and when moving the scroll bar.
    onContentYChanged: hoveredIndex = indexAt(contentX, contentY + height * 0.5)

    // Functions to manage the current page/zooming,etc.
    function scrollUp(): void {
        if (contentY - scrollSteps > 0) {
            contentY -= scrollSteps;
        } else {
            contentY = 0;
        }
    }

    function scrollDown(): void {
        if (contentY + scrollSteps < contentHeight - height) {
            contentY += scrollSteps;
        } else {
            contentY = contentHeight - height;
        }
    }
    function goToPage(newIndex: int): void {
        // currentIndex is first changed to the hoveredIndex and then is changed to the new index.
        // This ensures that the page is changed even when the current index matches the new index.
        // Note: The current index is not constantly updated because otherwise it would start the animation.
        // that's why it is only updated here.
        currentIndex = hoveredIndex;
        if (newIndex > -1 && newIndex < count) {
            currentIndex = newIndex;
        }
    }

    function setRenderScale(newRenderScale: real): void {
        if (newRenderScale < 2 && newRenderScale > 0.5) {
            renderScale = newRenderScale;
        }
    }

    model: pdfDocument.pageModel
    delegate: Item {
        id: pageItem
        // Inspired from: https://doc.qt.io/qt-6/qtpdf-multipage-example.html
        required property int index
        required property string label
        required property size pointSize
        // Zoom-in support: the minimum width is that of the parent to keep the document in the center
        // if it does not take up the whole page, the maximum width is that of the image.
        width: image.sourceSize.width > pdfDocumentView.width ? image.sourceSize.width : pdfDocumentView.width
        // This notifies the parent that the content width exceeds its own and must show the horizontal scrollbar
        onWidthChanged: pdfDocumentView.contentWidth = pageItem.width
        height: image.height + pageNumber.height

        Rectangle {
            id: paper
            // For some reason the rectangle is sometimes a pixel larger than the image
            // and this causes an unintended border with pdfs that already have a background and is not white in color.
            // Removing a pixel fixes the problem.
            width: image.width - 1
            height: image.height - 1
            anchors.horizontalCenter: parent.horizontalCenter
            PdfPageImage {
                id: image
                document: pdfDocumentView.pdfDocument
                currentFrame: pageItem.index
                asynchronous: true
                fillMode: Image.PreserveAspectFit
                // Updates the size when the component is created
                Component.onCompleted: setSize()
                // Updates the size when the geometry/display mode, etc are updated.
                Connections {
                    target: pdfDocumentView
                    function onCurrentViewModeChanged() {
                        image.setSize();
                    }
                    function onWidthChanged() {
                        image.setSize();
                    }
                    function onHeightChanged() {
                        image.setSize();
                    }
                    function onRenderScaleChanged() {
                        image.setSize();
                    }
                }

                // The width/height must be updated in the same function to avoid the error:
                // Binding loop detected for property "height"/"width"
                // This error only occurred at certain ratios, for example:
                // a window with a resolution of 800x600 did not cause problems, a 800x650 did
                // (it also depended on the width/height of the page)
                // The previous code, which was causing problems,
                // used two separate functions and was based on this:
                // https://doc.qt.io/qt-6/qtpdf-multipage-example.html (search "landscape")

                // Updates width/height based on settings:
                property bool landscape: pageItem.pointSize.width > pageItem.pointSize.height
                function setSize(): void {
                    let ToMaximumWidth = false;
                    let newWidth = 0;
                    let newHeight = 0;
                    switch (pdfDocumentView.currentViewMode) {
                    case PdfDocumentView.ViewModes.Automatic:
                        ToMaximumWidth = image.landscape;
                        break;
                    case PdfDocumentView.ViewModes.FitToWidth:
                        ToMaximumWidth = true;
                        break;
                    case PdfDocumentView.ViewModes.FitToHeight:
                        ToMaximumWidth = false;
                        break;
                    }
                    if (ToMaximumWidth) {
                        newWidth = (pdfDocumentView.width - pdfDocumentView.fitWidthSideMargin) * pdfDocumentView.renderScale;
                        newHeight = newWidth * (pageItem.pointSize.height / pageItem.pointSize.width);
                    } else {
                        newHeight = (pdfDocumentView.height - pageNumber.height) * pdfDocumentView.renderScale;
                        newWidth = newHeight * (pageItem.pointSize.width / pageItem.pointSize.height);
                    }
                    image.width = newWidth;
                    image.height = newHeight;
                }

                sourceSize.width: width
                sourceSize.height: height
            }
            Text {
                id: pageNumber
                // Centers the text below the page
                anchors {
                    top: paper.bottom
                    right: paper.right
                    left: paper.left
                }
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
                text: pageItem.label
                color: Kirigami.Theme.textColor
            }
        }
    }
}
