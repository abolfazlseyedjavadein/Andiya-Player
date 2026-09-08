import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

GlassPanel {
    id: root

    property string mode: "Library"
    signal addFilesRequested()
    signal openRequested()

    readonly property var sourceModel: mode === "Playlist" ? player.playlist : player.recentMedia

    panelColor: Theme.surface

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        RowLayout {
            Layout.fillWidth: true

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 1

                Text {
                    text: root.mode
                    color: Theme.text
                    font.pixelSize: Theme.fontScale * 21
                    font.weight: Font.DemiBold
                }

                Text {
                    text: root.mode === "Playlist" ? player.playlist.length + " queued items"
                                                     : player.recentMedia.length + " recent items"
                    color: Theme.textMuted
                    font.pixelSize: Theme.fontScale * 9
                }
            }

            IconButton {
                glyph: "+"
                toolTip: root.mode === "Playlist" ? "Add files to playlist" : "Open media"
                active: true
                buttonSize: 34
                onClicked: root.mode === "Playlist" ? root.addFilesRequested() : root.openRequested()
            }
        }

        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 36
            radius: 10
            color: Theme.backgroundSoft
            border.width: 1
            border.color: Theme.borderSoft

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 8
                spacing: 7

                Text { text: "\u2315"; color: Theme.textMuted; font.pixelSize: Theme.fontScale * 14 }
                TextField {
                    id: search
                    Layout.fillWidth: true
                    placeholderText: "Filter media"
                    color: Theme.text
                    placeholderTextColor: Theme.textMuted
                    font.pixelSize: Theme.fontScale * 10
                    background: Item {}
                }
            }
        }

        ListView {
            id: mediaList
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 8
            model: root.sourceModel
            boundsBehavior: Flickable.StopAtBounds

            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

            delegate: Rectangle {
                id: mediaCard
                required property int index
                required property var modelData

                readonly property bool matches: search.text.length === 0
                    || modelData.title.toLowerCase().includes(search.text.toLowerCase())

                width: mediaList.width
                height: matches ? 66 : 0
                visible: matches
                radius: 12
                color: hover.hovered || (root.mode === "Playlist" && player.playlistIndex === index)
                       ? Theme.surfaceHover : Theme.surfaceRaised
                border.width: root.mode === "Playlist" && player.playlistIndex === index ? 1 : 0
                border.color: Theme.cyan
                clip: true

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 9
                    spacing: 10

                    Rectangle {
                        id: thumbFrame
                        width: 44
                        height: 48
                        radius: 9
                        clip: true

                        // Video thumbnails are decoded lazily by ThumbnailProvider and cached
                        // to disk, so this only re-reads a file rather than re-decoding.
                        property string thumbUrl: modelData.kind === "Image"
                            ? modelData.url
                            : thumbnails.cachedThumbnailUrl(modelData.url)

                        Connections {
                            target: thumbnails
                            function onThumbnailReady(sourceUrl, thumbnailUrl) {
                                if (sourceUrl.toString() === modelData.url)
                                    thumbFrame.thumbUrl = thumbnailUrl
                            }
                        }

                        Component.onCompleted: {
                            // Only video has decodable frames; audio files keep their badge.
                            const isVideo = modelData.kind === "Video" || modelData.kind === "Media"
                            if (isVideo && !modelData.live && thumbFrame.thumbUrl.length === 0)
                                thumbnails.requestThumbnail(modelData.url)
                        }

                        gradient: Gradient {
                            GradientStop { position: 0; color: Qt.rgba(Theme.cyan.r, Theme.cyan.g, Theme.cyan.b, 0.32) }
                            GradientStop { position: 1; color: Qt.rgba(Theme.violet.r, Theme.violet.g, Theme.violet.b, 0.22) }
                        }

                        Image {
                            anchors.fill: parent
                            visible: thumbFrame.thumbUrl.length > 0
                            source: thumbFrame.thumbUrl
                            fillMode: Image.PreserveAspectCrop
                            asynchronous: true
                            cache: false
                        }

                        Text {
                            anchors.centerIn: parent
                            visible: thumbFrame.thumbUrl.length === 0
                            text: modelData.kind === "Audio" ? "AUD" : (modelData.kind === "Image" ? "IMG" : "PLAY")
                            color: Theme.text
                            font.pixelSize: Theme.fontScale * 8
                            font.weight: Font.Bold
                        }

                        Text {
                            anchors.right: parent.right
                            anchors.bottom: parent.bottom
                            anchors.margins: 3
                            visible: thumbFrame.thumbUrl.length > 0 && modelData.kind !== "Image"
                            text: "\u25B6"
                            color: "#FFFFFF"
                            font.pixelSize: Theme.fontScale * 8
                            style: Text.Outline
                            styleColor: "#A0000000"
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 3

                        Text {
                            Layout.fillWidth: true
                            text: modelData.title
                            color: Theme.text
                            font.pixelSize: Theme.fontScale * 11
                            font.weight: Font.Medium
                            elide: Text.ElideMiddle
                        }

                        Text {
                            Layout.fillWidth: true
                            text: root.mode === "Playlist"
                                  ? (player.playlistIndex === index ? "NOW PLAYING" : "IN QUEUE")
                                  : (modelData.opened || modelData.kind)
                            color: player.playlistIndex === index ? Theme.cyan : Theme.textMuted
                            font.pixelSize: Theme.fontScale * 8
                            elide: Text.ElideRight
                        }
                    }

                    IconButton {
                        visible: root.mode === "Playlist"
                        glyph: "\u00D7"
                        toolTip: "Remove"
                        buttonSize: 28
                        onClicked: player.removeFromPlaylist(mediaCard.index)
                    }
                }

                HoverHandler { id: hover }
                TapHandler {
                    onTapped: root.mode === "Playlist"
                              ? player.playPlaylistIndex(mediaCard.index)
                              : player.openRecent(mediaCard.index)
                }
            }

            Text {
                anchors.centerIn: parent
                visible: mediaList.count === 0
                text: root.mode === "Playlist" ? "Your playlist is empty" : "Open media to build your history"
                color: Theme.textMuted
                font.pixelSize: Theme.fontScale * 11
            }
        }

        IconButton {
            Layout.fillWidth: true
            label: root.mode === "Playlist" ? "Clear playlist" : "Clear history"
            glyph: "\u00D7"
            enabled: root.sourceModel.length > 0
            onClicked: root.mode === "Playlist" ? player.clearPlaylist() : player.clearRecentMedia()
        }
    }
}
