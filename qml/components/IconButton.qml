import QtQuick
import QtQuick.Controls

Rectangle {
    id: root

    property string glyph: ""
    property string label: ""
    property string toolTip: ""
    property bool active: false
    property bool prominent: false
    property bool danger: false
    // Set when the button sits on a dark scrim over video, where theme text
    // colours would be unreadable in the light styles.
    property bool darkSurface: false
    property color accentColor: Theme.cyan
    property int buttonSize: 40

    readonly property color contentColor: darkSurface ? "#FFFFFF" : Theme.text

    activeFocusOnTab: true
    Accessible.role: Accessible.Button
    Accessible.name: toolTip || label || glyph
    Accessible.onPressAction: if(enabled)clicked()
    Keys.onSpacePressed: if(enabled)clicked()
    Keys.onReturnPressed: if(enabled)clicked()
    signal clicked()

    implicitWidth: label.length > 0 ? contentRow.implicitWidth + 24 : buttonSize
    implicitHeight: buttonSize
    radius: prominent ? buttonSize / 2 : Theme.radiusSmall
    color: {
        if (!enabled)
            return Theme.light ? "#0A000000" : "#0AFFFFFF"
        if (prominent)
            return accentColor
        if (active)
            return Qt.rgba(accentColor.r, accentColor.g, accentColor.b, 0.14)
        if (hover.hovered)
            return danger ? "#22FF6B7D" : (darkSurface ? "#2EFFFFFF" : Theme.surfaceHover)
        return "transparent"
    }
    border.width: activeFocus ? 2 : (active && !prominent ? 1 : 0)
    border.color: activeFocus ? Theme.cyan : Qt.rgba(accentColor.r, accentColor.g, accentColor.b, 0.32)
    opacity: enabled ? 1.0 : 0.42
    scale: tap.pressed ? 0.94 : 1.0

    Behavior on color { ColorAnimation { duration: Theme.animationFast } }
    Behavior on scale { NumberAnimation { duration: Theme.animationFast; easing.type: Easing.OutCubic } }

    Row {
        id: contentRow
        anchors.centerIn: parent
        spacing: label.length > 0 ? 8 : 0

        Text {
            text: root.glyph
            color: root.prominent ? Theme.background : (root.danger && hover.hovered ? Theme.red : (root.active ? root.accentColor : root.contentColor))
            font.family: root.glyph.length > 2 ? "Segoe UI" : "Segoe UI Symbol"
            font.pixelSize: root.glyph.length > 2 ? 9 : (root.prominent ? 18 : 17)
            font.weight: Font.DemiBold
            anchors.verticalCenter: parent.verticalCenter
        }

        Text {
            visible: root.label.length > 0
            text: root.label
            color: root.prominent ? Theme.background : root.contentColor
            font.pixelSize: Theme.fontScale * 13
            font.weight: Font.DemiBold
            anchors.verticalCenter: parent.verticalCenter
        }
    }

    HoverHandler { id: hover; enabled: root.enabled }
    TapHandler {
        id: tap
        enabled: root.enabled
        onTapped: root.clicked()
    }

    ToolTip.visible: hover.hovered && root.toolTip.length > 0
    ToolTip.text: root.toolTip
    ToolTip.delay: 550
}
