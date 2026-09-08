import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// A larger, roomier view of the same filter pipeline shown as a compact
// strip in PluginDrawer's "FILTER GRAPH" row. Nodes wrap onto additional
// lines (via Flow) instead of requiring horizontal scrolling, so a long
// chain of plugins stays fully readable regardless of how many are active.
Popup {
    id: root

    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    width: Math.min(820, parent ? parent.width - 48 : 820)
    height: Math.min(520, parent ? parent.height - 48 : 520)
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
        anchors.margins: 24
        spacing: 16

        RowLayout {
            Layout.fillWidth: true

            ColumnLayout {
                spacing: 2
                Text {
                    text: "Filter graph"
                    color: Theme.text
                    font.pixelSize: Theme.fontScale * 18
                    font.weight: Font.DemiBold
                }
                Text {
                    text: "Every enabled filter, in the exact order applied to playback and captures"
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

        Flickable {
            Layout.fillWidth: true
            Layout.fillHeight: true
            contentHeight: graphFlow.implicitHeight
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

            Flow {
                id: graphFlow
                width: parent.width
                spacing: 10

                Rectangle {
                    implicitWidth: sourceLabel.implicitWidth + 28
                    implicitHeight: 46
                    radius: 12
                    color: Theme.backgroundSoft
                    border.width: 1
                    border.color: Theme.borderSoft
                    Text {
                        id: sourceLabel
                        anchors.centerIn: parent
                        text: "Source"
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontScale * 12
                        font.weight: Font.DemiBold
                    }
                }

                Repeater {
                    id: nodeRepeater
                    model: pluginModel

                    RowLayout {
                        id: nodeDelegate
                        required property int index
                        required property string name
                        required property string vendor
                        required property string accent
                        required property bool pluginEnabled
                        required property bool available
                        property color accentColor: accent
                        visible: pluginEnabled && available
                        spacing: 10

                        Text { text: "\u2192"; color: Theme.textMuted; font.pixelSize: Theme.fontScale * 16 }

                        Rectangle {
                            implicitWidth: Math.max(nodeName.implicitWidth, nodeVendor.implicitWidth) + 28
                            implicitHeight: 46
                            radius: 12
                            color: Qt.rgba(nodeDelegate.accentColor.r, nodeDelegate.accentColor.g, nodeDelegate.accentColor.b, 0.12)
                            border.width: 1
                            border.color: Qt.rgba(nodeDelegate.accentColor.r, nodeDelegate.accentColor.g, nodeDelegate.accentColor.b, 0.55)

                            ColumnLayout {
                                anchors.centerIn: parent
                                spacing: 0
                                Text {
                                    id: nodeName
                                    Layout.alignment: Qt.AlignHCenter
                                    text: nodeDelegate.name
                                    color: nodeDelegate.accentColor
                                    font.pixelSize: Theme.fontScale * 12
                                    font.weight: Font.DemiBold
                                }
                                Text {
                                    id: nodeVendor
                                    Layout.alignment: Qt.AlignHCenter
                                    text: nodeDelegate.vendor
                                    color: Theme.textMuted
                                    font.pixelSize: Theme.fontScale * 8
                                }
                            }
                        }

                        ColumnLayout {
                            spacing: 0
                            IconButton {
                                glyph: "\u2303"
                                toolTip: "Move earlier"
                                buttonSize: 18
                                enabled: nodeDelegate.index > 0
                                onClicked: pluginModel.moveFilterUp(nodeDelegate.index)
                            }
                            IconButton {
                                glyph: "\u2304"
                                toolTip: "Move later"
                                buttonSize: 18
                                enabled: nodeDelegate.index < nodeRepeater.count - 1
                                onClicked: pluginModel.moveFilterDown(nodeDelegate.index)
                            }
                        }
                    }
                }

                Text { text: "\u2192"; color: Theme.textMuted; font.pixelSize: Theme.fontScale * 16 }

                Rectangle {
                    implicitWidth: outputLabel.implicitWidth + 28
                    implicitHeight: 46
                    radius: 12
                    color: Theme.backgroundSoft
                    border.width: 1
                    border.color: Theme.borderSoft
                    Text {
                        id: outputLabel
                        anchors.centerIn: parent
                        text: "Output"
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontScale * 12
                        font.weight: Font.DemiBold
                    }
                }
            }
        }

        Text {
            Layout.fillWidth: true
            text: nodeRepeater.count === 0 ? "No plugins installed yet." : "Tip: use \u2303 / \u2304 here or in the Tools list to reorder the pipeline."
            color: Theme.textMuted
            font.pixelSize: Theme.fontScale * 9
        }
    }
}
