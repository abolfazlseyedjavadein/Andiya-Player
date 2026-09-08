import QtQuick
import QtQuick.Controls

Menu {
    id: control

    topPadding: 6
    bottomPadding: 6

    background: Rectangle {
        implicitWidth: 220
        color: Theme.surfaceRaised
        border.color: Theme.border
        border.width: 1
        radius: Theme.radiusSmall
    }
}
