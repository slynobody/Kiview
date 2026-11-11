// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2024 danilo agostini <kyre334@gmail.com>
import QtQuick
import org.kde.kiview
import org.kde.kirigami as Kirigami
import org.kde.ki18n

Item {
    id: viewersLoader
    anchors.fill: parent

    property list<Kirigami.Action> globalActions

    //these actions are shown in the headerbar of the application.
    globalActions: [
        Kirigami.Action {
            icon.name: "go-up"
            text: KI18n.i18nc("@action:button show the folder or the archive of the shown file", "Up")
            displayHint: Kirigami.DisplayHint.IconOnly
            visible: Manager.canGoUp
            enabled: Manager.canGoUp
            shortcut: KiviewApp.shortcuts.go_up[0] ?? ""
            onTriggered: Manager.goUp()
        },
        Kirigami.Action {
            icon.name: "kt-info-widget"
            onTriggered: shortcutsDialog.open()
        }
    ]

    InfoDialog {
        id: shortcutsDialog
    }

    //these actions depend on the current viewer and are shown in the application's headerbar.
    property list<Kirigami.Action> actions

    // It is used to load the correct viewer
    property int selectedViewer
    selectedViewer: Manager.currentViewer

    // Used later to store created objects:
    // This allows viewers to be dynamically created and makes startup faster
    property QtObject currentViewer
    property QtObject textViewerObj
    property QtObject imageViewerObj
    property QtObject fallbackViewerObj
    property QtObject documentViewerObj
    property QtObject videoViewerObj
    property QtObject containerViewerObj

    onCurrentViewerChanged: {
        // The actions to be displayed in the header bar
        if (viewersLoader.currentViewer !== null) {
            viewersLoader.actions = combineLists(viewersLoader.currentViewer.actions, viewersLoader.globalActions);
        } else {
            viewersLoader.actions = viewersLoader.globalActions;
        }
    }

    onSelectedViewerChanged: viewersLoader.loadSelectedViewer()

    function loadSelectedViewer() {
        // Shows or hides viewers based on what is needed
        if (viewersLoader.selectedViewer === Manager.Viewer.NoViewer) {
            return;
        }
        let newViewer;
        if (viewersLoader.currentViewer !== null) {
            viewersLoader.currentViewer.visible = false;
        }
        switch (selectedViewer) {
        case Manager.Viewer.Text:
            if (textViewerObj === null) {
                textViewerObj = text_viewer.createObject(viewersLoader);
            }
            newViewer = textViewerObj;
            break;
        case Manager.Viewer.Image:
            if (imageViewerObj === null) {
                imageViewerObj = image_viewer.createObject(viewersLoader);
            }
            newViewer = imageViewerObj;
            break;
        case Manager.Viewer.Fallback:
            if (fallbackViewerObj === null) {
                fallbackViewerObj = fallback_viewer.createObject(viewersLoader);
            }
            newViewer = fallbackViewerObj;
            break;
        case Manager.Viewer.Document:
            if (documentViewerObj === null) {
                documentViewerObj = document_viewer.createObject(viewersLoader);
            }
            newViewer = documentViewerObj;
            break;
        case Manager.Viewer.Video:
            if (videoViewerObj === null) {
                videoViewerObj = video_viewer.createObject(viewersLoader);
            }
            newViewer = videoViewerObj;
            break;
        case Manager.Viewer.Container:
            if (containerViewerObj === null) {
                containerViewerObj = container_viewer.createObject(viewersLoader);
            }
            newViewer = containerViewerObj;
            break;
        }

        // Destroys the old viewer to free up ram
        if (viewersLoader.currentViewer !== newViewer) {
            unloadViewer(viewersLoader.currentViewer);
        }
        viewersLoader.currentViewer = newViewer;
        viewersLoader.currentViewer.visible = true;
    }

    // This function is called from main.qml to avoid having the kiview icon appear for a split second in the taskbar when in daemon mode.
    // The appearance of the icon in the taskbar is probably due to the fact that the
    // children are instantiated before the parents , and thus, before root.hide() (main.qml) is called.
    function preloadViewers(): void {
        // Preloads DocumentViewer to speed up the opening of the first document.
        if (viewersLoader.documentViewerObj === null) {
            documentViewerObj = document_viewer.createObject(viewersLoader);
            documentViewerObj.visible = false;
        }
    }

    function unloadViewer(viewer) {
        if (viewer !== null && viewer.isPersistent == false) {
            // Destroys the viewer to take up less ram.
            viewer.destroy();
        }
    }

    function unloadAllViewers() {
        // Destroys all viewers to save ram
        let viewersList = [textViewerObj, imageViewerObj, fallbackViewerObj, documentViewerObj, videoViewerObj, containerViewerObj];
        for (let i = 0; i < viewersList.length; i++) {
            if (viewersList[i] !== null && !viewersList[i].isPersistent) {
                viewersList[i].destroy();
            }
        }
        //  Calls the garbage collector
        gc();
    }

    function combineLists(list1, list2) {
        return [...list1, ...list2];
    }

    // Components that can be created dynamically
    // This allows viewers to be dynamically created and makes startup faster
    Component {
        id: text_viewer
        TextViewerUi {}
    }

    Component {
        id: image_viewer
        ImageViewerUi {}
    }

    Component {
        id: fallback_viewer
        FallbackViewerUi {}
    }

    Component {
        id: document_viewer
        DocumentViewerUi {}
    }
    Component {
        id: video_viewer
        VideoViewerUi {}
    }

    Component {
        id: container_viewer
        ContainerViewerUI {}
    }
}
