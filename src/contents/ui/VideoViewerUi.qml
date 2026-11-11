// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2024 danilo agostini <kyre334@gmail.com>
import QtQuick
import QtCore
import QtQuick.Window
import QtQuick.Controls
import QtMultimedia
import org.kde.kiview
import org.kde.kirigami as Kirigami

// Videos currently fail to play in Qt6/Qml via "QtMultimedia" on some Linux distributions.
// The reason seems to be that you need gstreamer 1.22 with qml6glsink and they are currently not available on kde neon (ubuntu 22.04).
Item {
    id: videoViewer
    anchors.fill: parent
    // Property to enable/disable unloading of the viewer when not in use.
    property bool isPersistent: false
    Settings {
        id: settings
        property alias volume: videoFrame.volume
    }
    // Hides the volume slider when the visibility changes.
    // This is to prevent the slider from remaining open when a user changes files.
    onVisibleChanged: volumeSlider.visible = false
    // A video is shown on black, which the navigation buttons follow.
    readonly property bool showsVideo: videoFrame.hasVideo && videoFrame.errorString === ""

    // While a video plays, the controls hide when the mouse stays still for 5 seconds or leaves the viewer.
    // They are painted over the video, so that the video does not move when they show or hide.
    property bool pointerActive: false
    readonly property bool controlsShown: !videoFrame.hasVideo || videoFrame.playbackState !== MediaPlayer.PlayingState || pointerActive
                                          || volumeSlider.visible || videoSlider.pressed

    function showControls(): void {
        pointerActive = true;
        pointerTimer.restart();
    }

    // Qt Quick sends hover events at each frame of the video, and the position of a still pointer can
    // vary by a fraction of a pixel with fractional scaling, so only a move of 2 pixels counts.
    property point pointerPosition
    HoverHandler {
        onPointChanged: {
            if (Math.abs(point.position.x - videoViewer.pointerPosition.x) + Math.abs(point.position.y - videoViewer.pointerPosition.y) >= 2) {
                videoViewer.pointerPosition = point.position;
                videoViewer.showControls();
            }
        }
        onHoveredChanged: {
            if (!hovered) {
                pointerTimer.stop();
                videoViewer.pointerActive = false;
            }
        }
    }
    Timer {
        id: pointerTimer
        interval: 5000
        onTriggered: videoViewer.pointerActive = false
    }

    property list<Kirigami.Action> actions
    //these actions are read by viewersLoader and are shown in the "header bar" of the application.
    actions: [
        OpenAppButton {}
    ]

    Text {
        id: errorMessage
        anchors.verticalCenter: parent.verticalCenter
        anchors.horizontalCenter: parent.horizontalCenter
        visible: videoFrame.errorString !== ""
        text: videoFrame.errorString
        color: Kirigami.Theme.textColor
        font.pointSize: 20
    }

    // Videos are shown on black, as in other players. Audio files have their own background below.
    Rectangle {
        anchors.fill: videoFrame
        color: "black"
        visible: videoFrame.errorString === ""
    }

    Video {
        id: videoFrame
        focus: true
        volume: settings.volume

        // Background for audio files.
        Rectangle {
            id: videoFrameBackground
            anchors.fill: parent
            color: Kirigami.Theme.backgroundColor
            visible: videoFrame.errorString === "" && videoFrame.seekable && !videoFrame.hasVideo
            Kirigami.Icon {
                id: placeholderIcon
                anchors {
                    top: parent.top
                    horizontalCenter: parent.horizontalCenter
                    verticalCenter: parent.verticalCenter
                }
                height: parent.height / 3
                width: parent.width / 3
                source: "media-default-album"
            }
        }

        // Pauses the media if the viewer is hidden and resumes playback if the source has not changed.
        onVisibleChanged: {
            if (!visible && seekable) {
                videoFrame.pause();
            } else if (visible && videoFrame.playbackState === MediaPlayer.PausedState) {
                videoFrame.play();
            }
        }

        source: Manager.videoViewer.viewerMedia
        onSourceChanged: play()

        anchors {
            right: parent.right
            left: parent.left
            top: parent.top
            bottom: parent.bottom
        }

        onPositionChanged: {
            if (!videoSlider.pressed)
                videoSlider.value = videoFrame.position;
        }

        onPlaybackStateChanged: {
            if (videoFrame.playbackState === MediaPlayer.StoppedState && videoFrame.position === videoFrame.duration)
                videoFrame.play();
        }

        MouseArea {
            anchors.fill: parent
            // The cursor hides with the controls.
            cursorShape: videoViewer.controlsShown ? Qt.ArrowCursor : Qt.BlankCursor
            // A click plays or pauses once the time for a double click is over, so that a double
            // click only switches to full screen.
            onClicked: {
                clickTimer.restart();
                volumeSlider.visible = false;
            }
            onDoubleClicked: {
                clickTimer.stop();
                videoViewer.Window.window.toggleFullScreen();
            }
            Timer {
                id: clickTimer
                interval: Qt.styleHints.mouseDoubleClickInterval
                onTriggered: videoViewer.togglePlay()
            }
            // A notch of the wheel changes the volume by 5%, or moves 5 seconds with Ctrl, as over the progress bar.
            onWheel: wheel => {
                if (wheel.modifiers & Qt.ControlModifier) {
                    videoViewer.seekBy(wheel.angleDelta.y / 120 * 5000);
                    return;
                }
                videoViewer.changeVolume(wheel.angleDelta.y);
            }
        }
    }

    Kirigami.Separator {
        id: lineSeparator
        visible: controlBar.visible
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: controlBar.top
    }

    Item {
        id: controlBar
        visible: videoFrame.errorString === "" && videoViewer.controlsShown
        anchors {
            right: parent.right
            left: parent.left
            bottom: parent.bottom
        }

        MouseArea {
            // This area is used to hide the volume slider (if visible/open)
            // if the user clicks on an empty spot in the control bar.
            anchors.fill: parent
            onClicked: volumeSlider.visible = false
        }

        height: 30
        Rectangle {
            id: controlBarBackground
            anchors.fill: parent
            color: Kirigami.Theme.alternateBackgroundColor
        }
        Button {
            id: togglePlayButton
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            flat: true
            onPressed: {
                videoViewer.togglePlay();
                volumeSlider.visible = false;
            }
            icon.name: (videoFrame.playbackState === MediaPlayer.PlayingState ? "media-playback-pause" : "media-playback-start")
        }

        Slider {
            id: videoSlider
            // Avoids taking focus: The arrow keys are used to scroll through files and Slider{}
            // uses the same keys to go forward/backward in the track.
            // This causes conflicts and makes scrolling through files not work.
            focusPolicy: Qt.NoFocus
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: togglePlayButton.right
            anchors.right: volumeButton.left
            from: 0
            to: videoFrame.duration

            onPressedChanged: {
                volumeSlider.visible = false;
                if (!videoSlider.pressed) {
                    videoFrame.seek(videoSlider.value);
                }
            }

            // A notch of the wheel moves 5 seconds, forward when scrolling up.
            WheelHandler {
                acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
                onWheel: event => videoViewer.seekBy(event.angleDelta.y / 120 * 5000)
            }
        }

        Button {
            id: volumeButton
            anchors.verticalCenter: parent.verticalCenter
            anchors.right: infoSection.left
            flat: true
            onPressed: volumeSlider.visible = !volumeSlider.visible
            icon.name: videoFrame.muted ? "player-volume-muted" : "player-volume"
            WheelHandler {
                acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
                onWheel: event => videoViewer.changeVolume(event.angleDelta.y)
            }
            Slider {
                id: volumeSlider
                anchors.horizontalCenter: parent.horizontalCenter
                visible: false
                // The focus is rejected to prevent the internal shortcuts of Slider{}
                // from causing conflicts with those of kiview.
                //(in the future all the arrow keys will be used to change the selected file)
                focusPolicy: Qt.NoFocus
                orientation: Qt.Vertical
                y: parent.y - height
                from: 0
                to: 1
                value: videoFrame.volume
                onValueChanged: videoFrame.volume = value
            }
        }

        Item {
            id: infoSection
            anchors.verticalCenter: parent.verticalCenter
            anchors.rightMargin: 15
            anchors.right: parent.right
            width: (totalTime.width * 2) + separator.width
            Text {
                id: currentTime
                anchors.right: separator.left
                anchors.verticalCenter: parent.verticalCenter
                color: Kirigami.Theme.textColor
                text: videoViewer.millisToMinutesAndSeconds(videoFrame.position)
            }
            Text {
                id: separator
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.verticalCenter: parent.verticalCenter
                color: Kirigami.Theme.textColor
                text: "/"
            }
            Text {
                id: totalTime
                anchors.left: separator.right
                anchors.verticalCenter: parent.verticalCenter
                color: Kirigami.Theme.textColor
                text: videoViewer.millisToMinutesAndSeconds(videoFrame.duration)
            }
        }
    }

    // To avoid conflicts, shortcuts must be disabled when the viewer is not visible
    Shortcut {
        enabled: videoViewer.visible
        sequences: KiviewApp.shortcuts.seek_forward
        onActivated: videoViewer.seekBy(10000)
    }
    Shortcut {
        enabled: videoViewer.visible
        sequences: KiviewApp.shortcuts.seek_backward
        onActivated: videoViewer.seekBy(-10000)
    }
    Shortcut {
        enabled: videoViewer.visible
        sequences: KiviewApp.shortcuts.skip_forward
        onActivated: videoViewer.seekBy(30000)
    }
    Shortcut {
        enabled: videoViewer.visible
        sequences: KiviewApp.shortcuts.skip_backward
        onActivated: videoViewer.seekBy(-30000)
    }
    Shortcut {
        enabled: videoViewer.visible
        sequences: KiviewApp.shortcuts.toggle_fullscreen
        onActivated: videoViewer.Window.window.toggleFullScreen()
    }
    Shortcut {
        enabled: videoViewer.visible
        sequences: KiviewApp.shortcuts.mute
        onActivated: videoFrame.muted = !videoFrame.muted
    }
    Shortcut {
        enabled: videoViewer.visible
        sequences: KiviewApp.shortcuts.play_pause
        onActivated: videoViewer.togglePlay()
    }

    // Changes the volume by 5% per notch of the wheel, given in eighths of a degree, and unmutes.
    function changeVolume(angleDelta: real): void {
        videoFrame.volume = Math.max(0, Math.min(1, videoFrame.volume + angleDelta / 120 * 0.05));
        videoFrame.muted = false;
    }

    // Moves the position by offset milliseconds, stopping at the start and the end of the track.
    function seekBy(offset: real): void {
        showControls();
        videoFrame.seek(Math.max(0, Math.min(videoFrame.duration, videoFrame.position + offset)));
    }

    function togglePlay(): void {
        showControls();
        if (videoFrame.playbackState === MediaPlayer.PlayingState) {
            videoFrame.pause();
        } else {
            videoFrame.play();
        }
    }

    function millisToMinutesAndSeconds(millis: real): string {
        const minutes = Math.floor(millis / 60000);
        const seconds = ((millis % 60000) / 1000).toFixed(0);
        return minutes + ":" + (seconds < 10 ? '0' : '') + seconds;
    }
}
