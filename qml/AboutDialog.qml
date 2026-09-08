import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Popup {
    id: root

    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    width: Math.min(560, parent ? parent.width - 48 : 560)
    height: 500
    x: parent ? Math.round((parent.width - width) / 2) : 0
    y: parent ? Math.round((parent.height - height) / 2) : 0
    padding: 0

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
        spacing: 18

        RowLayout {
            Layout.fillWidth: true

            Text {
                Layout.fillWidth: true
                text: "Creator"
                color: Theme.text
                font.pixelSize: Theme.fontScale * 22
                font.weight: Font.DemiBold
            }

            IconButton {
                glyph: "\u00D7"
                toolTip: "Close"
                onClicked: root.close()
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: 20
            color: Theme.backgroundSoft
            border.width: 1
            border.color: Theme.borderSoft

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 24
                spacing: 14

                Rectangle {
                    Layout.alignment: Qt.AlignHCenter
                    width: 76
                    height: 76
                    radius: 24
                    gradient: Gradient {
                        GradientStop { position: 0; color: Theme.cyan }
                        GradientStop { position: 1; color: Theme.violet }
                    }

                    Text {
                        anchors.centerIn: parent
                        text: "AS"
                        color: "#FFFFFF"
                        font.pixelSize: Theme.fontScale * 22
                        font.weight: Font.Bold
                    }
                }

                Text {
                    Layout.fillWidth: true
                    text: "Seyed Abolfazl Seyed Javadein"
                    color: Theme.text
                    wrapMode: Text.WordWrap
                    font.pixelSize: Theme.fontScale * 18
                    font.weight: Font.DemiBold
                    horizontalAlignment: Text.AlignHCenter
                }

                Text {
                    Layout.fillWidth: true
                    text: "Educator and developer focused on deep learning, computer vision, machine learning, image processing and Python."
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontScale * 11
                    wrapMode: Text.WordWrap
                    maximumLineCount: 3
                    horizontalAlignment: Text.AlignHCenter
                }

                Item { Layout.fillHeight: true }

                RowLayout {
                    Layout.alignment: Qt.AlignHCenter
                    spacing: 10

                    IconButton {
                        glyph: "\u2197"
                        label: "CodeTipsAcademy.com"
                        active: true
                        accentColor: Theme.cyan
                        onClicked: Qt.openUrlExternally("https://CodeTipsAcademy.com")
                    }

                    IconButton {
                        glyph: "in"
                        label: "LinkedIn"
                        active: true
                        accentColor: Theme.violet
                        onClicked: Qt.openUrlExternally("https://www.linkedin.com/in/abolfazl-javadein/")
                    }
                }
            }
        }

        Text {
            Layout.fillWidth: true
            text: "Andiya 0.1  |  Open-source media core"
            color: Theme.textMuted
            font.pixelSize: Theme.fontScale * 9
            horizontalAlignment: Text.AlignHCenter
        }
    }
}
