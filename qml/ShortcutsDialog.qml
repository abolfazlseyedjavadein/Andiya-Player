import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// A fast, always-available reference for every keyboard shortcut in Andiya.
// Opened via the "?" / F1 key or the keyboard-glyph button in the nav rail,
// so shortcuts never have to be memorized or hunted for in menus.
Popup {
    id: root

    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    width: Math.min(620, parent ? parent.width - 48 : 620)
    height: Math.min(560, parent ? parent.height - 48 : 560)
    x: parent ? Math.round((parent.width - width) / 2) : 0
    y: parent ? Math.round((parent.height - height) / 2) : 0
    padding: 0

    // groups: [{ title, rows: [{ label, keys: [..] }] }]
    readonly property var groups: [
        {
            title: "Playback",
            rows: [
                { label: "Play / pause", keys: ["Space"] },
                { label: "Seek back / forward 5s", keys: ["\u2190", "\u2192"] },
                { label: "Previous / next frame", keys: [",", "."] },
                { label: "Volume up / down", keys: ["Ctrl", "\u2191/\u2193"] }
            ]
        },
        {
            title: "Capture",
            rows: [
                { label: "Capture with active filters", keys: ["S"] },
                { label: "Capture untouched frame", keys: ["Ctrl", "Shift", "S"] },
                { label: "Compare original vs. filtered", keys: ["K"] }
            ]
        },
        {
            title: "Window & view",
            rows: [
                    { label: "Open media", keys: ["Ctrl", "O"] },
                    { label: "Open network stream (RTSP/RTMP/HTTP)", keys: ["Ctrl", "U"] },
                { label: "Toggle fullscreen", keys: ["F"] },
                { label: "Toggle fullscreen (alt)", keys: ["F11"] },
                { label: "Open settings", keys: ["Ctrl", ","] },
                { label: "Load subtitles", keys: ["Ctrl", "Alt", "S"] },
                { label: "Close panel / exit fullscreen", keys: ["Esc"] },
                { label: "Show this shortcut list", keys: ["?"] }
            ]
        }
    ]

    Overlay.modal: Rectangle { color: "#A006090D" }

    background: Rectangle {
        radius: 24
        color: Theme.surface
        border.width: 1
        border.color: Theme.border
    }

    contentItem: ColumnLayout {
        anchors.fill: parent
        anchors.margins: 26
        spacing: 16

        RowLayout {
            Layout.fillWidth: true

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2
                Text {
                    text: "Keyboard shortcuts"
                    color: Theme.text
                    font.pixelSize: Theme.fontScale * 20
                    font.weight: Font.DemiBold
                }
                Text {
                    text: "Press ? anytime to open this list"
                    color: Theme.textMuted
                    font.pixelSize: Theme.fontScale * 10
                }
            }

            IconButton {
                glyph: "\u00D7"
                toolTip: "Close"
                onClicked: root.close()
            }
        }

        Flickable {
            Layout.fillWidth: true
            Layout.fillHeight: true
            contentHeight: groupsColumn.implicitHeight
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

            ColumnLayout {
                id: groupsColumn
                width: parent.width
                spacing: 16

                Repeater {
                    model: root.groups

                    ColumnLayout {
                        id: groupDelegate
                        Layout.fillWidth: true
                        spacing: 8

                        required property var modelData

                        Text {
                            text: groupDelegate.modelData.title
                            color: Theme.textSecondary
                            font.pixelSize: Theme.fontScale * 11
                            font.weight: Font.DemiBold
                            font.letterSpacing: 0.6
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            implicitHeight: rowsColumn.implicitHeight + 12
                            radius: 14
                            color: Theme.backgroundSoft
                            border.width: 1
                            border.color: Theme.borderSoft

                            ColumnLayout {
                                id: rowsColumn
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.top: parent.top
                                anchors.margins: 6
                                spacing: 2

                                Repeater {
                                    model: groupDelegate.modelData.rows

                                    RowLayout {
                                        Layout.fillWidth: true
                                        Layout.preferredHeight: 34
                                        spacing: 10

                                        required property var modelData

                                        Text {
                                            Layout.fillWidth: true
                                            Layout.leftMargin: 8
                                            text: modelData.label
                                            color: Theme.text
                                            font.pixelSize: Theme.fontScale * 12
                                            elide: Text.ElideRight
                                        }

                                        RowLayout {
                                            spacing: 4
                                            Layout.rightMargin: 8

                                            Repeater {
                                                model: modelData.keys

                                                Rectangle {
                                                    required property string modelData
                                                    implicitWidth: Math.max(26, keyLabel.implicitWidth + 14)
                                                    implicitHeight: 22
                                                    radius: 6
                                                    color: Theme.surfaceRaised
                                                    border.width: 1
                                                    border.color: Theme.border

                                                    Text {
                                                        id: keyLabel
                                                        anchors.centerIn: parent
                                                        text: parent.modelData
                                                        color: Theme.text
                                                        font.pixelSize: Theme.fontScale * 10
                                                        font.weight: Font.DemiBold
                                                        font.family: "Consolas"
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
