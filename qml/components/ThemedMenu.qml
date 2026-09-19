import QtQuick
import QtQuick.Controls

Menu {
    id: control

    topPadding: 6
    bottomPadding: 6
    leftPadding: 4
    rightPadding: 4
    // Grow with the longest item so labels such as "Compare original vs. filtered"
    // are not clipped by the 220px default menu background.
    implicitWidth: Math.max(280, contentWidth + leftPadding + rightPadding)

    background: Rectangle {
        implicitWidth: control.implicitWidth
        color: Theme.surfaceRaised
        border.color: Theme.border
        border.width: 1
        radius: Theme.radiusSmall
    }
}
