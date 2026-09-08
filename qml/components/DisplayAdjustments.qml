import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

GlassPanel {
    id: root

    property real brightness: 0
    property real contrast: 0

    signal brightnessEdited(real value)
    signal contrastEdited(real value)
    signal resetRequested()

    implicitHeight: 66
    panelColor: Theme.surface

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 16
        anchors.rightMargin: 12
        spacing: 14

        Text {
            text: "DISPLAY"
            color: Theme.textMuted
            font.pixelSize: Theme.fontScale * 9
            font.weight: Font.DemiBold
            font.letterSpacing: 1.1
        }

        Rectangle {
            width: 1
            height: 28
            color: Theme.borderSoft
        }

        Text {
            text: "Brightness"
            color: Theme.textSecondary
            font.pixelSize: Theme.fontScale * 10
        }

        Slider {
            id: brightnessSlider
            Layout.fillWidth: true
            Layout.maximumWidth: 260
            from: -1
            to: 1
            stepSize: 0.01
            value: root.brightness
            onMoved: root.brightnessEdited(value)
        }

        Text {
            text: Math.round(root.brightness * 100) + "%"
            color: root.brightness === 0 ? Theme.textMuted : Theme.cyan
            font.family: "Consolas"
            font.pixelSize: Theme.fontScale * 10
            horizontalAlignment: Text.AlignRight
            Layout.preferredWidth: 38
        }

        Text {
            text: "Contrast"
            color: Theme.textSecondary
            font.pixelSize: Theme.fontScale * 10
            Layout.leftMargin: 8
        }

        Slider {
            id: contrastSlider
            Layout.fillWidth: true
            Layout.maximumWidth: 260
            from: -1
            to: 1
            stepSize: 0.01
            value: root.contrast
            onMoved: root.contrastEdited(value)
        }

        Text {
            text: Math.round(root.contrast * 100) + "%"
            color: root.contrast === 0 ? Theme.textMuted : Theme.violet
            font.family: "Consolas"
            font.pixelSize: Theme.fontScale * 10
            horizontalAlignment: Text.AlignRight
            Layout.preferredWidth: 38
        }

        IconButton {
            glyph: "RST"
            toolTip: "Reset display adjustments"
            enabled: Math.abs(root.brightness) > 0.001 || Math.abs(root.contrast) > 0.001
            onClicked: root.resetRequested()
        }
    }
}
