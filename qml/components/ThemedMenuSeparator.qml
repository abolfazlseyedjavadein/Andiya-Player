import QtQuick
import QtQuick.Controls

MenuSeparator {
    id: control

    implicitHeight: visible ? contentItem.implicitHeight + topPadding + bottomPadding : 0

    topPadding: 5
    bottomPadding: 5
    leftPadding: 10
    rightPadding: 10

    contentItem: Rectangle {
        implicitHeight: 1
        color: Theme.borderSoft
    }
}
