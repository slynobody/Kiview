// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2024 danilo agostini <kyre334@gmail.com>
import QtQuick
import QtQuick.Controls as Controls
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.statefulapp as StatefulApp
import org.kde.kiview
import QtQuick.Window

StatefulApp.StatefulWindow {
    id: root
    windowName: "Main"
    application: KiviewApp
    title: Manager.windowTitle
    width: Screen.width * 0.56
    height: Screen.height * 0.6
    minimumHeight: Screen.height * 0.4
    minimumWidth: Screen.width * 0.35

    // The visibility to return to from full screen, such as maximized.
    property int visibilityBeforeFullScreen: Window.Windowed

    function toggleFullScreen(): void {
        if (root.visibility === Window.FullScreen) {
            root.visibility = root.visibilityBeforeFullScreen;
        } else {
            root.visibilityBeforeFullScreen = root.visibility;
            root.showFullScreen();
        }
    }

    // Space belongs to the viewers and the dialogs, and does not close Kiview.
    // In full screen, the keys that close Kiview leave full screen first.
    Shortcut {
        sequences: KiviewApp.shortcuts.close
        onActivated: root.visibility === Window.FullScreen ? root.toggleFullScreen() : root.close()
    }
    Shortcut {
        sequences: KiviewApp.shortcuts.open_in_application
        onActivated: Manager.openApp()
    }
    Shortcut {
        sequences: KiviewApp.shortcuts.options_configure_keybinding
        onActivated: KiviewApp.shortcutsEditorAction()
    }

    // The keys used to scroll through files/perform certain actions
    // are sent via D-Bus signal (which apps subscribe to) to the underlying application (dolphin, etc).
    // This way the underlying application can manage how to react to these keys being pressed and tell kiview what to do.
    Item {
        id: keyPresses
        anchors.fill: parent
        focus: true
        Keys.onPressed: event => {
            if ([Qt.Key_Up, Qt.Key_Down, Qt.Key_Right, Qt.Key_Left].includes(event.key)) {
                BusInterface.SendKeyPresses(event.key, event.modifiers);
                event.accepted = true;
            }
        }
    }

    // If started with "--daemon", hide the application at startup (runs in background).
    Component.onCompleted: {
        if (BusInterface.isDaemon) {
            root.hide();
            // This function is called from main.qml to avoid having the kiview icon appear for a split second in the taskbar when in daemon mode.
            // The appearance of the icon in the taskbar is probably due to the fact that the
            // children are instantiated before the parents , and thus, before root.hide() is called.
            viewersLoader.preloadViewers();
        }
    }
    // Shows the application when it receives the signal from ContentManager.
    Connections {
        target: Manager
        // showNormal()/show() should only be called if the window is hidden,
        // otherwise the window will not remain maximized if it is.
        function onShowKiview() {
            if (!root.visible) {
                // In daemon mode, viewers are unloaded when Kiview is closed/hidden and,
                // therefore, The current viewer needs to be reloaded when Kiview is reopened.
                viewersLoader.loadSelectedViewer();
                root.showNormal();
                root.requestActivate();
            }
        }
    }
    // Quit application from QML: For some reason "QApplication::quit();"
    // (C++) does not trigger the Close event and so I can't reject it.
    // The same goes for "Qt.quit()" (Qml), that's why root.close() is used instead.
    Connections {
        target: Manager
        function onCloseQml() {
            root.close();
        }
    }

    // If it is in daemon mode, reject closing the application and hide it instead.
    onClosing: event => {
        event.accepted = !BusInterface.isDaemon;
        if (BusInterface.isDaemon) {
            viewersLoader.unloadAllViewers();
            root.hide();
        }
    }

    // Custom header bar to avoid a bug that causes PDF documents to be rendered on top of it.
    header: Controls.ToolBar {
        visible: root.visibility !== Window.FullScreen
        Keys.forwardTo: [keyPresses]
        Kirigami.ToolBarLayout {
            anchors.fill: parent

            Text {
                id: title
                text: Manager.fileName
                textFormat: Text.PlainText
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.right: actionToolBar.left
                verticalAlignment: Text.AlignVCenter
                leftPadding: 12
                color: Kirigami.Theme.textColor
                font.pointSize: Kirigami.Theme.defaultFont.pointSize * 1.35
                // defaultFont.pointSize * 1.35 comes from Kirigami.Heading:
                // This multiplication is needed to have the same font size used in the original headerbar and maintain consistency
                // https://api.kde.org/frameworks/kirigami/html/Heading_8qml_source.html
            }

            Kirigami.ActionToolBar {
                id: actionToolBar
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                anchors.right: parent.right
                anchors.left: title.right
                alignment: Qt.AlignRight
                actions: kiriPage.actions
            }
        }
    }

    pageStack.initialPage: Kirigami.Page {
        id: kiriPage
        padding: 0
        title: Manager.fileName
        actions: viewersLoader.actions
        // The default header bar is disabled due to a bug that causes PDF documents to be rendered on top of it.
        // (as if the anchors were not respected)
        globalToolBarStyle: Kirigami.ApplicationHeaderStyle.None
        Keys.forwardTo: [keyPresses]

        ViewersLoader {
            id: viewersLoader
        }
        NavButtons {
            autoHidden: viewersLoader.currentViewer?.controlsShown === false
            onDarkBackground: viewersLoader.currentViewer?.showsVideo === true
        }
    }
}
