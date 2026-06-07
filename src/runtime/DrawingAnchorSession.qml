import QtQuick

QtObject {
    id: anchorSession

    signal changed()

    property real drawingAnchorRootX: 0.50
    property real drawingAnchorRootY: 0.50
    property real drawingAnchorTipX: 0.50
    property real drawingAnchorTipY: 0.20
    property real drawingAnchorRightX: 0.80
    property real drawingAnchorRightY: 0.50
    property real drawingAnchorBottomX: 0.50
    property real drawingAnchorBottomY: 0.80
    property real drawingAnchorLeftX: 0.20
    property real drawingAnchorLeftY: 0.50

    function drawingAnchorPoint(anchorId) {
        if (anchorId === "anchor_root") {
            return { id: "anchor_root", label: "anchor_root", x: drawingAnchorRootX, y: drawingAnchorRootY }
        }
        if (anchorId === "anchor_tip") {
            return { id: "anchor_tip", label: "anchor_tip", x: drawingAnchorTipX, y: drawingAnchorTipY }
        }
        if (anchorId === "anchor_right") {
            return { id: "anchor_right", label: "anchor_right", x: drawingAnchorRightX, y: drawingAnchorRightY }
        }
        if (anchorId === "anchor_bottom") {
            return { id: "anchor_bottom", label: "anchor_bottom", x: drawingAnchorBottomX, y: drawingAnchorBottomY }
        }
        if (anchorId === "anchor_left") {
            return { id: "anchor_left", label: "anchor_left", x: drawingAnchorLeftX, y: drawingAnchorLeftY }
        }
        return { id: anchorId, label: anchorId, x: 0.5, y: 0.5 }
    }

    function setAnchorPosition(anchorId, x, y) {
        var clampedX = Math.max(0.02, Math.min(0.98, Number(x)))
        var clampedY = Math.max(0.02, Math.min(0.98, Number(y)))
        if (anchorId === "anchor_root") {
            if (drawingAnchorRootX !== clampedX || drawingAnchorRootY !== clampedY) {
                drawingAnchorRootX = clampedX
                drawingAnchorRootY = clampedY
                changed()
            }
            return true
        }
        if (anchorId === "anchor_tip") {
            if (drawingAnchorTipX !== clampedX || drawingAnchorTipY !== clampedY) {
                drawingAnchorTipX = clampedX
                drawingAnchorTipY = clampedY
                changed()
            }
            return true
        }
        if (anchorId === "anchor_right") {
            if (drawingAnchorRightX !== clampedX || drawingAnchorRightY !== clampedY) {
                drawingAnchorRightX = clampedX
                drawingAnchorRightY = clampedY
                changed()
            }
            return true
        }
        if (anchorId === "anchor_bottom") {
            if (drawingAnchorBottomX !== clampedX || drawingAnchorBottomY !== clampedY) {
                drawingAnchorBottomX = clampedX
                drawingAnchorBottomY = clampedY
                changed()
            }
            return true
        }
        if (anchorId === "anchor_left") {
            if (drawingAnchorLeftX !== clampedX || drawingAnchorLeftY !== clampedY) {
                drawingAnchorLeftX = clampedX
                drawingAnchorLeftY = clampedY
                changed()
            }
            return true
        }
        return false
    }

    function nearestAnchor(x, y, tolerance) {
        var anchors = ["anchor_root", "anchor_tip", "anchor_right", "anchor_bottom", "anchor_left"]
        var bestId = ""
        var bestDistance = Number(tolerance || 0.035)
        for (var index = 0; index < anchors.length; ++index) {
            var anchor = drawingAnchorPoint(anchors[index])
            var dx = Number(anchor.x) - Number(x)
            var dy = Number(anchor.y) - Number(y)
            var distance = Math.sqrt(dx * dx + dy * dy)
            if (distance <= bestDistance) {
                bestDistance = distance
                bestId = anchor.id
            }
        }
        return bestId
    }
}
