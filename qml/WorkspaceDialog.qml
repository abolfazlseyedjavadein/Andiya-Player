import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

Dialog {
    id: root
    objectName: "workspaceDialog"
    title: "Workspace"
    width: Math.min(1040, parent.width - 40)
    height: Math.min(760, parent.height - 40)
    anchors.centerIn: parent
    modal: true
    standardButtons: Dialog.Close
    background: Rectangle { color: Theme.background; radius: 16; border.color: Theme.border }
    property string message: ""
    contentItem: ColumnLayout {
        spacing: 12
        TabBar {
            id: tabs
            objectName: "workspaceTabs"
            Layout.fillWidth: true
            Repeater { model: ["Bookmarks", "Export", "Filters", "Streams", "Diagnostics", "Preferences"]
                TabButton { required property string modelData; text: modelData }
            }
        }
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            contentWidth: availableWidth
            clip: true
            ColumnLayout {
                width: parent.width
                spacing: 16
                ColumnLayout {
                    visible: tabs.currentIndex === 0
                    Layout.fillWidth: true
                    Label { text: "Mark a moment, add a note, and return to it later."; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                    RowLayout {
                        Layout.fillWidth: true
                        TextField { id: bookmarkNote; placeholderText: "Bookmark name or note"; Layout.fillWidth: true }
                        Button { text: "Add bookmark"; enabled: player.hasMedia && !player.imageMode; onClicked: {player.addBookmark(bookmarkNote.text); bookmarkNote.clear()} }
                    }
                    RowLayout {
                        Button { text: "Set loop A"; enabled: player.seekable; onClicked: player.setLoopStart() }
                        Button { text: "Set loop B"; enabled: player.seekable; onClicked: player.setLoopEnd() }
                        Button { text: "Clear loop"; onClicked: player.clearLoop() }
                        Label { text: player.loopEnabled ? "Loop: " + player.loopStart + "–" + player.loopEnd + " ms" : "Set B after A to repeat a section." }
                    }
                    Label { visible: player.bookmarks.length === 0; text: "No bookmarks yet."; color: Theme.textSecondary }
                    Repeater {
                        model: player.bookmarks
                        Frame {
                            required property var modelData
                            required property int index
                            Layout.fillWidth: true
                            ColumnLayout {
                                anchors.fill: parent
                                RowLayout {
                                    Layout.fillWidth: true
                                    TextField { text: modelData.label; Layout.fillWidth: true; Accessible.name: "Bookmark note"; onEditingFinished: player.editBookmark(index,text) }
                                    Label { text: TimeFormat.time(modelData.position) }
                                    Button { text: "Go"; onClicked: player.jumpBookmark(index) }
                                    Button { text: "Delete"; onClicked: player.removeBookmark(index) }
                                }
                                Label { text: modelData.url; Layout.fillWidth: true; elide: Text.ElideMiddle; color: Theme.textSecondary }
                            }
                        }
                    }
                    RowLayout {
                        Button { text: "Export workspace…"; onClicked: saveWorkspace.open() }
                        Button { text: "Import workspace…"; onClicked: importNotice.open() }
                    }
                    Label { text: "Workspaces include bookmarks, notes, playlists, filter presets and preferences. Media files stay at their current paths."; wrapMode: Text.WordWrap; Layout.fillWidth: true; color: Theme.textSecondary }
                }
                ColumnLayout {
                    visible: tabs.currentIndex === 1
                    Layout.fillWidth: true
                    Label { text: "Lossless, full-resolution export"; font.bold: true; font.pixelSize: 20 * studio.textScale }
                    Label { text: "Capture frames from the current position at regular intervals. Exports use their own decoder, so playback can continue."; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                    RowLayout {
                        Label { text: "Interval (seconds)" }
                        SpinBox { id: interval; from: 1; to: 86400; value: 10; editable: true }
                        Label { text: "Frames (max. 200)" }
                        SpinBox { id: count; from: 1; to: 200; value: 12; editable: true }
                    }
                    CheckBox { id: filtered; text: "Apply the enabled filter stack"; checked: true }
                    CheckBox { id: sheet; text: "Also create a contact sheet with timestamps"; checked: true }
                    RowLayout {
                        Button { text: "Capture interval"; enabled: !player.batchRunning && player.seekable; onClicked: player.startBatchCapture(interval.value,count.value,filtered.checked,sheet.checked) }
                        Button { text: "Export bookmarks"; enabled: !player.batchRunning && player.bookmarks.length > 0; onClicked: player.exportBookmarks(filtered.checked,sheet.checked) }
                        Button { text: "Export images…"; enabled: !player.batchRunning; onClicked: imageFiles.open() }
                    }
                    Label { text: "Image batches use the enabled filters. Completed files are kept when you cancel."; Layout.fillWidth: true; wrapMode: Text.WordWrap; color: Theme.textSecondary }
                    ProgressBar { Layout.fillWidth: true; value: player.batchProgress }
                    Label { text: player.batchStatus || "Choose an export to begin"; Layout.fillWidth: true; wrapMode: Text.WordWrap }
                    RowLayout {
                        Button { text: "Cancel export"; enabled: player.batchRunning; onClicked: player.cancelBatch() }
                        Button { text: "Output folder…"; onClicked: exportFolder.open() }
                        Button { text: "Show files"; onClicked: player.revealCaptureDirectory() }
                    }
                    Label { text: player.captureDirectoryPath; Layout.fillWidth: true; wrapMode: Text.WrapAnywhere }
                }
                ColumnLayout {
                    visible: tabs.currentIndex === 2
                    Layout.fillWidth: true
                    CheckBox { text: "Show filters during video playback"; checked: player.liveFilters; onToggled: player.setLiveFilters(checked) }
                    Label { text: "Playback previews are limited to 960 px. Captures and batch exports retain the decoded resolution. Slow filters skip previews to keep the interface responsive."; wrapMode: Text.WordWrap; Layout.fillWidth: true; color: Theme.textSecondary }
                    RowLayout {
                        Layout.fillWidth: true
                        TextField { id: presetName; placeholderText: "New preset name"; Layout.fillWidth: true }
                        Button { text: "Save preset"; enabled: presetName.text.trim().length > 0; onClicked: pluginModel.savePreset(presetName.text) }
                        ComboBox { id: presets; model: pluginModel.presetNames; Layout.preferredWidth: 190 }
                        Button { text: "Load"; enabled: presets.count > 0; onClicked: pluginModel.loadPreset(presets.currentText) }
                        Button { text: "Delete"; enabled: presets.count > 0; onClicked: pluginModel.deletePreset(presets.currentText) }
                    }
                    Repeater {
                        model: pluginModel.details
                        Frame {
                            id: card
                            required property var modelData
                            Layout.fillWidth: true
                            ColumnLayout {
                                anchors.fill: parent
                                RowLayout {
                                    Layout.fillWidth: true
                                    Label { text: card.modelData.name + "  " + (card.modelData.version || ""); font.bold: true; Layout.fillWidth: true }
                                    Button { text: "↑"; Accessible.name: "Move filter earlier"; onClicked: pluginModel.moveFilterUp(card.modelData.index) }
                                    Button { text: "↓"; Accessible.name: "Move filter later"; onClicked: pluginModel.moveFilterDown(card.modelData.index) }
                                    Switch { text: "Enabled"; checked: card.modelData.enabled; enabled: card.modelData.available; onToggled: pluginModel.setPluginEnabled(card.modelData.index,checked) }
                                }
                                Label { text: card.modelData.publisherStatus || ""; wrapMode: Text.WordWrap; Layout.fillWidth: true; color: Theme.textSecondary }
                                Label { text: "Requested access: " + (card.modelData.permissions || []).join(", "); Layout.fillWidth: true; wrapMode: Text.WordWrap }
                                Label { visible: !!card.modelData.error; text: card.modelData.error || ""; color: Theme.red; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                                RowLayout {
                                    Label { text: "Strength" }
                                    Slider { Layout.fillWidth: true; from: 0; to: 1; value: card.modelData.strength; onPressedChanged: if(!pressed)pluginModel.setStrength(card.modelData.index,value) }
                                    Label { text: Math.round(card.modelData.strength * 100) + "%" }
                                    Button { text: "Restart"; enabled: card.modelData.enabled; onClicked: pluginModel.setPluginEnabled(card.modelData.index,true) }
                                    Button { text: "Rollback"; onClicked: { const error=pluginModel.rollbackPlugin(card.modelData.id); root.message=error || "Previous plugin version restored." } }
                                }
                                Repeater {
                                    model: card.modelData.parameters || []
                                    RowLayout {
                                        required property var modelData
                                        Layout.fillWidth: true
                                        Label { text: modelData.label || modelData.key }
                                        Slider {
                                            Layout.fillWidth: true
                                            from: modelData.min === undefined ? 0 : modelData.min
                                            to: modelData.max === undefined ? 1 : modelData.max
                                            stepSize: modelData.step || 0.01
                                            value: card.modelData.values[modelData.key] === undefined ? modelData.default : card.modelData.values[modelData.key]
                                            onPressedChanged: if(!pressed)pluginModel.setParameter(card.modelData.index,modelData.key,value)
                                        }
                                    }
                                }
                            }
                        }
                    }
                    RowLayout {
                        Button { text: "Install folder…"; onClicked: pluginFolder.open() }
                        Button { text: "Install bundle…"; onClicked: pluginBundle.open() }
                        Button { text: "Rescan"; onClicked: pluginModel.reload() }
                    }
                    Label { text: "Plugin catalog"; font.bold: true }
                    RowLayout {
                        Layout.fillWidth: true
                        TextField { id: catalogAddress; placeholderText: "HTTPS catalog address"; Layout.fillWidth: true }
                        Button { text: "Load"; onClicked: studio.loadCatalog(catalogAddress.text) }
                        Button { text: "Local catalog…"; onClicked: catalogFile.open() }
                    }
                    Repeater {
                        model: studio.catalog
                        RowLayout {
                            required property var modelData
                            required property int index
                            Layout.fillWidth: true
                            Label { text: modelData.name + "  " + (modelData.version || ""); Layout.fillWidth: true }
                            Button { text: "Install / update"; onClicked: studio.installCatalogItem(index) }
                        }
                    }
                }
                ColumnLayout {
                    visible: tabs.currentIndex === 3
                    Layout.fillWidth: true
                    Label { text: "Named stream profiles"; font.bold: true; font.pixelSize: 20 * studio.textScale }
                    TextField { id: profileName; placeholderText: "Profile name"; Layout.fillWidth: true }
                    TextField { id: profileAddress; placeholderText: "Complete stream URL, including credentials if required"; echoMode: TextInput.PasswordEchoOnEdit; Layout.fillWidth: true }
                    RowLayout {
                        Button { text: "Save securely"; enabled: studio.secureProfilesAvailable; onClicked: { studio.saveProfile(profileName.text,profileAddress.text); profileAddress.clear() } }
                        Button { text: "Reconnect current stream"; onClicked: player.reconnectStream() }
                    }
                    Label { text: studio.secureProfilesAvailable ? "Windows encrypts complete profile addresses for your account. History and exported workspaces omit usernames, passwords, query strings and fragments." : "Encrypted profiles are currently available on Windows. Use Open stream for playback on this platform. History and exported workspaces omit URL credentials, query strings and fragments."; Layout.fillWidth: true; wrapMode: Text.WordWrap; color: Theme.textSecondary }
                    Repeater {
                        model: studio.profiles
                        RowLayout {
                            required property var modelData
                            required property int index
                            Layout.fillWidth: true
                            Label { text: modelData.name; font.bold: true }
                            Label { text: modelData.address; elide: Text.ElideMiddle; Layout.fillWidth: true }
                            Button { text: "Play"; onClicked: studio.openProfile(index) }
                            Button { text: "Delete"; onClicked: studio.deleteProfile(index) }
                        }
                    }
                    Label { text: "Transport and codec availability depend on the packaged decoder. HTTP(S) files and adaptive manifests use the same stream dialog. Unsupported device or network transports report an error."; Layout.fillWidth: true; wrapMode: Text.WordWrap }
                }
                ColumnLayout {
                    visible: tabs.currentIndex === 4
                    Layout.fillWidth: true
                    Label { text: "Playback diagnostics"; font.bold: true; font.pixelSize: 20 * studio.textScale }
                    Repeater {
                        model: Object.keys(player.diagnostics)
                        RowLayout {
                            required property string modelData
                            Layout.fillWidth: true
                            Label { text: modelData; Layout.preferredWidth: 210; font.bold: true }
                            Label { text: String(player.diagnostics[modelData] === undefined ? "Unavailable" : player.diagnostics[modelData]); Layout.fillWidth: true; wrapMode: Text.WordWrap }
                        }
                    }
                    Label { text: "If playback fails: check the address or file, credentials, network access and codec availability. A failed filter is bypassed; use Restart on its filter card. The skipped count measures filter previews, not decoder drops."; Layout.fillWidth: true; wrapMode: Text.WordWrap; color: Theme.textSecondary }
                }
                ColumnLayout {
                    visible: tabs.currentIndex === 5
                    Layout.fillWidth: true
                    Label { text: "Appearance"; font.bold: true }
                    RowLayout {
                        Layout.fillWidth: true
                        Label { text: "Theme"; Layout.preferredWidth: 180 }
                        ComboBox {
                            id: workspaceThemePicker
                            objectName: "workspaceThemePicker"
                            Layout.fillWidth: true
                            Accessible.name: "Theme"
                            model: ["Aurora Glass", "Midnight", "Graphite", "Ember", "Pearl",
                                    "Daylight Blue", "Rose Bloom", "Neon Gamer", "Warm Sunset",
                                    "Nordic Frost", "Synthwave", "Deep Jade"]
                            currentIndex: Math.max(0, model.indexOf(Theme.styleName))
                            onActivated: Theme.styleName = currentText
                            Connections {
                                target: Theme
                                function onStyleNameChanged() {
                                    workspaceThemePicker.currentIndex = workspaceThemePicker.model.indexOf(Theme.styleName)
                                }
                            }
                        }
                    }
                    Label { text: "The theme changes immediately and is saved automatically."; color: Theme.textSecondary; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                    Label { text: "Text size"; font.bold: true }
                    Slider { Layout.fillWidth: true; from: 0.85; to: 1.5; stepSize: 0.05; value: studio.textScale; onMoved: studio.textScale=value }
                    Label { text: Math.round(studio.textScale * 100) + "%" }
                    Label { text: "Keyboard shortcuts"; font.bold: true }
                    Repeater {
                        model: Object.keys(studio.shortcuts)
                        RowLayout {
                            required property string modelData
                            Layout.fillWidth: true
                            Label { text: modelData; Layout.preferredWidth: 180 }
                            TextField { id: keyField; text: studio.shortcuts[modelData]; Layout.fillWidth: true; Accessible.name: modelData + " shortcut" }
                            Button { text: "Apply"; onClicked: root.message=studio.setShortcut(modelData,keyField.text) || "Shortcut saved." }
                        }
                    }
                    Button { text: "Reset shortcuts"; onClicked: studio.resetShortcuts() }
                    Label { text: "Use Tab to move between controls and Enter or Space to activate focused buttons. Single-key playback commands are paused while you type."; Layout.fillWidth: true; wrapMode: Text.WordWrap }
                }
            }
        }
        Label { text: root.message || studio.status; visible: text.length > 0; Layout.fillWidth: true; wrapMode: Text.WordWrap; color: Theme.cyan }
    }
    Connections { target: studio; function onStatusChanged(){root.message=""} }
    FileDialog { id: imageFiles; title: "Export images"; fileMode: FileDialog.OpenFiles; nameFilters: ["Images (*.png *.jpg *.jpeg *.webp *.bmp *.tif *.tiff)"]; onAccepted: player.exportImages(selectedFiles) }
    FolderDialog { id: exportFolder; title: "Export folder"; onAccepted: player.setCaptureDirectory(selectedFolder) }
    FolderDialog { id: pluginFolder; title: "Plugin folder"; onAccepted: root.message=pluginModel.installPlugin(selectedFolder) || "Plugin installed." }
    FileDialog { id: pluginBundle; title: "Plugin bundle"; nameFilters: ["Plugin bundle (*.andiyaplugin *.json)"]; onAccepted: studio.installBundle(selectedFile) }
    FileDialog { id: catalogFile; title: "Plugin catalog"; nameFilters: ["Catalog (*.json)"]; onAccepted: studio.loadCatalog(selectedFile) }
    FileDialog { id: saveWorkspace; title: "Export workspace"; fileMode: FileDialog.SaveFile; defaultSuffix: "andiya"; nameFilters: ["Workspace (*.andiya)"]; onAccepted: studio.exportWorkspace(selectedFile) }
    FileDialog { id: loadWorkspace; title: "Import workspace"; nameFilters: ["Workspace (*.andiya *.json)"]; onAccepted: studio.importWorkspace(selectedFile) }
    Dialog {
        id: importNotice
        title: "Replace workspace data?"
        anchors.centerIn: parent
        modal: true
        standardButtons: Dialog.Ok | Dialog.Cancel
        Label { text: "Import replaces your current bookmarks, playlist, presets and preferences.\nExport your current workspace first if you want to keep it."; wrapMode: Text.WordWrap }
        onAccepted: loadWorkspace.open()
    }
}
