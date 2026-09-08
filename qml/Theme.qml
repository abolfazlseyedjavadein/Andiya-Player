pragma Singleton

import QtQuick

QtObject {
    property real fontScale: studio.textScale
    property string styleName: "Midnight"

    readonly property var colors: {
        if (styleName === "Aurora Glass") {
            return {
                light: false, background: "#111722", backgroundSoft: "#1A2230",
                surface: "#202A37", surfaceRaised: "#293545", surfaceHover: "#34465A",
                border: "#46576D", borderSoft: "#334154",
                text: "#F2F7FA", textSecondary: "#AAB8C7", textMuted: "#74869A",
                cyan: "#4DDBD5", violet: "#9A7AF2", green: "#48C978",
                amber: "#F2B94B", red: "#F06464",
                media: "#0B1119", mediaTop: "#182432", mediaMiddle: "#0E151F", mediaEnd: "#080C12"
            }
        }
        if (styleName === "Pearl") {
            return {
                light: true, background: "#F6F1E9", backgroundSoft: "#FFFCF7",
                surface: "#FFFFFF", surfaceRaised: "#FAF5ED", surfaceHover: "#EEE4D6",
                border: "#D4C5B2", borderSoft: "#E8DDCE",
                text: "#2C251F", textSecondary: "#665B51", textMuted: "#8C7E71",
                cyan: "#167D82", violet: "#8A5C8D", green: "#2D8061",
                amber: "#AD681C", red: "#C84755",
                media: "#EDE6DC", mediaTop: "#FFFDF9", mediaMiddle: "#F4EDE4", mediaEnd: "#E8DFD3"
            }
        }
        if (styleName === "Daylight Blue") {
            return {
                light: true, background: "#DCECF8", backgroundSoft: "#EFF8FE",
                surface: "#FAFDFF", surfaceRaised: "#E7F3FB", surfaceHover: "#CFE5F4",
                border: "#98BED8", borderSoft: "#C1D9E9",
                text: "#0C2940", textSecondary: "#365E79", textMuted: "#66859B",
                cyan: "#007EAE", violet: "#536AD3", green: "#167D60",
                amber: "#B66B13", red: "#C83E52",
                media: "#D4E5F1", mediaTop: "#F5FBFF", mediaMiddle: "#E2F0F8", mediaEnd: "#CDDFEC"
            }
        }
        if (styleName === "Rose Bloom") {
            return {
                light: true, background: "#FCEBF3", backgroundSoft: "#FFF6FA",
                surface: "#FFFCFE", surfaceRaised: "#F9E3EF", surfaceHover: "#F2CEE1",
                border: "#DDA7C4", borderSoft: "#EDCFE0",
                text: "#3A2030", textSecondary: "#775066", textMuted: "#9A7086",
                cyan: "#B34586", violet: "#7C55C7", green: "#2D876D",
                amber: "#BB6A2E", red: "#D33F70",
                media: "#F2DDE8", mediaTop: "#FFF8FC", mediaMiddle: "#F7E7F0", mediaEnd: "#ECD5E2"
            }
        }
        if (styleName === "Neon Gamer") {
            return {
                light: false, background: "#05060D", backgroundSoft: "#090B16",
                surface: "#0D1020", surfaceRaised: "#14182D", surfaceHover: "#202641",
                border: "#344063", borderSoft: "#202A48",
                text: "#F2F7FF", textSecondary: "#9AA9C9", textMuted: "#637398",
                cyan: "#00E8C3", violet: "#A76BFF", green: "#48F08A",
                amber: "#FFD84A", red: "#FF4778",
                media: "#03050A", mediaTop: "#0C1222", mediaMiddle: "#050812", mediaEnd: "#020309"
            }
        }
        if (styleName === "Warm Sunset") {
            return {
                light: false, background: "#1A0D09", backgroundSoft: "#24120D",
                surface: "#301A13", surfaceRaised: "#3C2319", surfaceHover: "#503024",
                border: "#704637", borderSoft: "#4A2D23",
                text: "#FFF4E9", textSecondary: "#D9B7A2", textMuted: "#A77C67",
                cyan: "#FFB14D", violet: "#FF7068", green: "#7FD39A",
                amber: "#FFD166", red: "#FF5F59",
                media: "#120806", mediaTop: "#29130D", mediaMiddle: "#160A07", mediaEnd: "#0E0504"
            }
        }
        if (styleName === "Graphite") {
            return {
                light: false, background: "#0E0F11", backgroundSoft: "#141518",
                surface: "#1B1D20", surfaceRaised: "#24272B", surfaceHover: "#303338",
                border: "#3A3E44", borderSoft: "#292C31",
                text: "#F4F7FB", textSecondary: "#A5ABB5", textMuted: "#707780",
                cyan: "#8CC8D8", violet: "#A7AEBB", green: "#43D19E",
                amber: "#FFB454", red: "#FF6B7D",
                media: "#090A0C", mediaTop: "#17191C", mediaMiddle: "#0D0F11", mediaEnd: "#07080A"
            }
        }
        if (styleName === "Ember") {
            return {
                light: false, background: "#120D0B", backgroundSoft: "#19110E",
                surface: "#211713", surfaceRaised: "#2A1D18", surfaceHover: "#36251E",
                border: "#52352B", borderSoft: "#35231D",
                text: "#F8F3EE", textSecondary: "#B9A69A", textMuted: "#806C61",
                cyan: "#FFB15C", violet: "#FF766E", green: "#80D29B",
                amber: "#FFC46B", red: "#FF665E",
                media: "#090504", mediaTop: "#21120E", mediaMiddle: "#0E0806", mediaEnd: "#080403"
            }
        }
        if (styleName === "Nordic Frost") {
            return {
                light: true, background: "#E8EDF2", backgroundSoft: "#F4F7FA",
                surface: "#FFFFFF", surfaceRaised: "#EEF2F6", surfaceHover: "#DCE4EC",
                border: "#B9C4CE", borderSoft: "#D3DBE2",
                text: "#1E2A33", textSecondary: "#51616D", textMuted: "#7C8B96",
                cyan: "#3E8CA0", violet: "#7477BD", green: "#478A67",
                amber: "#AD7C3C", red: "#BC5656",
                media: "#DCE4EA", mediaTop: "#F6F9FB", mediaMiddle: "#E4EAEF", mediaEnd: "#CFD8DF"
            }
        }
        if (styleName === "Synthwave") {
            return {
                light: false, background: "#14091F", backgroundSoft: "#1B0E29",
                surface: "#21122F", surfaceRaised: "#2B1740", surfaceHover: "#3A2054",
                border: "#4A2A63", borderSoft: "#33193F",
                text: "#FCE8FF", textSecondary: "#D6A8E8", textMuted: "#9670AC",
                cyan: "#35E7E0", violet: "#C86BFF", green: "#4CE39A",
                amber: "#FFB84D", red: "#FF4D8D",
                media: "#0E0616", mediaTop: "#250F35", mediaMiddle: "#150822", mediaEnd: "#09040E"
            }
        }
        if (styleName === "Deep Jade") {
            return {
                light: false, background: "#071510", backgroundSoft: "#0C1E17",
                surface: "#10261D", surfaceRaised: "#163124", surfaceHover: "#1F4030",
                border: "#2C5240", borderSoft: "#1E3A2C",
                text: "#F1FAF4", textSecondary: "#A9C9B8", textMuted: "#6E9280",
                cyan: "#4FD3B8", violet: "#8CA9FF", green: "#6FE39A",
                amber: "#E8C15B", red: "#FF6F6F",
                media: "#050F0B", mediaTop: "#123023", mediaMiddle: "#081C14", mediaEnd: "#04100B"
            }
        }
        return {
            light: false, background: "#090C11", backgroundSoft: "#0D1118",
            surface: "#121821", surfaceRaised: "#18202B", surfaceHover: "#202A38",
            border: "#273243", borderSoft: "#1D2633",
            text: "#F4F7FB", textSecondary: "#98A6B8", textMuted: "#647286",
            cyan: "#5CD6FF", violet: "#9B7CFF", green: "#43D19E",
            amber: "#FFB454", red: "#FF6B7D",
            media: "#070A0E", mediaTop: "#101722", mediaMiddle: "#080B10", mediaEnd: "#06080C"
        }
    }

    readonly property bool light: colors.light
    readonly property color background: colors.background
    readonly property color backgroundSoft: colors.backgroundSoft
    readonly property color surface: colors.surface
    readonly property color surfaceRaised: colors.surfaceRaised
    readonly property color surfaceHover: colors.surfaceHover
    readonly property color border: colors.border
    readonly property color borderSoft: colors.borderSoft
    readonly property color text: colors.text
    readonly property color textSecondary: colors.textSecondary
    readonly property color textMuted: colors.textMuted
    readonly property color cyan: colors.cyan
    readonly property color violet: colors.violet
    readonly property color green: colors.green
    readonly property color amber: colors.amber
    readonly property color red: colors.red
    readonly property color mediaSurface: colors.media
    readonly property color mediaGradientStart: colors.mediaTop
    readonly property color mediaGradientMiddle: colors.mediaMiddle
    readonly property color mediaGradientEnd: colors.mediaEnd

    readonly property int radiusSmall: 8
    readonly property int radius: 13
    readonly property int radiusLarge: 20
    readonly property int animationFast: 120
    readonly property int animation: 190
}
