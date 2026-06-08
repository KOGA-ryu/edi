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

function loadSnapModule() {
    const modulePath = path.join(__dirname, "..", "src", "runtime", "DrawingCanvasSnap.js")
    const source = fs.readFileSync(modulePath, "utf8").replace(".pragma library", "")
    const context = {
        Math,
        Number,
        String,
        Array,
        Object,
    }
    vm.createContext(context)
    vm.runInContext(source, context, { filename: modulePath })
    return context
}

function baseSettings(overrides = {}) {
    return {
        canvasSizePx: 512,
        boardSizePx: 512,
        zoom: 1.0,
        gridEnabled: false,
        gridStepPx: 32,
        objectSnapEnabled: false,
        objectSnapTolerancePx: 14,
        endpointEnabled: true,
        midpointEnabled: true,
        centerEnabled: true,
        vertexEnabled: true,
        objectPriority: "before_grid",
        ...overrides,
    }
}

function runPointAndGridContract(snap) {
    const invalid = snap.resolveSnap({ x: Infinity, y: -Infinity }, [], baseSettings())
    expect(invalid.kind === "none", "disabled snap should report none")
    expect(Number.isFinite(invalid.x) && Number.isFinite(invalid.y), "disabled snap should return finite coordinates")
    expectNear(invalid.x, 0, "invalid x should clamp through finite fallback")
    expectNear(invalid.y, 0, "invalid y should clamp through finite fallback")

    const raw = snap.resolveSnap({ x: 1.2, y: 0.25 }, [], baseSettings())
    expectNear(raw.x, 1, "disabled snap should clamp x")
    expectNear(raw.y, 0.25, "disabled snap should preserve valid y")

    const grid = snap.resolveSnap({ x: 0.52, y: -0.1 }, [], baseSettings({ gridEnabled: true, gridStepPx: 64 }))
    expect(grid.kind === "grid", "grid snap should report grid kind")
    expectNear(grid.x, 0.5, "grid snap should round x to step")
    expectNear(grid.y, 0, "grid snap should clamp y to canvas")
    expect(grid.stepPx === 64, "grid snap should report stepPx")
    expect(grid.label === "grid 64px", "grid snap should report a readable label")
}

function runObjectPriorityContract(snap) {
    const objects = [
        { id: "line_a", kind: "line", x1: 0.3, y1: 0.2, x2: 0.7, y2: 0.2 },
    ]
    const objectFirst = snap.resolveSnap(
        { x: 0.295, y: 0.204 },
        objects,
        baseSettings({ gridEnabled: true, objectSnapEnabled: true, gridStepPx: 32 })
    )
    expect(objectFirst.kind === "object", "object snap should win when priority is before_grid")
    expect(objectFirst.sourceKind === "endpoint", "object snap should explain endpoint source")
    expect(objectFirst.sourceObjectId === "line_a", "object snap should report source object id")
    expectNear(objectFirst.x, 0.3, "object endpoint should set x")
    expectNear(objectFirst.y, 0.2, "object endpoint should set y")

    const gridOnly = snap.resolveSnap(
        { x: 0.295, y: 0.204 },
        objects,
        baseSettings({ gridEnabled: true, objectSnapEnabled: false, gridStepPx: 32 })
    )
    expect(gridOnly.kind === "grid", "grid should win when object snap is disabled")
    expectNear(gridOnly.x, 0.3125, "grid-only snap should use grid x")
    expectNear(gridOnly.y, 0.1875, "grid-only snap should use grid y")

    const gridFirst = snap.resolveSnap(
        { x: 0.295, y: 0.204 },
        objects,
        baseSettings({ gridEnabled: true, objectSnapEnabled: true, gridStepPx: 32, objectPriority: "after_grid" })
    )
    expect(gridFirst.kind === "grid", "grid should win when object priority is after_grid")

    const outsideTolerance = snap.resolveSnap(
        { x: 0.1, y: 0.9 },
        objects,
        baseSettings({ objectSnapEnabled: true, objectSnapTolerancePx: 4 })
    )
    expect(outsideTolerance.kind === "none", "object candidate outside tolerance should not snap")
}

function runCandidateContract(snap) {
    const rectangleKinds = snap.snapCandidatesForObject(
        { id: "rect_a", kind: "rectangle", x: 0.2, y: 0.3, width: 0.2, height: 0.4 },
        baseSettings()
    ).map(candidate => candidate.sourceKind)
    expect(rectangleKinds.includes("vertex"), "rectangle should expose vertex candidates")
    expect(rectangleKinds.includes("midpoint"), "rectangle should expose midpoint candidates")
    expect(rectangleKinds.includes("center"), "rectangle should expose center candidates")

    const lineKinds = snap.snapCandidatesForObject(
        { id: "line_b", kind: "line", x1: 0.1, y1: 0.1, x2: 0.5, y2: 0.1 },
        baseSettings()
    ).map(candidate => candidate.sourceKind)
    expect(lineKinds.filter(kind => kind === "endpoint").length === 2, "line should expose two endpoints")
    expect(lineKinds.includes("midpoint"), "line should expose midpoint")

    const polygonKinds = snap.snapCandidatesForObject(
        { id: "poly_a", kind: "polygon", points: [[0.1, 0.1], [0.3, 0.1], [0.2, 0.3]] },
        baseSettings()
    ).map(candidate => candidate.sourceKind)
    expect(polygonKinds.includes("vertex"), "polygon should expose vertex candidates")
    expect(polygonKinds.includes("midpoint"), "polygon should expose midpoint candidates")
    expect(polygonKinds.includes("center"), "polygon should expose center candidate")
}

const snap = loadSnapModule()
runPointAndGridContract(snap)
runObjectPriorityContract(snap)
runCandidateContract(snap)

if (process.exitCode) {
    process.exit(process.exitCode)
}
