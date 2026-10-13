import QtQuick

Canvas {
    id: root
    property string glyph: ""
    property color ink: Theme.text
    readonly property bool supported: ["LIB", "IMG", "VOL", "MUTE", "FULL", "CMP", "+", "i", "\u21B6", "\u2637", "\u25B6", "\u2016", "\u25C7", "\u2318", "\u2699", "\u22EF", "\u23EE", "\u23ED", "\u00D7", "\u21AF"].indexOf(glyph) >= 0
    implicitWidth: 22
    implicitHeight: 22
    onGlyphChanged: requestPaint()
    onInkChanged: requestPaint()
    onWidthChanged: requestPaint()
    onHeightChanged: requestPaint()
    onPaint: {
        const c = getContext("2d")
        c.reset()
        c.scale(width / 24, height / 24)
        c.strokeStyle = ink
        c.fillStyle = ink
        c.lineWidth = 1.65
        c.lineCap = "round"
        c.lineJoin = "round"
        function line(points) {
            c.beginPath(); c.moveTo(points[0], points[1])
            for (let i = 2; i < points.length; i += 2) c.lineTo(points[i], points[i + 1])
            c.stroke()
        }
        function box(x, y, w, h) { c.strokeRect(x, y, w, h) }
        switch (glyph) {
        case "LIB": box(3, 4, 7, 16); box(14, 4, 7, 16); line([6, 8, 7, 8]); line([17, 8, 18, 8]); break
        case "IMG": box(3, 4, 18, 16); line([4, 17, 9, 12, 13, 16, 16, 12, 20, 16]); c.beginPath(); c.arc(15, 9, 1.3, 0, 7); c.fill(); break
        case "\u21B6": c.beginPath(); c.arc(12, 12, 8, -2.5, 2.7); c.stroke(); line([3, 4, 3, 9, 8, 9]); line([12, 7, 12, 12, 16, 14]); break
        case "\u2637": for (let y = 6; y <= 18; y += 6) { line([8, y, 21, y]); c.beginPath(); c.arc(3, y, 1, 0, 7); c.fill() } break
        case "\u25B6": c.beginPath(); c.moveTo(8, 5); c.lineTo(19, 12); c.lineTo(8, 19); c.closePath(); c.fill(); break
        case "\u2016": line([8, 5, 8, 19]); line([16, 5, 16, 19]); break
        case "\u25C7": line([12, 3, 21, 12, 12, 21, 3, 12, 12, 3]); line([8, 12, 16, 12]); line([12, 8, 12, 16]); break
        case "\u2699": c.beginPath(); c.arc(12, 12, 6, 0, 7); c.stroke(); c.beginPath(); c.arc(12, 12, 2, 0, 7); c.stroke(); for (let i = 0; i < 8; ++i) { const a = i * Math.PI / 4; line([12 + 6 * Math.cos(a), 12 + 6 * Math.sin(a), 12 + 9 * Math.cos(a), 12 + 9 * Math.sin(a)]) } break
        case "\u2318": box(2, 5, 20, 14); for (let y = 9; y <= 13; y += 4) for (let x = 6; x <= 18; x += 4) { c.beginPath(); c.arc(x, y, .65, 0, 7); c.fill() } line([8, 16, 16, 16]); break
        case "i": c.beginPath(); c.arc(12, 12, 9, 0, 7); c.stroke(); line([12, 11, 12, 17]); c.beginPath(); c.arc(12, 7, .9, 0, 7); c.fill(); break
        case "+": line([12, 5, 12, 19]); line([5, 12, 19, 12]); break
        case "\u00D7": line([6, 6, 18, 18]); line([18, 6, 6, 18]); break
        case "\u22EF": for (let x = 5; x <= 19; x += 7) { c.beginPath(); c.arc(x, 12, 1.3, 0, 7); c.fill() } break
        case "FULL": line([8, 3, 3, 3, 3, 8]); line([16, 3, 21, 3, 21, 8]); line([3, 16, 3, 21, 8, 21]); line([16, 21, 21, 21, 21, 16]); break
        case "CMP": box(3, 4, 18, 16); line([12, 4, 12, 20]); break
        case "VOL": case "MUTE": line([4, 9, 8, 9, 13, 5, 13, 19, 8, 15, 4, 15, 4, 9]); if (glyph === "VOL") { c.beginPath(); c.arc(13, 12, 5, -.9, .9); c.stroke(); c.beginPath(); c.arc(13, 12, 8, -.9, .9); c.stroke() } else { line([17, 9, 22, 15]); line([22, 9, 17, 15]) } break
        case "\u23EE": case "\u23ED": if (glyph === "\u23EE") { c.translate(24, 0); c.scale(-1, 1) } line([6, 5, 16, 12, 6, 19, 6, 5]); line([19, 5, 19, 19]); break
        case "\u21AF": c.beginPath(); c.arc(12, 12, 3, 0, 7); c.stroke(); c.beginPath(); c.arc(12, 12, 8, -.75, .75); c.stroke(); c.beginPath(); c.arc(12, 12, 8, Math.PI - .75, Math.PI + .75); c.stroke(); break
        }
    }
}
