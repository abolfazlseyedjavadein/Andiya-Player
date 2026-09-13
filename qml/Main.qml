import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

ApplicationWindow {
    id: window

    width: Math.max(980, player.savedWindowWidth)
    height: Math.max(680, player.savedWindowHeight)
    minimumWidth: 980
    minimumHeight: 680
    visible: true
    title: player.hasMedia ? player.title + " - Andiya" : "Andiya"
    color: "transparent"
    flags: Qt.Window | Qt.FramelessWindowHint
           | Qt.WindowSystemMenuHint | Qt.WindowMinimizeButtonHint
           | Qt.WindowMaximizeButtonHint | Qt.WindowCloseButtonHint

    font.pixelSize: 14 * studio.textScale
    palette.window: Theme.background
    palette.windowText: Theme.text
    palette.base: Theme.surface
    palette.alternateBase: Theme.surfaceRaised
    palette.text: Theme.text
    palette.placeholderText: Theme.textMuted
    palette.button: Theme.surfaceRaised
    palette.buttonText: Theme.text
    palette.highlight: Theme.cyan
    palette.highlightedText: Theme.light ? "#FFFFFF" : Theme.background

    property bool pluginsOpen: false
    property bool screenshotsOpen: false
    property bool libraryOpen: true
    property string libraryMode: "Library"
    // Reserve space for both docks and the transport controls on wide windows.
    readonly property bool dualSidebars: width >= 1280

    onDualSidebarsChanged: {
        if (!dualSidebars && (pluginsOpen || screenshotsOpen))
            libraryOpen = false
    }

    function openLibraryPanel(mode) {
        libraryMode = mode
        libraryOpen = true
        if (!dualSidebars) {
            pluginsOpen = false
            screenshotsOpen = false
        }
    }

    function showRightSidebar(panel, opened) {
        pluginsOpen = opened && panel === "tools"
        screenshotsOpen = opened && panel === "captures"
        if (opened && !dualSidebars)
            libraryOpen = false
    }
    property int visibilityBeforeFullscreen: Window.Windowed
    property real displayBrightness: 0
    property real displayContrast: 0

    function toggleFullscreen() {
        if (visibility === Window.FullScreen) {
            visibilityBeforeFullscreen === Window.Maximized ? showMaximized() : showNormal()
        } else {
            visibilityBeforeFullscreen = visibility
            showFullScreen()
        }
    }

    function plainShortcutEnabled() {
        return !activeFocusItem || (activeFocusItem.cursorPosition === undefined && !activeFocusItem.activeFocusOnTab)
    }

    Component.onCompleted: {
        Theme.styleName = player.themeName
        if (player.savedWindowX >= 0)
            window.x = player.savedWindowX
        if (player.savedWindowY >= 0)
            window.y = player.savedWindowY
        if (player.savedWindowMaximized)
            Qt.callLater(window.showMaximized)
    }

    onClosing: function(close) {
        player.saveWindowState(window.x, window.y, window.width, window.height,
                               window.visibility === Window.Maximized)
    }

    Connections {
        target: Theme
        function onStyleNameChanged() { player.setThemeName(Theme.styleName) }
    }

    Rectangle {
        anchors.fill: parent
        color: window.visibility === Window.FullScreen ? "#000000" : Theme.background
        gradient: Gradient {
            orientation: Gradient.Vertical
            GradientStop { position: 0; color: Theme.backgroundSoft }
            GradientStop { position: 0.48; color: Theme.background }
            GradientStop { position: 1; color: Qt.darker(Theme.background, 1.16) }
        }
    }

    Rectangle {
        id: appFrame
        anchors.fill: parent
        anchors.margins: window.visibility === Window.FullScreen ? 0 : 1
        radius: window.visibility === Window.FullScreen ? 0 : 15
        color: window.visibility === Window.FullScreen ? "#000000"
                                                     : Qt.rgba(Theme.background.r, Theme.background.g, Theme.background.b, 0.96)
        border.width: window.visibility === Window.FullScreen ? 0 : 1
        border.color: Theme.border
        clip: true

        Item {
            id: titleBar
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            height: window.visibility === Window.FullScreen ? 0 : 66
            visible: height > 0
            clip: true

            Rectangle {
                anchors.fill: parent
                color: Qt.rgba(Theme.surface.r, Theme.surface.g, Theme.surface.b, 0.76)
                border.width: 0

                Rectangle {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    height: 1
                    color: Theme.borderSoft
                }
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 16
                anchors.rightMargin: 14
                spacing: 12

                Rectangle {
                    width: 36
                    height: 36
                    radius: 10
                    gradient: Gradient {
                        GradientStop { position: 0; color: Theme.cyan }
                        GradientStop { position: 1; color: Theme.violet }
                    }

                    Image {
                        source: "qrc:/assets/andiya-icon.png"
                        anchors.fill: parent
                        anchors.margins: 2
                        fillMode: Image.PreserveAspectFit
                        mipmap: true
                    }
                }

                Row {
                    spacing: 4
                    Text {
                        text: "ANDIYA"
                        color: Theme.cyan
                        font.pixelSize: Theme.fontScale * 16
                        font.weight: Font.Bold
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    Text {
                        text: "Player"
                        color: Theme.text
                        font.pixelSize: Theme.fontScale * 16
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }

                Rectangle { width: 1; height: 28; color: Theme.borderSoft }

                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    Text {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        text: player.hasMedia ? "Filename:  " + player.title
                                              : "Open video, audio or images"
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontScale * 10
                        elide: Text.ElideMiddle
                    }

                    DragHandler {
                        target: null
                        onActiveChanged: if (active) window.startSystemMove()
                    }
                    TapHandler {
                        acceptedButtons: Qt.LeftButton
                        onDoubleTapped: window.visibility === Window.Maximized
                                        ? window.showNormal() : window.showMaximized()
                    }
                }

                IconButton {
                    glyph: "+"
                    label: "Open"
                    active: true
                    buttonSize: 36
                    onClicked: openDialog.open()
                }

                Row {
                    spacing: 2

                    // Windows-style caption buttons (minimize, maximize/restore, close)
                    // rather than macOS-style traffic lights, since this build targets Windows.
                    component CaptionButton: Rectangle {
                        id: capBtn
                        property string glyph: ""
                        property bool restoreIcon: false
                        property string tip: ""
                        property bool danger: false
                        signal activated()

                        width: 46
                        height: 36
                        radius: 6
                        color: hover.hovered ? (danger ? "#E81123" : Theme.surfaceHover) : "transparent"

                        Text {
                            anchors.centerIn: parent
                            visible: !capBtn.restoreIcon
                            text: capBtn.glyph
                            font.family: "Segoe UI"
                            font.pixelSize: Theme.fontScale * 11
                            color: hover.hovered && capBtn.danger ? "#FFFFFF" : Theme.text
                        }

                        Row {
                            anchors.centerIn: parent
                            visible: capBtn.restoreIcon
                            spacing: 2
                            Rectangle { width: 6; height: 6; color: "transparent"; border.width: 1; border.color: Theme.text }
                            Rectangle { width: 6; height: 6; color: "transparent"; border.width: 1; border.color: Theme.text }
                        }

                        HoverHandler { id: hover }
                        TapHandler { onTapped: capBtn.activated() }

                        ToolTip.visible: hover.hovered
                        ToolTip.text: capBtn.tip
                        ToolTip.delay: 550

                        Behavior on color { ColorAnimation { duration: Theme.animationFast } }
                    }

                    CaptionButton {
                        glyph: "\u2212"
                        tip: "Minimize"
                        onActivated: window.showMinimized()
                    }
                    CaptionButton {
                        glyph: "\u25A1"
                        restoreIcon: window.visibility === Window.Maximized
                        tip: window.visibility === Window.Maximized ? "Restore" : "Maximize"
                        onActivated: window.visibility === Window.Maximized
                                     ? window.showNormal() : window.showMaximized()
                    }
                    CaptionButton {
                        glyph: "\u2715"
                        danger: true
                        tip: "Close"
                        onActivated: Qt.quit()
                    }
                }
            }
        }

        RowLayout {
            id: workspace
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: titleBar.bottom
            anchors.bottom: parent.bottom
            anchors.margins: window.visibility === Window.FullScreen ? 0 : 10
            spacing: window.visibility === Window.FullScreen ? 0 : 10

            GlassPanel {
                id: navRail
                visible: window.visibility !== Window.FullScreen
                Layout.fillHeight: true
                Layout.preferredWidth: 76
                panelColor: Theme.surface

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 7
                    spacing: 5

                    NavRailButton {
                        glyph: "LIB"
                        label: "Library"
                        active: libraryPanel.visible && window.libraryMode === "Library"
                        onClicked: window.openLibraryPanel("Library")
                    }
                    NavRailButton {
                        glyph: "\u21B6"
                        label: "History"
                        active: libraryPanel.visible && window.libraryMode === "History"
                        onClicked: window.openLibraryPanel("History")
                    }
                    NavRailButton {
                        glyph: "\u2637"
                        label: "Playlist"
                        active: libraryPanel.visible && window.libraryMode === "Playlist"
                        onClicked: window.openLibraryPanel("Playlist")
                    }
                    NavRailButton {
                        glyph: "\u25B6"
                        label: "Playing"
                        active: !libraryPanel.visible
                        onClicked: window.libraryOpen = false
                    }
                    NavRailButton {
                        glyph: "\u25C7"
                        label: "Tools"
                        active: window.pluginsOpen
                        accentColor: Theme.violet
                        onClicked: window.showRightSidebar("tools", !window.pluginsOpen)
                    }
                    NavRailButton {
                        glyph: "IMG"
                        label: "Captures"
                        active: window.screenshotsOpen
                        accentColor: Theme.amber
                        onClicked: window.showRightSidebar("captures", !window.screenshotsOpen)
                    }

                    Item { Layout.fillHeight: true }
                    Rectangle { Layout.fillWidth: true; height: 1; color: Theme.borderSoft }

                    NavRailButton {
                        glyph: "i"
                        label: "Creator"
                        onClicked: aboutDialog.open()
                    }
                    NavRailButton {
                        glyph: "\u2318"
                        label: "Shortcuts"
                        onClicked: shortcutsDialog.open()
                    }
                    NavRailButton {
                        glyph: "\u2699"
                        label: "Workspace"
                        onClicked: workspaceDialog.open()
                    }
                }
            }

            MediaLibraryPanel {
                id: libraryPanel
                visible: window.visibility !== Window.FullScreen && window.libraryOpen
                Layout.fillHeight: true
                Layout.preferredWidth: visible ? 278 : 0
                mode: window.libraryMode
                onOpenRequested: openDialog.open()
                onAddFilesRequested: playlistDialog.open()
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                // Guards the transport bar's usable width and stops its
                // contents bleeding into the docked side panels (which are
                // declared after this column and would otherwise paint over
                // any overflow) if the window is ever squeezed too tight.
                Layout.minimumWidth: 520
                clip: true
                spacing: window.visibility === Window.FullScreen ? 0 : 9

                MediaStage {
                    id: mediaStage
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    onOpenRequested: openDialog.open()
                    onNetworkStreamRequested: networkStreamDialog.openWith("")
                    onPluginsRequested: window.showRightSidebar("tools", true)
                    onFullscreenRequested: window.toggleFullscreen()
                    onSubtitleRequested: subtitleDialog.open()
                    fullscreenMode: window.visibility === Window.FullScreen
                    brightness: window.displayBrightness
                    contrast: window.displayContrast
                }

                DisplayAdjustments {
                    Layout.fillWidth: true
                    visible: window.visibility !== Window.FullScreen && player.hasMedia
                             && (player.mediaKind === "Video" || player.imageMode)
                    brightness: window.displayBrightness
                    contrast: window.displayContrast
                    onBrightnessEdited: function(value) { window.displayBrightness = value }
                    onContrastEdited: function(value) { window.displayContrast = value }
                    onResetRequested: {
                        window.displayBrightness = 0
                        window.displayContrast = 0
                    }
                }

                TimelineControls {
                    Layout.fillWidth: true
                    visible: window.visibility !== Window.FullScreen
                    hostWindow: window
                    onSubtitleRequested: subtitleDialog.open()
                    onAudioOptionsRequested: settingsDialog.open()
                    onNetworkStreamRequested: networkStreamDialog.openWith("")
                }
            }

            ScreenshotsPanel {
                Layout.fillHeight: true
                Layout.preferredWidth: window.screenshotsOpen && window.visibility !== Window.FullScreen ? 330 : 0
                visible: Layout.preferredWidth > 0
                opacity: visible ? 1 : 0
                onCloseRequested: window.screenshotsOpen = false

                Behavior on Layout.preferredWidth { NumberAnimation { duration: Theme.animation; easing.type: Easing.OutCubic } }
                Behavior on opacity { NumberAnimation { duration: Theme.animation } }
            }

            PluginDrawer {
                Layout.fillHeight: true
                Layout.preferredWidth: window.pluginsOpen && window.visibility !== Window.FullScreen ? 330 : 0
                visible: Layout.preferredWidth > 0
                opacity: visible ? 1 : 0
                onCloseRequested: window.pluginsOpen = false
                onInstallRequested: pluginFolderDialog.open()
                Behavior on Layout.preferredWidth {
                    NumberAnimation { duration: Theme.animation; easing.type: Easing.OutCubic }
                }
            }
        }
    }

    // Invisible edge and corner grips. Qt.FramelessWindowHint removes the OS
    // resize border entirely, so without these the window could only be resized
    // by dragging the tiny geometric corner of appFrame's rounded rectangle.
    Item {
        id: resizeGrips
        anchors.fill: parent
        z: 90
        visible: window.visibility !== Window.FullScreen && window.visibility !== Window.Maximized

        component ResizeGrip: Item {
            id: grip
            property int edges: 0
            property int cursorHint: Qt.ArrowCursor

            HoverHandler { cursorShape: grip.cursorHint }
            DragHandler {
                target: null
                onActiveChanged: if (active) window.startSystemResize(grip.edges)
            }
        }

        ResizeGrip {
            anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top
            height: 5
            edges: Qt.TopEdge
            cursorHint: Qt.SizeVerCursor
        }
        ResizeGrip {
            anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom
            height: 5
            edges: Qt.BottomEdge
            cursorHint: Qt.SizeVerCursor
        }
        ResizeGrip {
            anchors.top: parent.top; anchors.bottom: parent.bottom; anchors.left: parent.left
            width: 5
            edges: Qt.LeftEdge
            cursorHint: Qt.SizeHorCursor
        }
        ResizeGrip {
            anchors.top: parent.top; anchors.bottom: parent.bottom; anchors.right: parent.right
            width: 5
            edges: Qt.RightEdge
            cursorHint: Qt.SizeHorCursor
        }
        ResizeGrip {
            anchors.left: parent.left; anchors.top: parent.top
            width: 12; height: 12
            edges: Qt.LeftEdge | Qt.TopEdge
            cursorHint: Qt.SizeFDiagCursor
        }
        ResizeGrip {
            anchors.right: parent.right; anchors.top: parent.top
            width: 12; height: 12
            edges: Qt.RightEdge | Qt.TopEdge
            cursorHint: Qt.SizeBDiagCursor
        }
        ResizeGrip {
            anchors.left: parent.left; anchors.bottom: parent.bottom
            width: 12; height: 12
            edges: Qt.LeftEdge | Qt.BottomEdge
            cursorHint: Qt.SizeBDiagCursor
        }
        ResizeGrip {
            anchors.right: parent.right; anchors.bottom: parent.bottom
            width: 12; height: 12
            edges: Qt.RightEdge | Qt.BottomEdge
            cursorHint: Qt.SizeFDiagCursor
        }
    }

    Rectangle {
        id: toast
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: visible ? 28 : 10
        width: Math.min(580, toastRow.implicitWidth + 36)
        height: 58
        radius: 15
        color: Theme.light ? "#F8FFFFFF" : "#F0202A37"
        border.width: 1
        border.color: Theme.border
        visible: opacity > 0 && window.visibility !== Window.FullScreen
        opacity: 0
        z: 100

        property string heading: ""
        property string message: ""
        function show(title, body) {
            heading = title
            message = body
            opacity = 1
            hideTimer.restart()
        }

        RowLayout {
            id: toastRow
            anchors.fill: parent
            anchors.leftMargin: 15
            anchors.rightMargin: 12
            spacing: 11
            Text { text: "\u2713"; color: Theme.green; font.pixelSize: Theme.fontScale * 17; font.weight: Font.Bold }
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 1
                Text { Layout.fillWidth: true; text: toast.heading; color: Theme.text; font.pixelSize: Theme.fontScale * 11; font.weight: Font.DemiBold; elide: Text.ElideRight }
                Text { Layout.fillWidth: true; text: toast.message; color: Theme.textSecondary; font.pixelSize: Theme.fontScale * 9; elide: Text.ElideMiddle }
            }
            IconButton {
                visible: player.lastCapturePath.length > 0 && toast.heading === "Frame captured"
                label: "Show"
                buttonSize: 32
                active: true
                accentColor: Theme.green
                onClicked: player.revealLastCapture()
            }
        }
        Timer { id: hideTimer; interval: 4300; onTriggered: toast.opacity = 0 }
        Behavior on opacity { NumberAnimation { duration: Theme.animation } }
    }

    Connections {
        target: player
        function onNotification(title, message) { toast.show(title, message) }
    }

    Dialog {
        id: pluginApproval
        parent: Overlay.overlay
        anchors.centerIn: parent
        width: Math.min(600, parent.width - 40)
        modal: true
        property string pluginId: ""
        property string fingerprint: ""
        property string explanation: ""
        standardButtons: Dialog.Ok | Dialog.Cancel
        contentItem: Label { text: pluginApproval.explanation; wrapMode: Text.WordWrap }
        onAccepted: pluginModel.approvePlugin(pluginId,fingerprint)
    }
    Connections {
        target: pluginModel
        function onApprovalRequired(id,fingerprint,name,details) {
            pluginApproval.pluginId=id
            pluginApproval.fingerprint=fingerprint
            pluginApproval.title="Enable " + name + "?"
            pluginApproval.explanation=details
            pluginApproval.open()
        }
    }
    WorkspaceDialog { id: workspaceDialog; parent: Overlay.overlay }
    SettingsDialog {
        id: settingsDialog
        parent: Overlay.overlay
        onCreatorRequested: { close(); aboutDialog.open() }
        onSubtitleRequested: subtitleDialog.open()
    }
    AboutDialog { id: aboutDialog; parent: Overlay.overlay }
    ShortcutsDialog { id: shortcutsDialog; parent: Overlay.overlay }
    NetworkStreamDialog { id: networkStreamDialog; parent: Overlay.overlay }

    FileDialog {
        id: openDialog
        title: "Open media in Andiya"
        fileMode: FileDialog.OpenFile
        nameFilters: [
            "All supported media (*.mp4 *.mkv *.mov *.avi *.webm *.mp3 *.wav *.flac *.aac *.m4a *.ogg *.opus *.png *.jpg *.jpeg *.webp *.bmp *.gif *.tif *.tiff *.avif)",
            "Video files (*.mp4 *.mkv *.mov *.avi *.webm)",
            "Audio files (*.mp3 *.wav *.flac *.aac *.m4a *.ogg *.opus)",
            "Images (*.png *.jpg *.jpeg *.webp *.bmp *.gif *.tif *.tiff *.avif)",
            "All files (*)"
        ]
        onAccepted: player.openMedia(selectedFile)
    }

    FileDialog {
        id: playlistDialog
        title: "Add media to playlist"
        fileMode: FileDialog.OpenFiles
        nameFilters: ["Media files (*.mp4 *.mkv *.mov *.avi *.webm *.mp3 *.wav *.flac *.m4a *.ogg *.opus)", "All files (*)"]
        onAccepted: {
            for (let index = 0; index < selectedFiles.length; ++index)
                player.addToPlaylist(selectedFiles[index])
        }
    }

    FileDialog {
        id: subtitleDialog
        title: "Load subtitles"
        fileMode: FileDialog.OpenFile
        nameFilters: ["Subtitle files (*.srt *.vtt)", "All files (*)"]
        onAccepted: player.openSubtitle(selectedFile)
    }

    FolderDialog {
        id: pluginFolderDialog
        title: "Choose an Andiya plugin folder"
        onAccepted: {
            const error = pluginModel.installPlugin(selectedFolder)
            error.length > 0 ? toast.show("Plugin installation failed", error)
                             : toast.show("Plugin installed", "The plugin is installed in Andiya.")
        }
    }

    Shortcut { sequence: studio.shortcuts.workspace; onActivated: workspaceDialog.open() }
    Shortcut { sequence: "Ctrl+O"; onActivated: openDialog.open() }
    Shortcut { sequence: "Ctrl+U"; onActivated: networkStreamDialog.openWith("") }
    Shortcut { sequence: studio.shortcuts.play; enabled: window.plainShortcutEnabled(); onActivated: { player.togglePlayback(); mediaStage.revealOverlay() } }
    Shortcut { sequence: studio.shortcuts.capture; enabled: window.plainShortcutEnabled(); onActivated: player.captureOriginalFrame() }
    Shortcut { sequence: "C"; enabled: window.plainShortcutEnabled(); onActivated: player.captureOriginalFrame() }
    Shortcut { sequence: "Ctrl+Shift+S"; onActivated: player.captureOriginalFrame() }
    Shortcut {
        sequence: studio.shortcuts.compare
        enabled: window.plainShortcutEnabled()
        onActivated: player.compareModeActive ? player.exitCompareMode() : player.enterCompareMode()
    }
    Shortcut { sequence: studio.shortcuts.back; enabled: window.plainShortcutEnabled(); onActivated: { player.jumpSeconds(-5); mediaStage.revealOverlay() } }
    Shortcut { sequence: studio.shortcuts.forward; enabled: window.plainShortcutEnabled(); onActivated: { player.jumpSeconds(5); mediaStage.revealOverlay() } }
    Shortcut {
        sequence: studio.shortcuts.previousFrame
        enabled: window.plainShortcutEnabled()
        onActivated: { player.stepFrame(-1); mediaStage.revealOverlay() }
    }
    Shortcut {
        sequence: studio.shortcuts.nextFrame
        enabled: window.plainShortcutEnabled()
        onActivated: { player.stepFrame(1); mediaStage.revealOverlay() }
    }
    Shortcut { sequence: studio.shortcuts.fullscreen; enabled: window.plainShortcutEnabled(); onActivated: window.toggleFullscreen() }
    Shortcut { sequence: "F11"; onActivated: window.toggleFullscreen() }
    Shortcut { sequence: "Ctrl+,"; onActivated: settingsDialog.open() }
    Shortcut { sequence: "F1"; onActivated: shortcutsDialog.open() }
    Shortcut { sequence: "?"; enabled: window.plainShortcutEnabled(); onActivated: shortcutsDialog.open() }
    Shortcut { sequence: "Shift+/"; enabled: window.plainShortcutEnabled(); onActivated: shortcutsDialog.open() }
    Shortcut { sequence: "Ctrl+Alt+S"; onActivated: subtitleDialog.open() }
    Shortcut { sequence: "Ctrl+Up"; onActivated: { player.setVolume(player.volume + 0.05); mediaStage.revealOverlay() } }
    Shortcut { sequence: "Ctrl+Down"; onActivated: { player.setVolume(player.volume - 0.05); mediaStage.revealOverlay() } }
    Shortcut {
        sequence: "Escape"
        onActivated: {
            if (settingsDialog.opened) settingsDialog.close()
            else if (aboutDialog.opened) aboutDialog.close()
                else if (shortcutsDialog.opened) shortcutsDialog.close()
                else if (networkStreamDialog.opened) networkStreamDialog.close()
            else if (player.compareModeActive) player.exitCompareMode()
            else if (window.visibility === Window.FullScreen) window.toggleFullscreen()
            else if (window.pluginsOpen) window.pluginsOpen = false
        }
    }
}
