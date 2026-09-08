import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

GlassPanel {
    id: root

    property var hostWindow
    signal subtitleRequested()
    signal audioOptionsRequested()
    signal networkStreamRequested()

    implicitHeight: 108
    panelColor: Theme.surface

    ColumnLayout {
        anchors.fill: parent
        anchors.leftMargin: 14
        anchors.rightMargin: 14
        anchors.topMargin: 9
        anchors.bottomMargin: 9
        spacing: 6

        Slider {
            id: timeline
            Layout.fillWidth: true
            from: 0
            to: 1
            value: player.duration > 0 ? player.position / player.duration : 0
            enabled: player.hasMedia && !player.imageMode && !player.isLiveStream && player.duration > 0
            onMoved: player.seek(value * player.duration)

            background: Rectangle {
                x: timeline.leftPadding
                y: timeline.topPadding + timeline.availableHeight / 2 - height / 2
                width: timeline.availableWidth
                height: 4
                radius: 2
                color: Theme.border

                Rectangle {
                    width: timeline.visualPosition * parent.width
                    height: parent.height
                    radius: 2
                    gradient: Gradient {
                        orientation: Gradient.Horizontal
                        GradientStop { position: 0; color: Theme.cyan }
                        GradientStop { position: 1; color: Theme.violet }
                    }
                }
            }

            handle: Rectangle {
                x: timeline.leftPadding + timeline.visualPosition * (timeline.availableWidth - width)
                y: timeline.topPadding + timeline.availableHeight / 2 - height / 2
                implicitWidth: timeline.hovered || timeline.pressed ? 15 : 11
                implicitHeight: implicitWidth
                radius: width / 2
                color: "#FFFFFF"
                border.width: 3
                border.color: Theme.cyan
                layer.enabled: true
                Behavior on implicitWidth { NumberAnimation { duration: Theme.animationFast } }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 6

            IconButton {
                glyph: "\u23EE"
                toolTip: "Previous playlist item"
                enabled: player.playlist.length > 0
                onClicked: player.playPreviousPlaylistItem()
            }

            IconButton {
                glyph: player.playing ? "\u2016" : "\u25B6"
                toolTip: player.playing ? "Pause" : "Play"
                prominent: true
                buttonSize: 44
                enabled: player.hasMedia && !player.imageMode
                onClicked: player.togglePlayback()
            }

            IconButton {
                glyph: "\u23ED"
                toolTip: "Next playlist item"
                enabled: player.playlist.length > 0
                onClicked: player.playNextPlaylistItem()
            }

            IconButton {
                glyph: player.muted || player.volume <= 0.01 ? "MUTE" : "VOL"
                toolTip: player.muted ? "Unmute" : "Mute"
                onClicked: player.toggleMute()
            }

            Slider {
                id: volumeSlider
                Layout.preferredWidth: 94
                from: 0
                to: 2
                value: player.volume
                onMoved: player.setVolume(value)
                background: Rectangle {
                    x: volumeSlider.leftPadding
                    y: volumeSlider.topPadding + volumeSlider.availableHeight / 2 - height / 2
                    width: volumeSlider.availableWidth
                    height: 3
                    radius: 2
                    color: Theme.border
                    Rectangle {
                        width: volumeSlider.visualPosition * parent.width
                        height: parent.height
                        radius: 2
                        color: player.volume > 1 ? Theme.amber : Theme.cyan
                    }
                }
                handle: Rectangle {
                    x: volumeSlider.leftPadding + volumeSlider.visualPosition * (volumeSlider.availableWidth - width)
                    y: volumeSlider.topPadding + volumeSlider.availableHeight / 2 - height / 2
                    implicitWidth: 10
                    implicitHeight: 10
                    radius: 5
                    color: "#FFFFFF"
                }
            }

            Text {
                text: Math.round(player.volume * 100) + "%"
                color: player.volume > 1 ? Theme.amber : Theme.textSecondary
                font.pixelSize: Theme.fontScale * 9
                Layout.preferredWidth: 34
            }

            Item { Layout.fillWidth: true }

            Rectangle {
                visible: player.isLiveStream
                Layout.alignment: Qt.AlignVCenter
                implicitWidth: liveLabel.implicitWidth + 14
                implicitHeight: 18
                radius: 9
                color: "#22FF4D4D"
                border.width: 1
                border.color: Theme.red

                RowLayout {
                    anchors.centerIn: parent
                    spacing: 4
                    Rectangle { width: 6; height: 6; radius: 3; color: Theme.red }
                    Text {
                        id: liveLabel
                        text: "LIVE"
                        color: Theme.red
                        font.pixelSize: Theme.fontScale * 9
                        font.weight: Font.DemiBold
                        font.letterSpacing: 0.8
                    }
                }
            }

            ColumnLayout {
                spacing: -1
                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: player.isLiveStream ? TimeFormat.time(player.position)
                                              : TimeFormat.time(player.position) + " / " + TimeFormat.time(player.duration)
                    color: Theme.text
                    font.pixelSize: Theme.fontScale * 12
                    font.weight: Font.DemiBold
                }
                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: player.hasMedia ? player.statusText : "Current / Total"
                    color: Theme.textMuted
                    font.pixelSize: Theme.fontScale * 8
                }
            }

            Item { Layout.fillWidth: true }

            IconButton { glyph: "-10"; toolTip: "Back 10 seconds"; enabled: player.hasMedia && !player.imageMode; onClicked: player.jumpSeconds(-10) }
            IconButton { glyph: "+10"; toolTip: "Forward 10 seconds"; enabled: player.hasMedia && !player.imageMode; onClicked: player.jumpSeconds(10) }

            IconButton {
                glyph: "CMP"
                toolTip: player.compareModeActive ? "Close compare mode (K)" : "Compare original vs. filtered (K)"
                enabled: player.hasMedia
                active: player.compareModeActive
                accentColor: Theme.cyan
                onClicked: player.compareModeActive ? player.exitCompareMode() : player.enterCompareMode()
            }

            IconButton {
                glyph: "\u22EF"
                toolTip: "More controls"
                active: player.playbackRate !== 1 || player.hasSubtitles
                onClicked: overflowMenu.popup()
            }

            IconButton {
                glyph: "FULL"
                toolTip: "Fullscreen"
                enabled: player.hasMedia
                onClicked: if (root.hostWindow) root.hostWindow.toggleFullscreen()
            }
        }
    }

    // Secondary controls live here instead of the transport bar, which stays
    // usable at the app's minimum window width even with a side panel open.
    ThemedMenu {
        id: overflowMenu

        ThemedMenuItem {
            text: "Previous frame (,)"
            enabled: player.hasMedia && !player.imageMode
            onTriggered: player.stepFrame(-1)
        }
        ThemedMenuItem {
            text: "Next frame (.)"
            enabled: player.hasMedia && !player.imageMode
            onTriggered: player.stepFrame(1)
        }

        ThemedMenuSeparator {}

        ThemedMenu {
            title: "Playback speed (" + Number(player.playbackRate).toFixed(player.playbackRate % 1 === 0 ? 1 : 2) + "x)"
            ThemedMenuItem { text: "0.50x"; onTriggered: player.setPlaybackRate(0.5) }
            ThemedMenuItem { text: "0.75x"; onTriggered: player.setPlaybackRate(0.75) }
            ThemedMenuItem { text: "1.00x (Normal)"; onTriggered: player.setPlaybackRate(1.0) }
            ThemedMenuItem { text: "1.25x"; onTriggered: player.setPlaybackRate(1.25) }
            ThemedMenuItem { text: "1.50x"; onTriggered: player.setPlaybackRate(1.5) }
            ThemedMenuItem { text: "2.00x"; onTriggered: player.setPlaybackRate(2.0) }
            ThemedMenuItem { text: "3.00x"; onTriggered: player.setPlaybackRate(3.0) }
            ThemedMenuItem { text: "4.00x"; onTriggered: player.setPlaybackRate(4.0) }
        }

        ThemedMenuItem {
            text: player.hasSubtitles ? "Change subtitles (" + player.subtitleFileName + ")..." : "Load subtitles..."
            enabled: player.hasMedia && player.mediaKind === "Video"
            onTriggered: root.subtitleRequested()
        }
        ThemedMenuItem {
            text: "Audio and subtitle tracks..."
            enabled: player.hasMedia && !player.imageMode
            onTriggered: root.audioOptionsRequested()
        }

        ThemedMenuSeparator {}

        ThemedMenuItem { text: "Set loop start (A)"; enabled: player.hasMedia && player.seekable; onTriggered: player.setLoopStart() }
        ThemedMenuItem { text: "Set loop end (B)"; enabled: player.hasMedia && player.seekable; onTriggered: player.setLoopEnd() }
        ThemedMenuItem { text: player.loopEnabled ? "Clear A-B loop" : "Clear loop"; enabled: player.loopStart >= 0; onTriggered: player.clearLoop() }
        ThemedMenuItem { text: "Add bookmark"; enabled: player.hasMedia && !player.imageMode; onTriggered: player.addBookmark("") }
        ThemedMenuSeparator {}

        ThemedMenuItem {
            text: "Capture native frame (lossless PNG)"
            enabled: player.hasMedia
            onTriggered: player.captureOriginalFrame()
        }
        ThemedMenuItem {
            text: "Compare original vs. filtered (K)"
            enabled: player.hasMedia
            onTriggered: player.enterCompareMode()
        }
        ThemedMenuSeparator {}
        ThemedMenuItem {
            text: "Open network stream (Ctrl+U)..."
            onTriggered: root.networkStreamRequested()
        }
    }
}
