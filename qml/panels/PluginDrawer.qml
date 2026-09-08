import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

GlassPanel {
    id: root

    signal closeRequested()
    signal installRequested()

    FilterGraphDialog { id: filterGraphDialog; parent: Overlay.overlay }

    implicitWidth: 344
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
                    text: "Tools"
                    color: Theme.text
                    font.pixelSize: Theme.fontScale * 17
                    font.weight: Font.DemiBold
                }

                Text {
                    text: "Filter pipeline and intelligent tools"
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontScale * 11
                }
            }

            IconButton {
                glyph: "\u00D7"
                toolTip: "Close tools"
                onClicked: root.closeRequested()
            }
        }

        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 38
            radius: Theme.radiusSmall
            color: Theme.backgroundSoft
            border.width: 1
            border.color: Theme.borderSoft

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 8
                spacing: 8

                Text {
                    text: "\u2315"
                    color: Theme.textMuted
                    font.pixelSize: Theme.fontScale * 16
                }

                TextField {
                    id: searchField
                    Layout.fillWidth: true
                    placeholderText: "Find a plugin"
                    color: Theme.text
                    placeholderTextColor: Theme.textMuted
                    font.pixelSize: Theme.fontScale * 12
                    selectByMouse: true
                    background: Item {}
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true

            Text {
                text: "FILTER GRAPH"
                color: Theme.textMuted
                font.pixelSize: Theme.fontScale * 9
                font.weight: Font.DemiBold
                font.letterSpacing: 1.25
            }

            Item { Layout.fillWidth: true }

            IconButton {
                glyph: "\u2922"
                toolTip: "Expand filter graph (for long filter chains)"
                buttonSize: 22
                onClicked: filterGraphDialog.open()
            }
        }

        // Visualizes the active image-filter pipeline as a left-to-right
        // stack: Source -> enabled filter 1 -> enabled filter 2 -> Output.
        // This mirrors the exact order applyImageFilters() chains filters in
        // (PluginListModel.cpp), so what's shown here is what actually runs.
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 56
            radius: Theme.radiusSmall
            color: Theme.backgroundSoft
            border.width: 1
            border.color: Theme.borderSoft

            Flickable {
                anchors.fill: parent
                anchors.margins: 8
                contentWidth: graphRow.implicitWidth
                clip: true
                boundsBehavior: Flickable.StopAtBounds
                flickableDirection: Flickable.HorizontalFlick

                RowLayout {
                    id: graphRow
                    height: parent.height
                    spacing: 6

                    Rectangle {
                        implicitWidth: sourceLabel.implicitWidth + 16
                        implicitHeight: 30
                        radius: 8
                        color: Theme.surfaceRaised
                        border.width: 1
                        border.color: Theme.border
                        Text {
                            id: sourceLabel
                            anchors.centerIn: parent
                            text: "Source"
                            color: Theme.textSecondary
                            font.pixelSize: Theme.fontScale * 10
                            font.weight: Font.DemiBold
                        }
                    }

                    Repeater {
                        model: pluginModel

                        RowLayout {
                            id: nodeDelegate
                            required property int index
                            required property string name
                            required property string accent
                            required property bool pluginEnabled
                            required property bool available
                            property color accentColor: accent
                            visible: pluginEnabled && available
                            spacing: 6

                            Text { text: "\u2192"; color: Theme.textMuted; font.pixelSize: Theme.fontScale * 13 }

                            Rectangle {
                                implicitWidth: nodeLabel.implicitWidth + 16
                                implicitHeight: 30
                                radius: 8
                                color: Qt.rgba(1, 1, 1, 0.04)
                                border.width: 1
                                border.color: Qt.rgba(nodeDelegate.accentColor.r, nodeDelegate.accentColor.g, nodeDelegate.accentColor.b, 0.5)

                                Text {
                                    id: nodeLabel
                                    anchors.centerIn: parent
                                    text: nodeDelegate.name
                                    color: nodeDelegate.accentColor
                                    font.pixelSize: Theme.fontScale * 10
                                    font.weight: Font.DemiBold
                                }
                            }
                        }
                    }

                    Text { text: "\u2192"; color: Theme.textMuted; font.pixelSize: Theme.fontScale * 13 }

                    Rectangle {
                        implicitWidth: outputLabel.implicitWidth + 16
                        implicitHeight: 30
                        radius: 8
                        color: Theme.surfaceRaised
                        border.width: 1
                        border.color: Theme.border
                        Text {
                            id: outputLabel
                            anchors.centerIn: parent
                            text: "Output"
                            color: Theme.textSecondary
                            font.pixelSize: Theme.fontScale * 10
                            font.weight: Font.DemiBold
                        }
                    }
                }
            }
        }

        ListView {
            id: listView
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 10
            clip: true
            model: pluginModel
            boundsBehavior: Flickable.StopAtBounds

            ScrollBar.vertical: ScrollBar {
                policy: ScrollBar.AsNeeded
            }

            delegate: Rectangle {
                id: card
                required property int index
                required property string pluginId
                required property string name
                required property string vendor
                required property string description
                required property string capability
                required property string accent
                required property bool pluginEnabled
                required property bool online
                required property bool builtIn
                required property bool available
                required property string loadError
                property color accentColor: accent

                width: listView.width
                height: 126
                radius: Theme.radius
                color: cardHover.hovered ? Theme.surfaceHover : Theme.surface
                border.width: 1
                border.color: pluginEnabled ? Qt.rgba(accentColor.r, accentColor.g, accentColor.b, 0.34) : Theme.borderSoft

                Behavior on color { ColorAnimation { duration: Theme.animationFast } }
                Behavior on border.color { ColorAnimation { duration: Theme.animationFast } }

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 13
                    spacing: 7

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10

                        Rectangle {
                            width: 36
                            height: 36
                            radius: 11
                            color: Qt.rgba(card.accentColor.r, card.accentColor.g, card.accentColor.b, 0.13)

                            Text {
                                anchors.centerIn: parent
                                text: card.online ? "\u2197" : (card.capability.indexOf("AI") >= 0 ? "\u2726" : "\u25C8")
                                color: card.accentColor
                                font.pixelSize: Theme.fontScale * 16
                                font.weight: Font.DemiBold
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 1

                            Text {
                                Layout.fillWidth: true
                                text: card.name
                                color: Theme.text
                                font.pixelSize: Theme.fontScale * 13
                                font.weight: Font.DemiBold
                                elide: Text.ElideRight
                            }

                            Text {
                                text: card.vendor
                                color: Theme.textMuted
                                font.pixelSize: Theme.fontScale * 9
                            }
                        }

                        ColumnLayout {
                            spacing: 0
                            visible: card.capability.indexOf("IMAGE") >= 0

                            IconButton {
                                glyph: "\u2303"
                                toolTip: "Move earlier in pipeline"
                                buttonSize: 20
                                enabled: card.index > 0
                                onClicked: pluginModel.moveFilterUp(card.index)
                            }
                            IconButton {
                                glyph: "\u2304"
                                toolTip: "Move later in pipeline"
                                buttonSize: 20
                                enabled: card.index < listView.count - 1
                                onClicked: pluginModel.moveFilterDown(card.index)
                            }
                        }

                        Switch {
                            id: enableSwitch
                            checked: card.pluginEnabled
                            enabled: card.available

                            ToolTip.visible: hovered && !card.available
                            ToolTip.text: card.loadError

                            onClicked: pluginModel.setPluginEnabled(card.index, checked)

                            indicator: Rectangle {
                                implicitWidth: 36
                                implicitHeight: 20
                                x: enableSwitch.leftPadding
                                y: parent.height / 2 - height / 2
                                radius: 10
                                color: enableSwitch.checked ? card.accentColor : Theme.border

                                Rectangle {
                                    x: enableSwitch.checked ? parent.width - width - 3 : 3
                                    anchors.verticalCenter: parent.verticalCenter
                                    width: 14
                                    height: 14
                                    radius: 7
                                    color: Theme.text

                                    Behavior on x { NumberAnimation { duration: Theme.animationFast; easing.type: Easing.OutCubic } }
                                }

                                Behavior on color { ColorAnimation { duration: Theme.animationFast } }
                            }
                        }
                    }

                    Text {
                        Layout.fillWidth: true
                        text: card.description
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontScale * 10
                        wrapMode: Text.WordWrap
                        maximumLineCount: 2
                        elide: Text.ElideRight
                        lineHeight: 1.22
                    }

                    RowLayout {
                        Layout.fillWidth: true

                        Text {
                            text: card.capability
                            color: card.accentColor
                            font.pixelSize: Theme.fontScale * 8
                            font.weight: Font.DemiBold
                            font.letterSpacing: 0.8
                        }

                        Item { Layout.fillWidth: true }

                        Text {
                            text: card.available ? "LOADED" : (card.online ? "ONLINE" : (card.builtIn ? "PREVIEW" : "UNAVAILABLE"))
                            color: card.available ? Theme.green : Theme.textMuted
                            font.pixelSize: Theme.fontScale * 8
                            font.weight: Font.DemiBold
                            font.letterSpacing: 0.7
                        }
                    }
                }

                HoverHandler { id: cardHover }
            }
        }

        IconButton {
            Layout.fillWidth: true
            glyph: "+"
            label: "Install plugin"
            buttonSize: 40
            active: true
            accentColor: Theme.violet
            onClicked: root.installRequested()
        }
    }
}
