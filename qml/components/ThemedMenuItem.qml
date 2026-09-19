import QtQuick
import QtQuick.Controls

MenuItem {
    id: control

    implicitWidth: Math.max(260, contentItem.implicitWidth + leftPadding + rightPadding + (control.subMenu ? 18 : 0))
    implicitHeight: Math.max(34, contentItem.implicitHeight + topPadding + bottomPadding)
    padding: 8
    leftPadding: 14
    rightPadding: 16
    spacing: 8

    contentItem: Text {
        text: control.text
        font: control.font
        color: control.enabled ? Theme.text : Theme.textMuted
        wrapMode: Text.NoWrap
        elide: Text.ElideNone
        verticalAlignment: Text.AlignVCenter
        rightPadding: control.subMenu ? 16 : 0
    }

    arrow: Text {
        x: control.width - width - control.rightPadding
        y: control.topPadding + (control.availableHeight - height) / 2
        visible: control.subMenu
        text: "\u276F"
        font.pixelSize: Theme.fontScale * 11
        color: Theme.textSecondary
    }

    indicator: Text {
        x: control.leftPadding
        y: control.topPadding + (control.availableHeight - height) / 2
        visible: control.checkable && control.checked
        text: "\u2713"
        font.pixelSize: Theme.fontScale * 13
        color: Theme.cyan
    }

    background: Rectangle {
        implicitWidth: 260
        radius: Theme.radiusSmall
        color: control.highlighted ? Theme.surfaceHover : "transparent"
    }
}
