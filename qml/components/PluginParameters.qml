import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    id: root

    required property var plugin
    required property bool canDrawRectangle
    signal rectangleRequested(var plugin)
    signal actionRequested(var plugin, var action)
    readonly property var actions: root.plugin?.actions ?? []

    Button {
        text: "Draw rectangle"
        visible: !!root.plugin?.rectangle && root.plugin.available
                 && !root.actions.some(action => action.type === "rectangle")
        enabled: root.canDrawRectangle && (!root.plugin?.rectangleInteraction || root.plugin.enabled)
        onClicked: root.rectangleRequested(root.plugin)
    }
    Repeater {
        model: root.actions
        Button {
            required property var modelData
            text: modelData.label
            enabled: root.plugin.available && root.plugin.enabled
                     && (modelData.type !== "rectangle" || root.canDrawRectangle)
            onClicked: root.actionRequested(root.plugin, modelData)
        }
    }
    Label {
        visible: (!!root.plugin?.rectangleInteraction || root.actions.length > 0) && !root.plugin.enabled
        text: "Enable the plugin to use its actions."
        wrapMode: Text.WordWrap
        Layout.fillWidth: true
    }

    RowLayout {
        Layout.fillWidth: true
        visible: !!root.plugin
        Label { text: "Strength" }
        Slider {
            id: strength
            Layout.fillWidth: true
            Accessible.name: "Strength"
            from: 0
            to: 1
            value: root.plugin?.strength ?? 1
            onMoved: if (!pressed) commit()
            onPressedChanged: if (!pressed) commit()

            function commit() {
                if (root.plugin && value !== root.plugin.strength)
                    pluginModel.setStrength(root.plugin.index, value)
            }
        }
        Label { text: Math.round(strength.value * 100) + "%" }
    }

    Repeater {
        model: root.plugin?.parameters ?? []
        RowLayout {
            id: parameter

            required property var modelData
            readonly property real savedValue: root.plugin?.values[modelData.key] ?? modelData.default ?? 0
            Layout.fillWidth: true

            Label { text: parameter.modelData.label || parameter.modelData.key }
            Slider {
                id: slider
                Layout.fillWidth: true
                Accessible.name: parameter.modelData.label || parameter.modelData.key
                from: parameter.modelData.min ?? 0
                to: parameter.modelData.max ?? 1
                stepSize: parameter.modelData.step || 0.01
                value: parameter.savedValue
                onMoved: if (!pressed) commit()
                onPressedChanged: if (!pressed) commit()

                function commit() {
                    if (root.plugin && value !== parameter.savedValue)
                        pluginModel.setParameter(root.plugin.index, parameter.modelData.key, value)
                }
            }
            Label {
                text: Number(slider.value.toFixed(slider.stepSize >= 1 ? 0 : 3)).toString()
                horizontalAlignment: Text.AlignRight
                Layout.minimumWidth: 44
            }
        }
    }

}
