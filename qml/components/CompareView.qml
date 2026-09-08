import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Pro-frame capture & compare: a modal-like split view over the media stage
// showing the untouched decoded frame beside the same frame with every
// enabled plugin filter applied. Frame-accurate stepping (player.stepFrame)
// keeps both sides in sync so the user can pick the exact frame, then export
// either version as a lossless PNG. A "Live preview" toggle lets playback
// continue while both sides keep refreshing at a throttled rate, so the
// filtered pipeline can be watched in motion, not just frame-by-frame.
Item {
    id: root

    visible: player.compareModeActive
    enabled: visible
    property bool livePreview: false

    onVisibleChanged: if (!visible) livePreview = false

    // While playing with Live preview on, re-grab and re-filter both frames
    // on a throttled interval instead of every decoded frame -- keeps the
    // out-of-process Python filter chain from being hammered every ~16ms.
    Timer {
        interval: 350
        repeat: true
        running: root.visible && root.livePreview && player.playing
        onTriggered: player.refreshCompareFrame()
    }

    Rectangle {
        anchors.fill: parent
        color: "#E6060A0F"
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 14

        RowLayout {
            Layout.fillWidth: true

            ColumnLayout {
                spacing: 2
                Text {
                    text: "Compare frame"
                    color: "#FFFFFF"
                    font.pixelSize: Theme.fontScale * 18
                    font.weight: Font.DemiBold
                }
                Text {
                    text: "Step frames with , and . or play with live preview, then save a side"
                    color: "#B8C4D2"
                    font.pixelSize: Theme.fontScale * 10
                }
            }

            Item { Layout.fillWidth: true }

            IconButton {
                glyph: "\u00D7"
                toolTip: "Close compare mode (K)"
                darkSurface: true
                onClicked: player.exitCompareMode()
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 2

            // Original (untouched decoded frame)
            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 8

                RowLayout {
                    Layout.fillWidth: true
                    Text {
                        text: "ORIGINAL"
                        color: "#B8C4D2"
                        font.pixelSize: Theme.fontScale * 10
                        font.weight: Font.DemiBold
                        font.letterSpacing: 1.2
                    }
                    Item { Layout.fillWidth: true }
                    IconButton {
                        label: "Save this frame"
                        buttonSize: 26
                        darkSurface: true
                        onClicked: player.exportCompareFrame(false)
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    radius: 14
                    color: "#14FFFFFF"
                    border.width: 1
                    border.color: "#22FFFFFF"
                    clip: true

                    Image {
                        anchors.fill: parent
                        anchors.margins: 8
                        source: root.visible ? player.compareOriginalSource : ""
                        fillMode: Image.PreserveAspectFit
                        asynchronous: true
                        cache: false
                    }
                }
            }

            Rectangle { Layout.preferredWidth: 1; Layout.fillHeight: true; color: "#22FFFFFF" }

            // Filtered (every enabled plugin applied, same pipeline order as playback/capture)
            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 8

                RowLayout {
                    Layout.fillWidth: true
                    Text {
                        text: "FILTERED"
                        color: "#B8C4D2"
                        font.pixelSize: Theme.fontScale * 10
                        font.weight: Font.DemiBold
                        font.letterSpacing: 1.2
                    }
                    Text {
                        visible: player.compareHasFilters
                        text: root.livePreview ? "LIVE" : "PIPELINE"
                        color: Theme.green
                        font.pixelSize: Theme.fontScale * 9
                        font.weight: Font.DemiBold
                    }
                    Item { Layout.fillWidth: true }
                    IconButton {
                        label: "Save this frame"
                        buttonSize: 26
                        darkSurface: true
                        enabled: player.compareHasFilters
                        active: true
                        accentColor: Theme.cyan
                        onClicked: player.exportCompareFrame(true)
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    radius: 14
                    color: "#14FFFFFF"
                    border.width: 1
                    border.color: "#22FFFFFF"
                    clip: true

                    Image {
                        anchors.fill: parent
                        anchors.margins: 8
                        visible: player.compareHasFilters
                        source: root.visible ? player.compareFilteredSource : ""
                        fillMode: Image.PreserveAspectFit
                        asynchronous: true
                        cache: false
                    }

                    ColumnLayout {
                        anchors.centerIn: parent
                        visible: !player.compareHasFilters
                        spacing: 6
                        Text {
                            Layout.alignment: Qt.AlignHCenter
                            text: "No filters active"
                            color: "#B8C4D2"
                            font.pixelSize: Theme.fontScale * 13
                            font.weight: Font.DemiBold
                        }
                        Text {
                            Layout.alignment: Qt.AlignHCenter
                            text: "Enable a plugin in Tools to see a live comparison"
                            color: "#7C8B96"
                            font.pixelSize: Theme.fontScale * 10
                        }
                    }
                }
            }
        }

        // Frame-accurate filmstrip controls, on two fixed rows so they never
        // clip or overlap even when the window/panel is narrow (e.g. with
        // Library and Tools both open).
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: filmstripColumn.implicitHeight + 16
            radius: 14
            color: "#1AFFFFFF"
            border.width: 1
            border.color: "#22FFFFFF"

            ColumnLayout {
                id: filmstripColumn
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                anchors.margins: 8
                spacing: 6

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    IconButton {
                        glyph: player.playing ? "\u2016" : "\u25B6"
                        toolTip: player.playing ? "Pause" : "Play"
                        darkSurface: true
                        enabled: !player.imageMode
                        onClicked: player.togglePlayback()
                    }
                    IconButton {
                        glyph: "\u276E"
                        toolTip: "Previous frame (,)"
                        darkSurface: true
                        enabled: !player.imageMode
                        onClicked: player.stepFrame(-1)
                    }
                    IconButton {
                        glyph: "\u276F"
                        toolTip: "Next frame (.)"
                        darkSurface: true
                        enabled: !player.imageMode
                        onClicked: player.stepFrame(1)
                    }
                    IconButton {
                        label: "Live preview"
                        toolTip: "Keep refreshing both sides while playing"
                        darkSurface: true
                        enabled: !player.imageMode && player.compareHasFilters
                        active: root.livePreview
                        accentColor: Theme.green
                        onClicked: root.livePreview = !root.livePreview
                    }

                    Text {
                        text: TimeFormat.time(player.position) + " / " + TimeFormat.time(player.duration)
                        color: "#FFFFFF"
                        font.pixelSize: Theme.fontScale * 11
                        font.weight: Font.Medium
                        Layout.leftMargin: 6
                    }

                    Item { Layout.fillWidth: true }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    IconButton {
                        label: "Save original"
                        glyph: "\u2913"
                        darkSurface: true
                        onClicked: player.exportCompareFrame(false)
                    }
                    IconButton {
                        label: "Save filtered"
                        glyph: "\u2913"
                        prominent: true
                        accentColor: Theme.cyan
                        enabled: player.compareHasFilters
                        onClicked: player.exportCompareFrame(true)
                    }

                    Item { Layout.fillWidth: true }
                }
            }
        }
    }
}
