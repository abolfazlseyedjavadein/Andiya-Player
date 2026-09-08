import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Lets the user type a network stream URL (RTSP/RTSPS/RTMP/HTTP(S)/...) to
// play directly. RTSP itself is handled entirely by the FFmpeg-based Qt
// Multimedia backend Andiya ships with -- this dialog is just the missing
// piece of UI to reach it (no plugin involved; see PlayerController::
// openNetworkStream).
Popup {
    id: root

    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    width: Math.min(520, parent ? parent.width - 48 : 520)
    x: parent ? Math.round((parent.width - width) / 2) : 0
    y: parent ? Math.round((parent.height - height) / 2) : 0
    padding: 0

    function openWith(prefill) {
        urlField.text = prefill || ""
        root.open()
    }

    onClosed: urlField.clear()
    onOpened: {
        urlField.forceActiveFocus()
        urlField.selectAll()
    }

    Overlay.modal: Rectangle { color: "#A006090D" }

    background: Rectangle {
        radius: 20
        color: Theme.surface
        border.width: 1
        border.color: Theme.border
    }

    contentItem: ColumnLayout {
        width: root.width
        spacing: 14

        RowLayout {
            Layout.fillWidth: true
            Layout.margins: 22
            Layout.topMargin: 20
            Layout.bottomMargin: 0

            ColumnLayout {
                spacing: 2
                Text {
                    text: "Open network stream"
                    color: Theme.text
                    font.pixelSize: Theme.fontScale * 17
                    font.weight: Font.DemiBold
                }
                Text {
                    text: "RTSP, RTMP, or HTTP(S) -- playback uses the bundled FFmpeg backend"
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontScale * 10
                }
            }

            Item { Layout.fillWidth: true }

            IconButton {
                glyph: "\u00D7"
                toolTip: "Close"
                onClicked: root.close()
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 22
            Layout.rightMargin: 22
            spacing: 6

            Rectangle {
                Layout.fillWidth: true
                implicitHeight: 42
                radius: Theme.radiusSmall
                color: Theme.backgroundSoft
                border.width: 1
                border.color: urlField.activeFocus ? Theme.cyan : Theme.borderSoft

                TextField {
                    id: urlField
                    anchors.fill: parent
                    anchors.leftMargin: 12
                    anchors.rightMargin: 12
                    verticalAlignment: TextInput.AlignVCenter
                    placeholderText: "rtsp://192.168.1.10:554/stream1"
                    placeholderTextColor: Theme.textMuted
                    color: Theme.text
                    font.pixelSize: Theme.fontScale * 13
                    selectByMouse: true
                    echoMode: TextInput.PasswordEchoOnEdit
                    background: Item {}
                    onAccepted: openButton.clicked()
                }
            }

            Text {
                Layout.fillWidth: true
                text: "Examples: rtsp://user:pass@host:554/path  \u00B7  rtmp://host/live  \u00B7  https://host/stream.m3u8"
                color: Theme.textMuted
                font.pixelSize: Theme.fontScale * 9
                wrapMode: Text.WrapAtWordBoundaryOrAnywhere
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.margins: 22
            Layout.topMargin: 4
            spacing: 10

            Item { Layout.fillWidth: true }

            IconButton {
                label: "Cancel"
                darkSurface: false
                onClicked: root.close()
            }

            IconButton {
                id: openButton
                label: "Connect"
                prominent: true
                accentColor: Theme.cyan
                onClicked: {
                    if (urlField.text.trim().length === 0) {
                        return
                    }
                    player.openNetworkStream(urlField.text)
                    root.close()
                }
            }
        }
    }
}
