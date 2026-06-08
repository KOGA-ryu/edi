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

function loadModule(modulePath, context) {
    const source = fs.readFileSync(modulePath, "utf8")
        .replace(".pragma library", "")
        .replace(/^\.import .*$/gm, "")
    vm.runInContext(source, context, { filename: modulePath })
}

function loadProjectionModule() {
    const context = {
        Math,
        Number,
        String,
        Array,
        Object,
    }
    vm.createContext(context)
    loadModule(path.join(__dirname, "..", "src", "runtime", "DrawingCanvasHandles.js"), context)
    context.CanvasHandles = {
        isRectangleLike: context.isRectangleLike,
        rotatedRectCorners: context.rotatedRectCorners,
    }
    loadModule(path.join(__dirname, "..", "src", "runtime", "DrawingCanvasProjection.js"), context)
    return context
}

function runObjectBoundsContract(projection) {
    const lineBounds = projection.normalizedObjectBounds({ kind: "line", x1: 0.2, y1: 0.4, x2: 0.8, y2: 0.1 })
    expect(lineBounds.ok, "line bounds should be valid")
    expectNear(lineBounds.minX, 0.2, "line bounds min x")
    expectNear(lineBounds.minY, 0.1, "line bounds min y")
    expectNear(lineBounds.maxX, 0.8, "line bounds max x")
    expectNear(lineBounds.maxY, 0.4, "line bounds max y")

    const circleBounds = projection.normalizedObjectBounds({ kind: "circle", cx: 0.5, cy: 0.4, radius: 0.125 })
    expectNear(circleBounds.minX, 0.375, "circle bounds min x")
    expectNear(circleBounds.maxY, 0.525, "circle bounds max y")

    const rectBounds = projection.normalizedObjectBounds({ kind: "rectangle", x: 0.25, y: 0.25, width: 0.25, height: 0.125, rotation_deg: 90 })
    expect(rectBounds.ok, "rotated rectangle bounds should be valid")
    expectNear(rectBounds.minX, 0.3125, "rotated rectangle bounds min x")
    expectNear(rectBounds.minY, 0.1875, "rotated rectangle bounds min y")
    expectNear(rectBounds.maxX, 0.4375, "rotated rectangle bounds max x")
    expectNear(rectBounds.maxY, 0.4375, "rotated rectangle bounds max y")

    const polygonBounds = projection.normalizedObjectBounds({ kind: "polygon", points: [[0.1, 0.2], [0.5, 0.1], [0.3, 0.7]] })
    expectNear(polygonBounds.maxY, 0.7, "polygon bounds should include vertices")
}

function runSelectionContract(projection) {
    const doc = {
        selected_object_id: "script_line_01",
        selected_object_ids: [],
        selected_layer_id: "layer_02",
    }
    expect(projection.selectedObject(doc, "script_line_01"), "selectedObject should use single selection fallback")
    expect(!projection.selectedObject(doc, "script_line_02"), "selectedObject should reject unselected object")
    expect(projection.selectedLayer(doc, "layer_02"), "selectedLayer should identify selected layer")

    const multi = {
        selected_object_id: "script_line_01",
        selected_object_ids: ["script_line_02", "script_rect_01"],
    }
    expect(!projection.selectedObject(multi, "script_line_01"), "selected ids should override single selected id")
    expect(projection.selectedObject(multi, "script_line_02"), "selectedObject should use selected id set")
}

function runCombinedSelectionContract(projection) {
    const doc = {
        selected_object_ids: ["script_line_01", "script_rect_01", "script_hidden_01", "foreign_01"],
        layers: [
            {
                id: "layer_01",
                visible: true,
                objects: [
                    { id: "script_line_01", kind: "line", x1: 0.1, y1: 0.2, x2: 0.4, y2: 0.8 },
                    { id: "foreign_01", kind: "line", x1: 0, y1: 0, x2: 1, y2: 1 },
                    { id: "script_rect_01", kind: "rectangle", x: 0.6, y: 0.1, width: 0.2, height: 0.2 },
                ],
            },
            {
                id: "layer_02",
                visible: false,
                objects: [
                    { id: "script_hidden_01", kind: "circle", cx: 1, cy: 1, radius: 0.5 },
                ],
            },
        ],
    }
    const bounds = projection.combinedSelectionBounds(doc)
    expect(bounds.ok, "combined selection bounds should be valid for multi-select")
    expectNear(bounds.minX, 0.1, "combined selection should include visible selected line min x")
    expectNear(bounds.minY, 0.1, "combined selection should include visible selected rectangle min y")
    expectNear(bounds.maxX, 0.8, "combined selection should include visible selected rectangle max x")
    expectNear(bounds.maxY, 0.8, "combined selection should ignore hidden layer and non-script ids")

    expect(
        projection.objectIntersectsBounds({ kind: "line", x1: 0.2, y1: 0.2, x2: 0.3, y2: 0.3 }, 0.1, 0.1, 0.25, 0.25),
        "objectIntersectsBounds should detect overlap"
    )
    expect(
        !projection.objectIntersectsBounds({ kind: "line", x1: 0.7, y1: 0.7, x2: 0.8, y2: 0.8 }, 0.1, 0.1, 0.25, 0.25),
        "objectIntersectsBounds should reject misses"
    )
}

const projection = loadProjectionModule()
runObjectBoundsContract(projection)
runSelectionContract(projection)
runCombinedSelectionContract(projection)

if (process.exitCode) {
    process.exit(process.exitCode)
}
