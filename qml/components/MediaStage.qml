import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtMultimedia
import QtQuick.Effects

GlassPanel {
    id: root

    signal openRequested()
    signal networkStreamRequested()
    signal pluginsRequested()
    signal fullscreenRequested()
    signal subtitleRequested()

    property bool fullscreenMode: false
    property real brightness: 0
    property real contrast: 0

    function revealOverlay() { overlay.reveal() }

    clip: true
    radius: fullscreenMode ? 0 : Theme.radiusLarge
    border.width: fullscreenMode ? 0 : 1
    panelColor: fullscreenMode ? "#000000" : Theme.mediaSurface
    strokeColor: fullscreenMode ? "transparent" : (player.hasMedia ? Theme.border : Theme.borderSoft)

    Rectangle {
        anchors.fill: parent
        anchors.margins: root.fullscreenMode ? 0 : 1
        radius: root.fullscreenMode ? 0 : root.radius - 1
        color: root.fullscreenMode ? "#000000" : Theme.mediaSurface

        gradient: Gradient {
            GradientStop { position: 0.0; color: root.fullscreenMode ? "#000000" : Theme.mediaGradientStart }
            GradientStop { position: 0.52; color: root.fullscreenMode ? "#000000" : Theme.mediaGradientMiddle }
            GradientStop { position: 1.0; color: root.fullscreenMode ? "#000000" : Theme.mediaGradientEnd }
        }
    }

    Canvas {
        id: ambience
        anchors.fill: parent
        opacity: player.hasMedia ? 0.10 : 0.55
        visible: !root.fullscreenMode && (!player.hasMedia || player.mediaKind === "Audio")

        onPaint: {
            const ctx = getContext("2d")
            ctx.clearRect(0, 0, width, height)

            const glow = ctx.createRadialGradient(width * 0.5, height * 0.43, 2,
                                                   width * 0.5, height * 0.43, Math.min(width, height) * 0.56)
            glow.addColorStop(0, Qt.rgba(Theme.cyan.r, Theme.cyan.g, Theme.cyan.b, 0.15))
            glow.addColorStop(0.5, Qt.rgba(Theme.cyan.r, Theme.cyan.g, Theme.cyan.b, 0.05))
            glow.addColorStop(1, "transparent")
            ctx.fillStyle = glow
            ctx.fillRect(0, 0, width, height)

            ctx.lineWidth = 1
            ctx.strokeStyle = Qt.rgba(Theme.text.r, Theme.text.g, Theme.text.b, 0.035)
            for (let x = 0; x < width; x += 64) {
                ctx.beginPath(); ctx.moveTo(x, 0); ctx.lineTo(x, height); ctx.stroke()
            }
            for (let y = 0; y < height; y += 64) {
                ctx.beginPath(); ctx.moveTo(0, y); ctx.lineTo(width, y); ctx.stroke()
            }
        }

        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()
        Connections { target: Theme; function onStyleNameChanged() { ambience.requestPaint() } }
    }

    VideoOutput {
        id: videoOutput
        anchors.fill: parent
        anchors.margins: root.fullscreenMode ? 0 : 1
        visible: player.hasMedia && !player.imageMode && player.mediaKind === "Video"
        fillMode: VideoOutput.PreserveAspectFit
        layer.enabled: Math.abs(root.brightness) > 0.001 || Math.abs(root.contrast) > 0.001
        layer.effect: MultiEffect {
            brightness: root.brightness
            contrast: root.contrast
        }

        Component.onCompleted: player.attachVideoOutput(videoOutput)
    }

    Image {
        anchors.fill: parent
        anchors.margins: root.fullscreenMode ? 0 : 20
        visible: player.imageMode
        source: player.imageSource
        fillMode: Image.PreserveAspectFit
        asynchronous: true
        cache: false
        layer.enabled: Math.abs(root.brightness) > 0.001 || Math.abs(root.contrast) > 0.001
        layer.effect: MultiEffect {
            brightness: root.brightness
            contrast: root.contrast
        }
    }

    Rectangle {
        id: subtitleBubble

        // Keeps subtitles clear of the fullscreen control overlay.
        property real overlayLift: overlay.shown ? 104 : 0
        Behavior on overlayLift {
            NumberAnimation { duration: Theme.animation; easing.type: Easing.OutCubic }
        }

        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: Math.max(22, parent.height * player.subtitlePosition / 100) + overlayLift
        width: Math.min(parent.width * 0.82, subtitleLabel.implicitWidth + 40)
        height: subtitleLabel.implicitHeight + 22
        radius: 10
        color: "#C8000000"
        border.width: 1
        border.color: "#45FFFFFF"
        visible: player.subtitleText.length > 0
        enabled: false
        z: 30

        Text {
            id: subtitleLabel
            anchors.centerIn: parent
            width: Math.min(root.width * 0.76, implicitWidth)
            text: player.subtitleText
            color: player.subtitleColor
            font.pixelSize: root.fullscreenMode ? player.subtitleFontSize + 5 : player.subtitleFontSize
            font.weight: Font.Medium
            horizontalAlignment: player.subtitleRtl ? Text.AlignRight : Text.AlignHCenter
            wrapMode: Text.Wrap
            textFormat: Text.PlainText
            style: Text.Outline
            styleColor: "#B0000000"
        }
    }

    Item {
        anchors.fill: parent
        visible: !root.fullscreenMode && !player.hasMedia

        Row {
            anchors.left: parent.left; anchors.top: parent.top; anchors.margins: 22
            spacing: 8
            Rectangle { anchors.verticalCenter: parent.verticalCenter; width: 5; height: 5; radius: 2.5; color: Theme.cyan }
            Text { text: "YOUR SCREENING ROOM"; color: Theme.textMuted; font.pixelSize: 9 * Theme.fontScale; font.letterSpacing: 1.8 }
        }

        ColumnLayout {
            anchors.centerIn: parent
            width: Math.min(530, parent.width - 48)
            spacing: 14

            Item {
                Layout.alignment: Qt.AlignHCenter
                Layout.bottomMargin: 6
                width: 220
                height: 108

                Rectangle {
                    x: 12; y: 27; width: 108; height: 72; radius: 12; rotation: -14
                    color: Qt.rgba(Theme.violet.r, Theme.violet.g, Theme.violet.b, 0.10)
                    border.width: 1; border.color: Qt.rgba(Theme.violet.r, Theme.violet.g, Theme.violet.b, 0.22)
                }
                Rectangle {
                    x: 110; y: 24; width: 98; height: 70; radius: 12; rotation: 13
                    color: Qt.rgba(Theme.amber.r, Theme.amber.g, Theme.amber.b, 0.09)
                    border.width: 1; border.color: Qt.rgba(Theme.amber.r, Theme.amber.g, Theme.amber.b, 0.20)
                }
                Rectangle {
                    anchors.centerIn: parent; width: 144; height: 88; radius: 14
                    color: Theme.surfaceRaised
                    border.width: 1; border.color: Qt.rgba(Theme.cyan.r, Theme.cyan.g, Theme.cyan.b, 0.35)

                    Rectangle {
                        anchors.centerIn: parent; width: 42; height: 42; radius: 21
                        color: Qt.rgba(Theme.cyan.r, Theme.cyan.g, Theme.cyan.b, 0.12)
                        VectorIcon { anchors.centerIn: parent; glyph: "\u25B6"; ink: Theme.cyan; width: 18; height: 18 }
                    }
                    Rectangle { x: 12; y: 72; width: 120; height: 2; radius: 1; color: Theme.borderSoft
                        Rectangle { width: 42; height: 2; radius: 1; color: Theme.cyan }
                    }
                }
            }

            Text {
                Layout.fillWidth: true
                text: "Every frame.\nA little more extraordinary."
                color: Theme.text
                horizontalAlignment: Text.AlignHCenter
                font.pixelSize: Theme.fontScale * (root.width < 640 ? 30 : 38)
                font.weight: Font.DemiBold
                font.letterSpacing: -1.1
                lineHeight: 1.12
                wrapMode: Text.WordWrap
            }

            Text {
                Layout.fillWidth: true
                text: "Watch, explore, and perfect your media.\nDrop a file here to get started."
                color: Theme.textSecondary
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
                lineHeight: 1.35
                font.pixelSize: Theme.fontScale * 14
            }

            RowLayout {
                Layout.alignment: Qt.AlignHCenter
                Layout.topMargin: 8
                spacing: 10

                IconButton {
                    glyph: "+"
                    label: "Open a file"
                    buttonSize: 44
                    prominent: true
                    onClicked: root.openRequested()
                }

                IconButton {
                    glyph: "\u21AF"
                    label: "Open a stream"
                    toolTip: "Open an RTSP/RTMP/HTTP stream URL"
                    buttonSize: 44
                    active: true
                    accentColor: Theme.cyan
                    onClicked: root.networkStreamRequested()
                }
            }

            Row {
                Layout.alignment: Qt.AlignHCenter
                Layout.topMargin: 4
                spacing: 8
                Repeater {
                    model: ["Video", "Audio", "Images"]
                    Rectangle {
                        required property string modelData
                        width: formatLabel.implicitWidth + 22; height: 25; radius: 12
                        color: Qt.rgba(Theme.text.r, Theme.text.g, Theme.text.b, 0.04)
                        border.width: 1; border.color: Theme.borderSoft
                        Text { id: formatLabel; anchors.centerIn: parent; text: modelData; color: Theme.textSecondary; font.pixelSize: 10 * Theme.fontScale }
                    }
                }
            }

            RowLayout {
                Layout.alignment: Qt.AlignHCenter
                Layout.topMargin: 2
                spacing: 6

                Rectangle {
                    implicitWidth: 22
                    implicitHeight: 20
                    radius: 6
                    color: Theme.surfaceRaised
                    border.width: 1
                    border.color: Theme.border
                    Text { anchors.centerIn: parent; text: "?"; color: Theme.textSecondary; font.pixelSize: Theme.fontScale * 10; font.weight: Font.DemiBold }
                }
                Text {
                    text: "Shortcuts at your fingertips"
                    color: Theme.textMuted
                    font.pixelSize: Theme.fontScale * 10
                }
            }
        }
    }

    Item {
        anchors.fill: parent
        visible: !root.fullscreenMode && player.hasMedia && player.mediaKind === "Audio"

        ColumnLayout {
            anchors.centerIn: parent
            spacing: 18

            Rectangle {
                Layout.alignment: Qt.AlignHCenter
                width: 178
                height: 178
                radius: 42
                color: Theme.surfaceRaised
                border.width: 1
                border.color: Theme.border

                Repeater {
                    model: 13
                    Rectangle {
                        required property int index
                        anchors.centerIn: parent
                        width: 5
                        height: 26 + ((index * 23) % 80)
                        radius: 3
                        color: index % 3 === 0 ? Theme.violet : Theme.cyan
                        opacity: 0.52 + (index % 4) * 0.1
                        transform: Translate { x: (index - 6) * 10 }

                        SequentialAnimation on height {
                            running: player.playing
                            loops: Animation.Infinite
                            NumberAnimation { to: 38 + ((index * 31) % 86); duration: 480 + index * 31; easing.type: Easing.InOutSine }
                            NumberAnimation { to: 22 + ((index * 17) % 48); duration: 520 + index * 27; easing.type: Easing.InOutSine }
                        }
                    }
                }
            }

            Text {
                Layout.alignment: Qt.AlignHCenter
                text: player.title
                color: Theme.text
                font.pixelSize: Theme.fontScale * 19
                font.weight: Font.DemiBold
                elide: Text.ElideMiddle
                Layout.maximumWidth: 520
            }

            Text {
                Layout.alignment: Qt.AlignHCenter
                text: "Audio playback"
                color: Theme.textSecondary
                font.pixelSize: Theme.fontScale * 12
            }
        }
    }

    Row {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.margins: 16
        spacing: 8
        visible: !root.fullscreenMode && player.hasMedia

        Rectangle {
            width: kindText.implicitWidth + 20
            height: 28
            radius: 14
            color: Theme.light ? "#E8FFFFFF" : "#B3121821"
            border.width: 1
            border.color: Theme.border

            Text {
                id: kindText
                anchors.centerIn: parent
                text: player.mediaKind.toUpperCase()
                color: Theme.textSecondary
                font.pixelSize: Theme.fontScale * 10
                font.weight: Font.DemiBold
                font.letterSpacing: 1
            }
        }

        Rectangle {
            visible: player.videoWidth > 0
            width: sizeText.implicitWidth + 20
            height: 28
            radius: 14
            color: Theme.light ? "#E8FFFFFF" : "#B3121821"
            border.width: 1
            border.color: Theme.border

            Text {
                id: sizeText
                anchors.centerIn: parent
                text: player.videoWidth + " x " + player.videoHeight
                color: Theme.textSecondary
                font.pixelSize: Theme.fontScale * 10
                font.weight: Font.Medium
            }
        }
    }

    Rectangle {
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 16
        width: statusRow.implicitWidth + 20
        height: 28
        radius: 14
        visible: !root.fullscreenMode && player.hasMedia
        color: Theme.light ? "#E8FFFFFF" : "#B3121821"
        border.width: 1
        border.color: Theme.border

        Row {
            id: statusRow
            anchors.centerIn: parent
            spacing: 7

            Rectangle {
                anchors.verticalCenter: parent.verticalCenter
                width: 6
                height: 6
                radius: 3
                color: player.playing ? Theme.green : Theme.amber
            }
            Text {
                text: player.statusText
                color: Theme.textSecondary
                font.pixelSize: Theme.fontScale * 10
                font.weight: Font.Medium
            }
        }
    }

    DropArea {
        id: dropArea
        anchors.fill: parent

        onEntered: function(drag) {
            if (drag.hasUrls)
                drag.acceptProposedAction()
        }

        onDropped: function(drop) {
            if (!drop.hasUrls || drop.urls.length === 0)
                return

            const dropped = drop.urls[0].toString().toLowerCase()
            if (dropped.endsWith(".srt") || dropped.endsWith(".vtt"))
                player.openSubtitle(drop.urls[0])
            else
                player.openMedia(drop.urls[0])
            drop.acceptProposedAction()
        }
    }

    Rectangle {
        anchors.fill: parent
        anchors.margins: 7
        radius: Theme.radiusLarge - 3
        color: "#145CD6FF"
        border.width: 2
        border.color: Theme.cyan
        visible: !root.fullscreenMode && dropArea.containsDrag

        Text {
            anchors.centerIn: parent
            text: "Drop to open in Andiya"
            color: Theme.text
            font.pixelSize: Theme.fontScale * 18
            font.weight: Font.DemiBold
        }
    }

    ThemedMenu {
        id: contextMenu

        ThemedMenuItem {
            text: "Capture native frame (lossless PNG)"
            enabled: player.hasMedia
            onTriggered: player.captureOriginalFrame()
        }
        ThemedMenuItem {
            text: "Capture frame with active plugins"
            enabled: player.hasMedia
            onTriggered: player.captureFrame()
        }
        ThemedMenuItem {
            text: "Compare original vs. filtered (K)"
            enabled: player.hasMedia
            onTriggered: player.enterCompareMode()
        }
        ThemedMenuItem {
            text: "Open network stream (Ctrl+U)..."
            onTriggered: root.networkStreamRequested()
        }
        ThemedMenuItem {
            text: player.hasSubtitles ? "Change subtitles..." : "Load subtitles..."
            enabled: player.hasMedia && player.mediaKind === "Video"
            onTriggered: root.subtitleRequested()
        }
        ThemedMenuItem {
            text: "Disable subtitles"
            visible: player.hasSubtitles
            onTriggered: player.clearSubtitles()
        }
        ThemedMenuSeparator {}
        ThemedMenuItem {
            text: player.playing ? "Pause" : "Play"
            enabled: player.hasMedia && !player.imageMode
            onTriggered: player.togglePlayback()
        }
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
        ThemedMenuItem {
            text: "Toggle fullscreen (F)"
            enabled: player.hasMedia
            onTriggered: root.fullscreenRequested()
        }
        ThemedMenuSeparator {}
        ThemedMenuItem {
            text: "Remove from player"
            enabled: player.hasMedia
            onTriggered: player.clear()
        }
    }

    // Deferred so a double-click toggles fullscreen without also flipping playback.
    Timer {
        id: singleClickTimer
        interval: 260
        onTriggered: player.togglePlayback()
    }

    MouseArea {
        anchors.fill: parent
        enabled: player.hasMedia
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        propagateComposedEvents: true
        cursorShape: root.fullscreenMode && !overlay.shown ? Qt.BlankCursor : Qt.ArrowCursor

        onClicked: function(mouse) {
            if (mouse.button === Qt.RightButton) {
                singleClickTimer.stop()
                contextMenu.popup(mouse.x, mouse.y)
                return
            }
            if (player.imageMode) {
                mouse.accepted = false
                return
            }
            overlay.reveal()
            singleClickTimer.restart()
        }

        onDoubleClicked: function(mouse) {
            if (mouse.button === Qt.LeftButton) {
                singleClickTimer.stop()
                root.fullscreenRequested()
                mouse.accepted = true
            }
        }
    }

    PlaybackOverlay {
        id: overlay
        anchors.fill: parent
        z: 40
        active: root.fullscreenMode && player.hasMedia
        onExitFullscreenRequested: root.fullscreenRequested()
        onSubtitleRequested: root.subtitleRequested()
    }

    CompareView {
        anchors.fill: parent
        z: 50
    }
}
