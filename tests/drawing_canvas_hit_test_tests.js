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

function loadHitTestModule() {
    const modulePath = path.join(__dirname, "..", "src", "runtime", "DrawingCanvasHitTest.js")
    const source = fs.readFileSync(modulePath, "utf8").replace(".pragma library", "")
    const context = {
        Math,
        Number,
        String,
        Array,
    }
    vm.createContext(context)
    vm.runInContext(source, context, { filename: modulePath })
    return context
}

function runPrimitiveScoreContract(hitTest) {
    expectNear(
        hitTest.objectHitScore({ kind: "line", x1: 0.1, y1: 0.1, x2: 0.9, y2: 0.1 }, 0.5, 0.12, 0.025),
        0.02,
        "line hit score should measure distance to segment"
    )
    expect(
        hitTest.objectHitScore({ kind: "arc", cx: 0.5, cy: 0.5, radius: 0.25, start_angle_deg: 0, end_angle_deg: 90 }, 0.5, 0.25, 0.025) > 0.9,
        "arc hit score should reject points outside the angle span"
    )
    expectNear(
        hitTest.objectHitScore({ kind: "rectangle", x: 0.2, y: 0.2, width: 0.2, height: 0.2 }, 0.3, 0.3, 0.025),
        0,
        "rectangle hit score should be zero inside the rectangle"
    )
    expectNear(
        hitTest.objectHitScore({ kind: "polygon", points: [[0.2, 0.2], [0.4, 0.2], [0.3, 0.4]] }, 0.3, 0.25, 0.025),
        0,
        "polygon hit score should be zero inside the polygon"
    )
}

function runObjectPriorityContract(hitTest) {
    const objects = [
        { id: "script_line_01", kind: "line", x1: 0.1, y1: 0.1, x2: 0.9, y2: 0.1 },
        { id: "script_line_02", kind: "line", x1: 0.1, y1: 0.1, x2: 0.9, y2: 0.1 },
    ]
    const hit = hitTest.hitObjectAt(objects, 0.5, 0.1, 0.025)
    expect(hit.objectId === "script_line_02", "hitObjectAt should prefer later drawn objects on equal score")
    expect(hit.kind === "object", "hitObjectAt should report object hit kind")
    expectNear(hit.distance, 0, "hitObjectAt should report hit distance")

    const miss = hitTest.hitObjectAt(objects, 0.5, 0.5, 0.025)
    expect(miss.objectId === "", "hitObjectAt should return empty id on miss")
    expect(miss.kind === "none", "hitObjectAt should report none kind on miss")
}

const hitTest = loadHitTestModule()
runPrimitiveScoreContract(hitTest)
runObjectPriorityContract(hitTest)

if (process.exitCode) {
    process.exit(process.exitCode)
}
