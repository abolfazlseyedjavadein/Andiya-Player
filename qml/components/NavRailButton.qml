import QtQuick
import QtQuick.Layouts

Rectangle {
    id: root

    property string glyph: ""
    property string label: ""
    property bool active: false
    property color accentColor: Theme.cyan
    activeFocusOnTab: true
    Accessible.role: Accessible.Button
    Accessible.name: label || glyph
    Accessible.onPressAction: if(enabled)clicked()
    Keys.onSpacePressed: if(enabled)clicked()
    Keys.onReturnPressed: if(enabled)clicked()
    signal clicked()

    implicitWidth: 62
    implicitHeight: 62
    radius: 11
    color: active ? Qt.rgba(accentColor.r, accentColor.g, accentColor.b, 0.16)
                  : (hover.hovered ? Theme.surfaceHover : "transparent")
    border.width: activeFocus ? 2 : (active ? 1 : 0)
    border.color: activeFocus ? Theme.cyan : Qt.rgba(accentColor.r, accentColor.g, accentColor.b, 0.28)

    ColumnLayout {
        anchors.centerIn: parent
        spacing: 3

        Text {
            Layout.alignment: Qt.AlignHCenter
            text: root.glyph
            color: root.active ? root.accentColor : Theme.textSecondary
            font.family: root.glyph.length > 2 ? "Segoe UI" : "Segoe UI Symbol"
            font.pixelSize: root.glyph.length > 2 ? 12 : 19
            font.weight: Font.DemiBold
        }

        Text {
            Layout.alignment: Qt.AlignHCenter
            text: root.label
            color: root.active ? root.accentColor : Theme.textMuted
            font.pixelSize: Theme.fontScale * 9
            font.weight: root.active ? Font.DemiBold : Font.Normal
        }
    }

    HoverHandler { id: hover }
    TapHandler { onTapped: root.clicked() }
}
