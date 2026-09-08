import QtQuick

Rectangle {
    id: root

    property color panelColor: Theme.surface
    property color strokeColor: Theme.borderSoft
    property real panelOpacity: 0.94

    color: Qt.rgba(panelColor.r, panelColor.g, panelColor.b, panelOpacity)
    radius: Theme.radiusLarge
    border.width: 1
    border.color: strokeColor

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 1
        height: 1
        radius: parent.radius
        color: Theme.light ? "#16000000" : "#18FFFFFF"
    }
}
