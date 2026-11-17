import QtQuick
import QtQuick.Controls

Menu {
    id: control

    topPadding: 6
    bottomPadding: 6
    leftPadding: 4
    rightPadding: 4
    implicitWidth: {
        let widestItem = 0
        for (let index = 0; index < count; ++index) {
            const item = itemAt(index)
            if (item)
                widestItem = Math.max(widestItem, item.implicitWidth)
        }
        return Math.max(280, Math.ceil(widestItem + leftPadding + rightPadding))
    }
    width: parent && parent.Window.window ? Math.min(implicitWidth, parent.Window.window.width)
                                          : implicitWidth
    delegate: ThemedMenuItem {}

    background: Rectangle {
        color: Theme.surfaceRaised
        border.color: Theme.border
        border.width: 1
        radius: Theme.radiusSmall
    }
}
