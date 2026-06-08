.pragma library

function finiteNumber(value, fallback) {
    var number = Number(value)
    return Number.isFinite(number) ? number : fallback
}

function boardBounds(viewWidth, viewHeight, zoom, panX, panY) {
    var width = Math.max(0, finiteNumber(viewWidth, 0))
    var height = Math.max(0, finiteNumber(viewHeight, 0))
    var safeZoom = Math.max(0.0001, finiteNumber(zoom, 1.0))
    var safePanX = finiteNumber(panX, 0)
    var safePanY = finiteNumber(panY, 0)
    var board = Math.max(32, Math.min(width, height) - 16) * safeZoom
    return {
        x: Math.round((width - board) / 2 + safePanX),
        y: Math.round((height - board) / 2 + safePanY),
        size: board
    }
}

function canvasToScreenX(bounds, normalizedX) {
    return finiteNumber(bounds && bounds.x, 0) + finiteNumber(normalizedX, 0) * finiteNumber(bounds && bounds.size, 1)
}

function canvasToScreenY(bounds, normalizedY) {
    return finiteNumber(bounds && bounds.y, 0) + finiteNumber(normalizedY, 0) * finiteNumber(bounds && bounds.size, 1)
}

function canvasToScreen(bounds, normalizedX, normalizedY) {
    return {
        x: canvasToScreenX(bounds, normalizedX),
        y: canvasToScreenY(bounds, normalizedY)
    }
}

function screenToCanvas(bounds, screenX, screenY) {
    var size = finiteNumber(bounds && bounds.size, 1)
    if (Math.abs(size) < 0.000001) {
        size = 1
    }
    return {
        x: (finiteNumber(screenX, 0) - finiteNumber(bounds && bounds.x, 0)) / size,
        y: (finiteNumber(screenY, 0) - finiteNumber(bounds && bounds.y, 0)) / size
    }
}
