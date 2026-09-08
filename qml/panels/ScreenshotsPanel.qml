import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

GlassPanel {
    id: root

    signal closeRequested()

    implicitWidth: 330
    panelColor: Theme.surface

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 14

        RowLayout {
            Layout.fillWidth: true

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2

                Text {
                    text: "Screenshots"
                    color: Theme.text
                    font.pixelSize: Theme.fontScale * 17
                    font.weight: Font.DemiBold
                }

                Text {
                    text: screenshots.count === 1 ? "1 captured frame" : screenshots.count + " captured frames"
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontScale * 11
                }
            }

            IconButton {
                glyph: "\u00D7"
                toolTip: "Close screenshots"
                onClicked: root.closeRequested()
            }
        }

        // Prominent preview of the most recent capture with its metadata,
        // since that is almost always what someone opens this panel to check.
        Rectangle {
            Layout.fillWidth: true
            visible: screenshots.hasScreenshots
            implicitHeight: 236
            radius: Theme.radius
            color: Theme.backgroundSoft
            border.width: 1
            border.color: Theme.borderSoft
            clip: true

            Image {
                id: latestImage
                anchors.fill: parent
                source: screenshots.latestUrl
                fillMode: Image.PreserveAspectCrop
                asynchronous: true
                cache: false
            }

            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: 92
                gradient: Gradient {
                    GradientStop { position: 0; color: "#00000000" }
                    GradientStop { position: 1; color: "#CC000000" }
                }
            }

            ColumnLayout {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.margins: 10
                spacing: 5

                Text {
                    Layout.fillWidth: true
                    text: screenshots.latestFileName
                    color: "#FFFFFF"
                    font.pixelSize: Theme.fontScale * 11
                    font.weight: Font.DemiBold
                    elide: Text.ElideMiddle
                }

                Text {
                    Layout.fillWidth: true
                    text: screenshots.latestDateTimeText + "  \u00B7  "
                          + screenshots.latestDimensionsText + "  \u00B7  "
                          + screenshots.latestFileSizeText
                    color: "#D9FFFFFF"
                    font.pixelSize: Theme.fontScale * 9
                    elide: Text.ElideRight
                }

                RowLayout {
                    Layout.topMargin: 4
                    spacing: 6

                    IconButton {
                        glyph: "\u2197"
                        label: "Open"
                        buttonSize: 30
                        darkSurface: true
                        onClicked: screenshots.openExternally(0)
                    }
                    IconButton {
                        glyph: "\u2318"
                        toolTip: "Show in folder"
                        buttonSize: 30
                        darkSurface: true
                        onClicked: screenshots.revealInFolder(0)
                    }
                    IconButton {
                        glyph: "\u22EF"
                        toolTip: "Remove options"
                        buttonSize: 30
                        darkSurface: true
                        onClicked: latestRemoveMenu.popup()
                    }

                    ThemedMenu {
                        id: latestRemoveMenu
                        ThemedMenuItem {
                            text: "Remove from list"
                            onTriggered: screenshots.removeFromList(0)
                        }
                        ThemedMenuItem {
                            text: "Delete permanently"
                            onTriggered: screenshots.deletePermanently(0)
                        }
                    }
                }
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: !screenshots.hasScreenshots
            spacing: 8

            Item { Layout.fillHeight: true }

            Text {
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                text: "No screenshots yet"
                color: Theme.textSecondary
                font.pixelSize: Theme.fontScale * 13
                font.weight: Font.DemiBold
            }

            Text {
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
                text: "Capture a frame from the playback overflow menu or right-click"
                      + " on the video, and it will show up here."
                color: Theme.textMuted
                font.pixelSize: Theme.fontScale * 10
            }

            Item { Layout.fillHeight: true }
        }

        RowLayout {
            Layout.fillWidth: true
            visible: screenshots.hasScreenshots

            Text {
                text: "ALL CAPTURES"
                color: Theme.textMuted
                font.pixelSize: Theme.fontScale * 9
                font.weight: Font.DemiBold
                font.letterSpacing: 1.25
            }

            Item { Layout.fillWidth: true }

            Text {
                text: screenshots.directory
                color: Theme.textMuted
                font.pixelSize: Theme.fontScale * 9
                elide: Text.ElideMiddle
                Layout.maximumWidth: 150
                horizontalAlignment: Text.AlignRight
            }
        }

        ListView {
            id: listView
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: screenshots.hasScreenshots
            clip: true
            spacing: 8
            model: screenshots
            boundsBehavior: Flickable.StopAtBounds

            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

            delegate: Rectangle {
                id: card
                required property int index
                required property string filePath
                required property string fileUrl
                required property string fileName
                required property string dateTimeText
                required property string fileSizeText
                required property string dimensionsText

                width: listView.width
                height: 62
                radius: 12
                color: cardHover.hovered ? Theme.surfaceHover : Theme.surfaceRaised

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 8
                    spacing: 10

                    Rectangle {
                        width: 46
                        height: 46
                        radius: 9
                        clip: true
                        color: Theme.backgroundSoft

                        Image {
                            anchors.fill: parent
                            source: card.fileUrl
                            fillMode: Image.PreserveAspectCrop
                            asynchronous: true
                            cache: false
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 3

                        Text {
                            Layout.fillWidth: true
                            text: card.fileName
                            color: Theme.text
                            font.pixelSize: Theme.fontScale * 11
                            font.weight: Font.Medium
                            elide: Text.ElideMiddle
                        }

                        Text {
                            Layout.fillWidth: true
                            text: card.dateTimeText + "  \u00B7  " + card.dimensionsText
                                  + "  \u00B7  " + card.fileSizeText
                            color: Theme.textMuted
                            font.pixelSize: Theme.fontScale * 8
                            elide: Text.ElideRight
                        }
                    }

                    IconButton {
                        glyph: "\u2318"
                        toolTip: "Show in folder"
                        buttonSize: 26
                        onClicked: screenshots.revealInFolder(card.index)
                    }

                    IconButton {
                        glyph: "\u22EF"
                        toolTip: "Remove options"
                        buttonSize: 26
                        onClicked: cardRemoveMenu.popup()
                    }

                    ThemedMenu {
                        id: cardRemoveMenu
                        ThemedMenuItem {
                            text: "Remove from list"
                            onTriggered: screenshots.removeFromList(card.index)
                        }
                        ThemedMenuItem {
                            text: "Delete permanently"
                            onTriggered: screenshots.deletePermanently(card.index)
                        }
                    }
                }

                HoverHandler { id: cardHover }
                TapHandler { onTapped: screenshots.openExternally(card.index) }
            }
        }

        IconButton {
            Layout.fillWidth: true
            glyph: "\u00D7"
            label: "Clear all screenshots"
            buttonSize: 40
            danger: true
            enabled: screenshots.hasScreenshots
            onClicked: clearOptions.open()
        }
    }

    Popup {
        id: clearOptions
        modal: true
        focus: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        x: Math.round((root.width - width) / 2)
        y: Math.round((root.height - height) / 2)
        width: 280

        Overlay.modal: Rectangle { color: "#A0060A0F" }

        background: Rectangle {
            radius: Theme.radius
            color: Theme.surface
            border.width: 1
            border.color: Theme.border
        }

        contentItem: ColumnLayout {
            spacing: 12

            Text {
                Layout.fillWidth: true
                text: "Clear " + screenshots.count + " screenshot(s)"
                color: Theme.text
                font.pixelSize: Theme.fontScale * 13
                font.weight: Font.DemiBold
                wrapMode: Text.WordWrap
            }

            Text {
                Layout.fillWidth: true
                text: "Choose whether to just hide them from Andiya, or delete the files from disk."
                color: Theme.textSecondary
                font.pixelSize: Theme.fontScale * 10
                wrapMode: Text.WordWrap
            }

            IconButton {
                Layout.fillWidth: true
                label: "Remove from list (keep files)"
                buttonSize: 36
                onClicked: { screenshots.clearListOnly(); clearOptions.close() }
            }

            IconButton {
                Layout.fillWidth: true
                label: "Delete all permanently"
                buttonSize: 36
                danger: true
                active: true
                onClicked: { deleteAllConfirm.open(); clearOptions.close() }
            }

            IconButton {
                Layout.fillWidth: true
                label: "Cancel"
                buttonSize: 32
                onClicked: clearOptions.close()
            }
        }
    }

    Popup {
        id: deleteAllConfirm
        modal: true
        focus: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        x: Math.round((root.width - width) / 2)
        y: Math.round((root.height - height) / 2)
        width: 260

        Overlay.modal: Rectangle { color: "#A0060A0F" }

        background: Rectangle {
            radius: Theme.radius
            color: Theme.surface
            border.width: 1
            border.color: Theme.border
        }

        contentItem: ColumnLayout {
            spacing: 14

            Text {
                Layout.fillWidth: true
                text: "Delete all screenshots?"
                color: Theme.text
                font.pixelSize: Theme.fontScale * 13
                font.weight: Font.DemiBold
                wrapMode: Text.WordWrap
            }

            Text {
                Layout.fillWidth: true
                text: "This permanently removes every captured frame from disk. This cannot be undone."
                color: Theme.textSecondary
                font.pixelSize: Theme.fontScale * 10
                wrapMode: Text.WordWrap
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                IconButton {
                    Layout.fillWidth: true
                    label: "Cancel"
                    buttonSize: 34
                    onClicked: deleteAllConfirm.close()
                }
                IconButton {
                    Layout.fillWidth: true
                    label: "Delete all"
                    buttonSize: 34
                    danger: true
                    active: true
                    onClicked: { screenshots.deleteAllPermanently(); deleteAllConfirm.close() }
                }
            }
        }
    }
}
