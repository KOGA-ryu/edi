const fs = require("fs")
const path = require("path")
const vm = require("vm")

function fail(message) {
    console.error(`FAIL: ${message}`)
    process.exitCode = 1
}

function expect(condition, message) {
    if (!condition) {
        fail(message)
    }
}

function expectNear(actual, expected, message) {
    if (Math.abs(actual - expected) >= 0.0001) {
        fail(`${message}; expected ${expected}, got ${actual}`)
    }
}

function loadViewportModule() {
    const modulePath = path.join(__dirname, "..", "src", "runtime", "DrawingCanvasViewport.js")
    const source = fs.readFileSync(modulePath, "utf8").replace(".pragma library", "")
    const context = {
        Math,
        Number,
        Object,
        Array,
    }
    vm.createContext(context)
    vm.runInContext(source, context, { filename: modulePath })
    return context
}

function runBoardBoundsContract(viewport) {
    const bounds = viewport.boardBounds(800, 600, 1.5, 12, -8)
    expect(typeof bounds === "object" && bounds !== null, "boardBounds should return an object")
    expectNear(bounds.size, 876.0, "boardBounds should use min dimension minus margin times zoom")
    expectNear(bounds.x, -26.0, "boardBounds should center and pan x deterministically")
    expectNear(bounds.y, -146.0, "boardBounds should center and pan y deterministically")
    expect(
        Number.isFinite(bounds.x) && Number.isFinite(bounds.y) && Number.isFinite(bounds.size) && bounds.size > 0,
        "boardBounds should return finite positive bounds"
    )

    const tinyBounds = viewport.boardBounds(4, 3, 0, 0, 0)
    expectNear(tinyBounds.size, 0.0032, "boardBounds should clamp invalid zoom while preserving minimum board rule")
}

function runCoordinateRoundTripContract(viewport) {
    const bounds = viewport.boardBounds(1024, 768, 2.0, -24, 40)
    const screen = viewport.canvasToScreen(bounds, 0.375, 0.625)
    const normalized = viewport.screenToCanvas(bounds, screen.x, screen.y)

    expectNear(normalized.x, 0.375, "screenToCanvas should round-trip x with canvasToScreen")
    expectNear(normalized.y, 0.625, "screenToCanvas should round-trip y with canvasToScreen")
    expectNear(viewport.canvasToScreenX(bounds, 0.375), screen.x, "canvasToScreenX should match canvasToScreen x")
    expectNear(viewport.canvasToScreenY(bounds, 0.625), screen.y, "canvasToScreenY should match canvasToScreen y")
}

const viewport = loadViewportModule()
runBoardBoundsContract(viewport)
runCoordinateRoundTripContract(viewport)

if (process.exitCode) {
    process.exit(process.exitCode)
}
