import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    property bool active: false

    signal exitFullscreenRequested()
    signal subtitleRequested()

    readonly property bool paused: player.hasMedia && !player.imageMode && !player.playing
    readonly property bool holdOpen: topHover.hovered || bottomHover.hovered
                                     || seekBar.pressed || volumeBar.pressed
    readonly property bool shown: active && (recentActivity || paused)

    property bool recentActivity: false

    visible: active

    function reveal() {
        if (!active)
            return
        recentActivity = true
        if (holdOpen || paused)
            idleTimer.stop()
        else
            idleTimer.restart()
    }

    onActiveChanged: {
        if (active) {
            reveal()
        } else {
            recentActivity = false
            idleTimer.stop()
        }
    }
    onHoldOpenChanged: reveal()
    onPausedChanged: reveal()

    Timer {
        id: idleTimer
        interval: 2600
        onTriggered: root.recentActivity = false
    }

    // Passive: tracks pointer movement anywhere over the video without stealing clicks.
    HoverHandler {
        enabled: root.active
        onPointChanged: root.reveal()
    }

    Item {
        id: topBar
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: 86
        enabled: root.shown
        opacity: root.shown ? 1 : 0

        Behavior on opacity { NumberAnimation { duration: Theme.animation } }

        Rectangle {
            anchors.fill: parent
            gradient: Gradient {
                orientation: Gradient.Vertical
                GradientStop { position: 0; color: "#C0000000" }
                GradientStop { position: 1; color: "#00000000" }
            }
        }

        HoverHandler { id: topHover; enabled: root.shown }

        RowLayout {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.leftMargin: 24
            anchors.rightMargin: 18
            anchors.topMargin: 14
            spacing: 14

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2

                Text {
                    Layout.fillWidth: true
                    text: player.title
                    color: "#FFFFFF"
                    font.pixelSize: Theme.fontScale * 17
                    font.weight: Font.DemiBold
                    elide: Text.ElideMiddle
                }

                Text {
                    Layout.fillWidth: true
                    text: player.videoWidth > 0
                          ? player.mediaKind.toUpperCase() + "   " + player.videoWidth + " x " + player.videoHeight
                          : player.mediaKind.toUpperCase()
                    color: "#B0FFFFFF"
                    font.pixelSize: Theme.fontScale * 10
                    font.letterSpacing: 1
                    elide: Text.ElideRight
                }
            }

            IconButton {
                glyph: "\u2715"
                toolTip: "Leave fullscreen (Esc)"
                buttonSize: 38
                darkSurface: true
                onClicked: root.exitFullscreenRequested()
            }
        }
    }

    Item {
        id: bottomBar
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 122
        enabled: root.shown
        opacity: root.shown ? 1 : 0

        Behavior on opacity { NumberAnimation { duration: Theme.animation } }

        Rectangle {
            anchors.fill: parent
            gradient: Gradient {
                orientation: Gradient.Vertical
                GradientStop { position: 0; color: "#00000000" }
                GradientStop { position: 0.42; color: "#8C000000" }
                GradientStop { position: 1; color: "#DE000000" }
            }
        }

        HoverHandler { id: bottomHover; enabled: root.shown }

        ColumnLayout {
            anchors.fill: parent
            anchors.leftMargin: 24
            anchors.rightMargin: 20
            anchors.topMargin: 22
            anchors.bottomMargin: 14
            spacing: 4

            Slider {
                id: seekBar
                Layout.fillWidth: true
                visible: !player.imageMode
                from: 0
                to: 1
                value: player.duration > 0 ? player.position / player.duration : 0
                enabled: player.hasMedia && !player.imageMode && player.duration > 0
                onMoved: player.seek(value * player.duration)

                background: Rectangle {
                    x: seekBar.leftPadding
                    y: seekBar.topPadding + seekBar.availableHeight / 2 - height / 2
                    width: seekBar.availableWidth
                    height: 5
                    radius: 3
                    color: "#4DFFFFFF"

                    Rectangle {
                        width: seekBar.visualPosition * parent.width
                        height: parent.height
                        radius: 3
                        gradient: Gradient {
                            orientation: Gradient.Horizontal
                            GradientStop { position: 0; color: Theme.cyan }
                            GradientStop { position: 1; color: Theme.violet }
                        }
                    }
                }

                handle: Rectangle {
                    x: seekBar.leftPadding + seekBar.visualPosition * (seekBar.availableWidth - width)
                    y: seekBar.topPadding + seekBar.availableHeight / 2 - height / 2
                    implicitWidth: seekBar.hovered || seekBar.pressed ? 17 : 13
                    implicitHeight: implicitWidth
                    radius: width / 2
                    color: "#FFFFFF"
                    border.width: 3
                    border.color: Theme.cyan
                    Behavior on implicitWidth { NumberAnimation { duration: Theme.animationFast } }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 5

                IconButton {
                    glyph: "\u23EE"
                    toolTip: "Previous playlist item"
                    darkSurface: true
                    enabled: player.playlist.length > 0
                    onClicked: player.playPreviousPlaylistItem()
                }

                IconButton {
                    glyph: "-10"
                    toolTip: "Back 10 seconds"
                    darkSurface: true
                    enabled: player.hasMedia && !player.imageMode
                    onClicked: player.jumpSeconds(-10)
                }

                IconButton {
                    glyph: player.playing ? "\u2016" : "\u25B6"
                    toolTip: player.playing ? "Pause" : "Play"
                    prominent: true
                    buttonSize: 48
                    enabled: player.hasMedia && !player.imageMode
                    onClicked: player.togglePlayback()
                }

                IconButton {
                    glyph: "+10"
                    toolTip: "Forward 10 seconds"
                    darkSurface: true
                    enabled: player.hasMedia && !player.imageMode
                    onClicked: player.jumpSeconds(10)
                }

                IconButton {
                    glyph: "\u23ED"
                    toolTip: "Next playlist item"
                    darkSurface: true
                    enabled: player.playlist.length > 0
                    onClicked: player.playNextPlaylistItem()
                }

                Text {
                    Layout.leftMargin: 8
                    text: TimeFormat.time(player.position) + "  /  " + TimeFormat.time(player.duration)
                    color: "#FFFFFF"
                    font.pixelSize: Theme.fontScale * 13
                    font.weight: Font.DemiBold
                }

                Item { Layout.fillWidth: true }

                IconButton {
                    glyph: "\u25C1"
                    toolTip: "Previous frame (,)"
                    darkSurface: true
                    enabled: player.hasMedia && !player.imageMode
                    onClicked: player.stepFrame(-1)
                }

                IconButton {
                    glyph: "\u25B7"
                    toolTip: "Next frame (.)"
                    darkSurface: true
                    enabled: player.hasMedia && !player.imageMode
                    onClicked: player.stepFrame(1)
                }

                IconButton {
                    label: Number(player.playbackRate).toFixed(player.playbackRate % 1 === 0 ? 1 : 2) + "x"
                    toolTip: "Playback speed"
                    darkSurface: true
                    active: player.playbackRate !== 1
                    accentColor: Theme.cyan
                    onClicked: speedMenu.popup()
                }

                IconButton {
                    label: "Sub"
                    toolTip: player.hasSubtitles ? player.subtitleFileName : "Load or configure subtitles"
                    darkSurface: true
                    active: player.hasSubtitles
                    accentColor: Theme.amber
                    enabled: player.hasMedia && player.mediaKind === "Video"
                    onClicked: root.subtitleRequested()
                }

                IconButton {
                    glyph: "CAP"
                    toolTip: "Capture native lossless frame"
                    darkSurface: true
                    accentColor: Theme.green
                    active: player.hasMedia
                    enabled: player.hasMedia
                    onClicked: player.captureOriginalFrame()
                }

                IconButton {
                    glyph: player.muted || player.volume <= 0.01 ? "MUTE" : "VOL"
                    toolTip: player.muted ? "Unmute" : "Mute"
                    darkSurface: true
                    onClicked: player.toggleMute()
                }

                Slider {
                    id: volumeBar
                    Layout.preferredWidth: 92
                    from: 0
                    to: 2
                    value: player.volume
                    onMoved: player.setVolume(value)

                    background: Rectangle {
                        x: volumeBar.leftPadding
                        y: volumeBar.topPadding + volumeBar.availableHeight / 2 - height / 2
                        width: volumeBar.availableWidth
                        height: 4
                        radius: 2
                        color: "#4DFFFFFF"

                        Rectangle {
                            width: volumeBar.visualPosition * parent.width
                            height: parent.height
                            radius: 2
                            color: player.volume > 1 ? Theme.amber : Theme.cyan
                        }
                    }

                    handle: Rectangle {
                        x: volumeBar.leftPadding + volumeBar.visualPosition * (volumeBar.availableWidth - width)
                        y: volumeBar.topPadding + volumeBar.availableHeight / 2 - height / 2
                        implicitWidth: 12
                        implicitHeight: 12
                        radius: 6
                        color: "#FFFFFF"
                    }
                }

                IconButton {
                    glyph: "\u2715"
                    toolTip: "Leave fullscreen (Esc)"
                    darkSurface: true
                    onClicked: root.exitFullscreenRequested()
                }
            }
        }
    }

    ThemedMenu {
        id: speedMenu
        ThemedMenuItem { text: "0.50x"; onTriggered: player.setPlaybackRate(0.5) }
        ThemedMenuItem { text: "0.75x"; onTriggered: player.setPlaybackRate(0.75) }
        ThemedMenuItem { text: "1.00x (Normal)"; onTriggered: player.setPlaybackRate(1.0) }
        ThemedMenuItem { text: "1.25x"; onTriggered: player.setPlaybackRate(1.25) }
        ThemedMenuItem { text: "1.50x"; onTriggered: player.setPlaybackRate(1.5) }
        ThemedMenuItem { text: "2.00x"; onTriggered: player.setPlaybackRate(2.0) }
        ThemedMenuItem { text: "3.00x"; onTriggered: player.setPlaybackRate(3.0) }
        ThemedMenuItem { text: "4.00x"; onTriggered: player.setPlaybackRate(4.0) }
    }
}
