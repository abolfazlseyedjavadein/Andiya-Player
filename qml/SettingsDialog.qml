import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

Popup {
    id: root

    signal creatorRequested()
    signal subtitleRequested()

    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    width: Math.min(760, parent ? parent.width - 48 : 760)
    height: Math.min(680, parent ? parent.height - 48 : 680)
    x: parent ? Math.round((parent.width - width) / 2) : 0
    y: parent ? Math.round((parent.height - height) / 2) : 0
    padding: 0

    Overlay.modal: Rectangle { color: "#A0060A0F" }

    background: Rectangle {
        radius: 22
        color: Theme.surface
        border.width: 1
        border.color: Theme.border
        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            height: 3
            radius: 2
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0; color: Theme.cyan }
                GradientStop { position: 1; color: Theme.violet }
            }
        }
    }

    contentItem: ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 12

        RowLayout {
            Layout.fillWidth: true
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 1
                Text { text: "Settings"; color: Theme.text; font.pixelSize: Theme.fontScale * 22; font.weight: Font.DemiBold }
                Text { text: "Appearance, playback, tracks and subtitles"; color: Theme.textSecondary; font.pixelSize: Theme.fontScale * 10 }
            }
            IconButton { glyph: "\u00D7"; toolTip: "Close settings"; onClicked: root.close() }
        }

        TabBar {
            id: settingsTabs
            Layout.fillWidth: true
            TabButton { text: "General" }
            TabButton { text: "Playback & tracks" }
            TabButton { text: "Subtitles" }
        }

        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: settingsTabs.currentIndex

            Flickable {
                contentHeight: generalColumn.implicitHeight
                clip: true
                boundsBehavior: Flickable.StopAtBounds
                ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                ColumnLayout {
                    id: generalColumn
                    width: parent.width - 10
                    spacing: 12

                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: 100
                        radius: 16
                        color: Theme.backgroundSoft
                        border.width: 1
                        border.color: Theme.borderSoft
                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 16
                            spacing: 14
                            Rectangle {
                                width: 42; height: 42; radius: 13
                                color: Qt.rgba(Theme.violet.r, Theme.violet.g, Theme.violet.b, 0.16)
                                Text { anchors.centerIn: parent; text: "UI"; color: Theme.violet; font.pixelSize: Theme.fontScale * 10; font.weight: Font.Bold }
                            }
                            ColumnLayout {
                                Layout.fillWidth: true; spacing: 2
                                Text { text: "Interface style"; color: Theme.text; font.pixelSize: Theme.fontScale * 13; font.weight: Font.Medium }
                                Text { text: "The selected theme is remembered automatically"; color: Theme.textMuted; font.pixelSize: Theme.fontScale * 9 }
                            }
                            ComboBox {
                                id: stylePicker
                                Layout.preferredWidth: 200
                                model: ["Aurora Glass", "Midnight", "Graphite", "Ember", "Pearl",
                                        "Daylight Blue", "Rose Bloom", "Neon Gamer", "Warm Sunset",
                                        "Nordic Frost", "Synthwave", "Deep Jade"]
                                currentIndex: Math.max(0, model.indexOf(Theme.styleName))
                                onActivated: Theme.styleName = currentText
                                Connections {
                                    target: Theme
                                    function onStyleNameChanged() {
                                        stylePicker.currentIndex = stylePicker.model.indexOf(Theme.styleName)
                                    }
                                }
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: 106
                        radius: 16
                        color: Theme.backgroundSoft
                        border.width: 1
                        border.color: Theme.borderSoft
                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 16
                            spacing: 12
                            Rectangle {
                                width: 42; height: 42; radius: 13
                                color: Qt.rgba(Theme.green.r, Theme.green.g, Theme.green.b, 0.14)
                                Text { anchors.centerIn: parent; text: "PNG"; color: Theme.green; font.pixelSize: Theme.fontScale * 9; font.weight: Font.Bold }
                            }
                            ColumnLayout {
                                Layout.fillWidth: true; spacing: 3
                                Text { text: "Lossless frame capture folder"; color: Theme.text; font.pixelSize: Theme.fontScale * 13; font.weight: Font.Medium }
                                Text { Layout.fillWidth: true; text: player.captureDirectoryPath; color: Theme.textMuted; font.pixelSize: Theme.fontScale * 9; elide: Text.ElideMiddle }
                            }
                            IconButton { label: "Change"; active: true; onClicked: captureFolderDialog.open() }
                            IconButton { label: "Open"; active: true; accentColor: Theme.green; onClicked: player.revealCaptureDirectory() }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: 116
                        radius: 16
                        color: Theme.backgroundSoft
                        border.width: 1
                        border.color: Theme.borderSoft
                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 16
                            spacing: 14
                            Rectangle {
                                width: 42; height: 42; radius: 13
                                color: Qt.rgba(Theme.cyan.r, Theme.cyan.g, Theme.cyan.b, 0.14)
                                Text { anchors.centerIn: parent; text: "HIS"; color: Theme.cyan; font.pixelSize: Theme.fontScale * 9; font.weight: Font.Bold }
                            }
                            ColumnLayout {
                                Layout.fillWidth: true; spacing: 3
                                Text { text: "History and resume"; color: Theme.text; font.pixelSize: Theme.fontScale * 13; font.weight: Font.Medium }
                                Text {
                                    Layout.fillWidth: true
                                    text: "Andiya remembers recent files, playlists and playback positions. Finished items restart from the beginning."
                                    color: Theme.textMuted; font.pixelSize: Theme.fontScale * 9; wrapMode: Text.WordWrap
                                }
                            }
                            IconButton { label: "Clear history"; enabled: player.recentMedia.length > 0; onClicked: player.clearRecentMedia() }
                        }
                    }
                }
            }

            Flickable {
                contentHeight: playbackColumn.implicitHeight
                clip: true
                boundsBehavior: Flickable.StopAtBounds
                ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                ColumnLayout {
                    id: playbackColumn
                    width: parent.width - 10
                    spacing: 12

                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: 150
                        radius: 16
                        color: Theme.backgroundSoft
                        border.width: 1
                        border.color: Theme.borderSoft
                        ColumnLayout {
                            anchors.fill: parent; anchors.margins: 16; spacing: 10
                            RowLayout {
                                Layout.fillWidth: true
                                ColumnLayout {
                                    Layout.fillWidth: true; spacing: 2
                                    Text { text: "Audio output"; color: Theme.text; font.pixelSize: Theme.fontScale * 13; font.weight: Font.Medium }
                                    Text { text: player.audioDeviceName; color: Theme.textMuted; font.pixelSize: Theme.fontScale * 9 }
                                }
                                Switch { text: player.muted ? "Muted" : "Sound on"; checked: !player.muted; onClicked: player.toggleMute() }
                            }
                            RowLayout {
                                Layout.fillWidth: true
                                Text { text: "Volume"; color: Theme.textSecondary; Layout.preferredWidth: 70 }
                                Slider { Layout.fillWidth: true; from: 0; to: 2; value: player.volume; onMoved: player.setVolume(value) }
                                Text { text: Math.round(player.volume * 100) + "%"; color: player.volume > 1 ? Theme.amber : Theme.text; Layout.preferredWidth: 42 }
                            }
                            Text { text: "Values above 100% use software boost and may clip loud recordings."; color: Theme.textMuted; font.pixelSize: Theme.fontScale * 9 }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: 100
                        radius: 16
                        color: Theme.backgroundSoft
                        border.width: 1
                        border.color: Theme.borderSoft
                        RowLayout {
                            anchors.fill: parent; anchors.margins: 16; spacing: 12
                            ColumnLayout {
                                Layout.fillWidth: true; spacing: 2
                                Text { text: "Playback speed"; color: Theme.text; font.pixelSize: Theme.fontScale * 13; font.weight: Font.Medium }
                                Text { text: "Saved for future sessions"; color: Theme.textMuted; font.pixelSize: Theme.fontScale * 9 }
                            }
                            Slider { id: speedSlider; Layout.preferredWidth: 260; from: 0.25; to: 4; stepSize: 0.05; value: player.playbackRate; onMoved: player.setPlaybackRate(value) }
                            Text { text: Number(player.playbackRate).toFixed(2) + "x"; color: Theme.cyan; Layout.preferredWidth: 48 }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: 126
                        radius: 16
                        color: Theme.backgroundSoft
                        border.width: 1
                        border.color: Theme.borderSoft
                        ColumnLayout {
                            anchors.fill: parent; anchors.margins: 16; spacing: 10
                            Text { text: "MEDIA TRACKS"; color: Theme.cyan; font.pixelSize: Theme.fontScale * 9; font.weight: Font.DemiBold; font.letterSpacing: 1.1 }
                            RowLayout {
                                Layout.fillWidth: true
                                Text { text: "Audio track"; color: Theme.textSecondary; Layout.preferredWidth: 110 }
                                ComboBox {
                                    Layout.fillWidth: true
                                    model: player.audioTracks
                                    textRole: "title"
                                    currentIndex: Math.max(0, player.activeAudioTrack)
                                    enabled: player.audioTracks.length > 0
                                    onActivated: player.setActiveAudioTrack(currentIndex)
                                }
                            }
                            RowLayout {
                                Layout.fillWidth: true
                                Text { text: "Embedded subtitles"; color: Theme.textSecondary; Layout.preferredWidth: 110 }
                                ComboBox {
                                    Layout.fillWidth: true
                                    model: player.subtitleTracks
                                    textRole: "title"
                                    currentIndex: Math.max(0, player.activeSubtitleTrack)
                                    enabled: player.subtitleTracks.length > 0
                                    onActivated: player.setActiveSubtitleTrack(currentIndex)
                                }
                                IconButton { label: "Off"; enabled: player.activeSubtitleTrack >= 0; onClicked: player.setActiveSubtitleTrack(-1) }
                            }
                        }
                    }
                }
            }

            Flickable {
                contentHeight: subtitleColumn.implicitHeight
                clip: true
                boundsBehavior: Flickable.StopAtBounds
                ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                ColumnLayout {
                    id: subtitleColumn
                    width: parent.width - 10
                    spacing: 12

                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: 88
                        radius: 16
                        color: Theme.backgroundSoft
                        border.width: 1
                        border.color: Theme.borderSoft
                        RowLayout {
                            anchors.fill: parent; anchors.margins: 16; spacing: 12
                            ColumnLayout {
                                Layout.fillWidth: true; spacing: 2
                                Text { text: player.hasSubtitles ? "Subtitles active" : "External subtitles"; color: Theme.text; font.pixelSize: Theme.fontScale * 13; font.weight: Font.Medium }
                                Text { text: player.subtitleFileName.length > 0 ? player.subtitleFileName : "Load SRT or WebVTT; matching filenames auto-load"; color: Theme.textMuted; font.pixelSize: Theme.fontScale * 9 }
                            }
                            IconButton { label: "Load file"; active: true; accentColor: Theme.amber; onClicked: root.subtitleRequested() }
                            IconButton { label: "Disable"; enabled: player.hasSubtitles; onClicked: player.clearSubtitles() }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: 234
                        radius: 16
                        color: Theme.backgroundSoft
                        border.width: 1
                        border.color: Theme.borderSoft
                        ColumnLayout {
                            anchors.fill: parent; anchors.margins: 16; spacing: 10
                            Text { text: "SUBTITLE APPEARANCE & SYNC"; color: Theme.amber; font.pixelSize: Theme.fontScale * 9; font.weight: Font.DemiBold; font.letterSpacing: 1.1 }
                            RowLayout {
                                Layout.fillWidth: true
                                Text { text: "Delay"; color: Theme.textSecondary; Layout.preferredWidth: 90 }
                                Slider { Layout.fillWidth: true; from: -10000; to: 10000; stepSize: 100; value: player.subtitleDelayMs; onMoved: player.setSubtitleDelayMs(value) }
                                Text { text: (player.subtitleDelayMs / 1000).toFixed(1) + " s"; color: Theme.text; Layout.preferredWidth: 52 }
                            }
                            RowLayout {
                                Layout.fillWidth: true
                                Text { text: "Font size"; color: Theme.textSecondary; Layout.preferredWidth: 90 }
                                Slider { Layout.fillWidth: true; from: 12; to: 42; stepSize: 1; value: player.subtitleFontSize; onMoved: player.setSubtitleFontSize(value) }
                                Text { text: player.subtitleFontSize + " px"; color: Theme.text; Layout.preferredWidth: 52 }
                            }
                            RowLayout {
                                Layout.fillWidth: true
                                Text { text: "Position"; color: Theme.textSecondary; Layout.preferredWidth: 90 }
                                Slider { Layout.fillWidth: true; from: 4; to: 36; stepSize: 1; value: player.subtitlePosition; onMoved: player.setSubtitlePosition(value) }
                                Text { text: player.subtitlePosition + "%"; color: Theme.text; Layout.preferredWidth: 52 }
                            }
                            RowLayout {
                                Layout.fillWidth: true
                                Text { text: "Text color"; color: Theme.textSecondary; Layout.preferredWidth: 90 }
                                Repeater {
                                    model: ["#FFFFFF", "#FFE16A", "#67E8F9", "#FF9FCB"]
                                    delegate: Rectangle {
                                        required property string modelData
                                        width: 34; height: 26; radius: 8; color: modelData
                                        border.width: player.subtitleColor === modelData ? 3 : 1
                                        border.color: player.subtitleColor === modelData ? Theme.cyan : Theme.border
                                        TapHandler { onTapped: player.setSubtitleColor(modelData) }
                                    }
                                }
                            }
                            Text {
                                Layout.fillWidth: true
                                text: "Persian and Arabic text is detected automatically and rendered with Unicode bidirectional layout and right alignment."
                                color: Theme.textMuted; font.pixelSize: Theme.fontScale * 9; wrapMode: Text.WordWrap
                            }
                        }
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            IconButton { glyph: "i"; label: "Creator"; active: true; accentColor: Theme.violet; onClicked: root.creatorRequested() }
            Text { Layout.fillWidth: true; text: "Andiya 0.1.1  |  Appearance and playback"; color: Theme.textMuted; font.pixelSize: Theme.fontScale * 9; horizontalAlignment: Text.AlignHCenter }
            IconButton { label: "Done"; prominent: true; buttonSize: 38; onClicked: root.close() }
        }
    }

    FolderDialog {
        id: captureFolderDialog
        title: "Choose the frame capture folder"
        onAccepted: player.setCaptureDirectory(selectedFolder)
    }
}
