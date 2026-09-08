pragma Singleton

import QtQuick

QtObject {
    function pad2(value) { return value < 10 ? "0" + value : "" + value }

    function time(milliseconds) {
        if (!isFinite(milliseconds) || milliseconds < 0)
            milliseconds = 0
        const total = Math.floor(milliseconds / 1000)
        const hours = Math.floor(total / 3600)
        const minutes = Math.floor((total % 3600) / 60)
        const seconds = total % 60
        return hours > 0 ? hours + ":" + pad2(minutes) + ":" + pad2(seconds)
                         : minutes + ":" + pad2(seconds)
    }
}
