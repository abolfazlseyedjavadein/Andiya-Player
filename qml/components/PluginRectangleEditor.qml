import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    required property rect picture
    property rect selection
    property real minimumWidth: 0.001
    property real minimumHeight: 0.001
    property bool pluginOwned: false
    signal accepted(rect region)
    signal canceled()
    signal workerFailed()

    PluginActionSession {
        id: session
        onSelectionChanged: function(selection) { root.selection = selection }
        onFailed: root.workerFailed()
    }

    function clamp(value, minimum, maximum) {
        return Math.min(Math.max(value, minimum), maximum)
    }

    function start(region, minWidth, minHeight, script, parameters, actionId) {
        pluginOwned = !!script
        if (pluginOwned) {
            selection = Qt.rect(0, 0, 0, 0)
            session.start(script, parameters, actionId, true)
            session.request({ type: "open", selection: [region.x, region.y, region.width, region.height] })
            return
        }
        minimumWidth = Math.max(0.001, minWidth)
        minimumHeight = Math.max(0.001, minHeight)
        const x = clamp(region.x, 0, 1 - minimumWidth)
        const y = clamp(region.y, 0, 1 - minimumHeight)
        selection = Qt.rect(x, y, clamp(region.width, minimumWidth, 1 - x),
                                  clamp(region.height, minimumHeight, 1 - y))
    }

    function stop() { session.stop() }

    MouseArea {
        id: drawing

        x: root.picture.x
        y: root.picture.y
        width: root.picture.width
        height: root.picture.height
        hoverEnabled: true

        property string operation: ""
        property point origin
        property rect initial
        property bool dragged: false
        readonly property bool overHandle: handle.contains(mapToItem(handle, mouseX, mouseY))
        readonly property bool overBox: box.contains(mapToItem(box, mouseX, mouseY))
        cursorShape: overHandle ? Qt.SizeFDiagCursor : overBox ? Qt.SizeAllCursor : Qt.CrossCursor

        function normalized(px, py) {
            return Qt.point(root.clamp(px / width, 0, 1), root.clamp(py / height, 0, 1))
        }

        onPressed: function(mouse) {
            if (session.busy) {
                mouse.accepted = false
                return
            }
            origin = normalized(mouse.x, mouse.y)
            initial = root.selection
            dragged = false
            if (root.pluginOwned) return
            const hitHandle = handle.contains(mapToItem(handle, mouse.x, mouse.y))
            const hitBox = box.contains(mapToItem(box, mouse.x, mouse.y))
            operation = hitHandle ? "resize" : hitBox ? "move" : "draw"
        }
        function forwardDrag(mouse) {
            const point = normalized(mouse.x, mouse.y)
            session.request({
                type: "drag",
                selection: [initial.x, initial.y, initial.width, initial.height],
                origin: [origin.x, origin.y],
                position: [point.x, point.y],
                handleSize: [Math.min(13 / width, initial.width), Math.min(13 / height, initial.height)]
            })
        }
        onPositionChanged: function(mouse) {
            if (!pressed) return
            if (root.pluginOwned) {
                dragged = true
                forwardDrag(mouse)
                return
            }
            const point = normalized(mouse.x, mouse.y)
            if (operation === "move") {
                root.selection = Qt.rect(
                    root.clamp(initial.x + point.x - origin.x, 0, 1 - initial.width),
                    root.clamp(initial.y + point.y - origin.y, 0, 1 - initial.height),
                    initial.width, initial.height)
            } else if (operation === "resize") {
                root.selection = Qt.rect(initial.x, initial.y,
                    root.clamp(initial.width + point.x - origin.x, root.minimumWidth, 1 - initial.x),
                    root.clamp(initial.height + point.y - origin.y, root.minimumHeight, 1 - initial.y))
            } else {
                const x = Math.min(origin.x, point.x, 1 - root.minimumWidth)
                const y = Math.min(origin.y, point.y, 1 - root.minimumHeight)
                root.selection = Qt.rect(x, y,
                    Math.max(root.minimumWidth, Math.max(origin.x, point.x) - x),
                    Math.max(root.minimumHeight, Math.max(origin.y, point.y) - y))
            }
        }
        onReleased: function(mouse) {
            if (root.pluginOwned && dragged) forwardDrag(mouse)
        }
        onCanceled: {
            if (root.pluginOwned) root.canceled()
            else root.selection = initial
        }

        Rectangle {
            id: box

            x: root.selection.x * drawing.width
            y: root.selection.y * drawing.height
            width: root.selection.width * drawing.width
            height: root.selection.height * drawing.height
            color: "#16FFDE38"
            border.color: "#171717"
            border.width: 5

            Rectangle {
                id: highlight
                anchors.fill: parent
                anchors.margins: 2
                visible: box.width > 4 && box.height > 4
                color: "transparent"
                border.color: "#FFDE38"
                border.width: 3
            }

            Rectangle {
                id: handle
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                width: Math.min(13, parent.width)
                height: Math.min(13, parent.height)
                color: highlight.border.color
                border.color: box.border.color
                border.width: 2
            }
        }
    }

    GlassPanel {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 8
        height: controls.implicitHeight + 12

        RowLayout {
            id: controls
            anchors.fill: parent
            anchors.margins: 6
            Label {
                Layout.fillWidth: true
                text: "Drag outside the box to draw. Drag inside to move, or drag the corner to resize."
                wrapMode: Text.WordWrap
            }
            Button { text: "Cancel"; onClicked: root.canceled() }
            Button {
                text: "Apply rectangle"
                enabled: !session.busy && !drawing.pressed
                onClicked: root.accepted(root.selection)
            }
        }
    }
}
