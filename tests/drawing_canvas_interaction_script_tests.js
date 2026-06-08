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

function loadModule(modulePath, context) {
    const source = fs.readFileSync(modulePath, "utf8")
        .replace(".pragma library", "")
        .replace(/^\.import .*$/gm, "")
    vm.runInContext(source, context, { filename: modulePath })
}

function loadScriptModule() {
    const context = {
        Math,
        Number,
        String,
        Array,
        Object,
        JSON,
    }
    vm.createContext(context)
    loadModule(path.join(__dirname, "..", "src", "runtime", "DrawingCanvasInteractionMetrics.js"), context)
    context.CanvasInteractionMetrics = {
        assertWithinBudget: context.assertWithinBudget,
    }
    loadModule(path.join(__dirname, "..", "src", "runtime", "DrawingCanvasInteractionScript.js"), context)
    return context
}

function readFixture(name) {
    return JSON.parse(fs.readFileSync(path.join(__dirname, "fixtures", "drawing_interactions", name), "utf8"))
}

function runValidationContract(script) {
    const missing = script.validateFixture({})
    expect(!missing.ok, "empty fixture should fail validation")
    expect(missing.failures.includes("fixture missing name"), "validation should require name")
    expect(missing.failures.includes("fixture requires at least one step"), "validation should require steps")

    const invalidStep = script.validateFixture({
        name: "bad_pointer",
        steps: [{ type: "pointerMove", x: 1 }],
        expect: { metrics: { mode: "dragging_object" } },
    })
    expect(!invalidStep.ok, "pointer step without finite coordinates should fail")
}

function runFixtureBudgetContract(script) {
    const fixture = readFixture("drag_line_endpoint.metrics.json")
    const result = script.evaluateFixture(fixture)
    expect(result.ok, "valid fixture should pass metrics and state expectations")

    fixture.actual.metrics.controllerMutations = 9
    const failed = script.evaluateFixture(fixture)
    expect(!failed.ok, "fixture should fail when actual metrics exceed budget")
    expect(
        failed.failures.some(message => message.indexOf("controllerMutations expected <=") >= 0),
        "fixture failure should explain controller mutation budget"
    )
}

function runPointerMoveStepContract(script) {
    const fixture = readFixture("drag_line_endpoint.metrics.json")
    fixture.actual.metrics.pointerMoves = 3
    const failed = script.evaluateFixture(fixture)
    expect(!failed.ok, "fixture should fail when pointerMoves do not match scripted pointerMove steps")
    expect(
        failed.failures.some(message => message.indexOf("pointerMoves expected to match") >= 0),
        "fixture failure should explain pointer move mismatch"
    )
}

function runBatchContract(script) {
    const good = readFixture("drag_line_endpoint.metrics.json")
    const bad = readFixture("drag_line_endpoint.metrics.json")
    bad.name = "bad_drag_line_endpoint"
    bad.actual.state.selectedObjectId = "script_other"
    const result = script.evaluateFixtures([good, bad])
    expect(!result.ok, "batch should fail when any fixture fails")
    expect(result.failures.length === 1, "batch should report only failing fixture")
    expect(result.failures[0].name === "bad_drag_line_endpoint", "batch should keep failing fixture name")
}

const script = loadScriptModule()
runValidationContract(script)
runFixtureBudgetContract(script)
runPointerMoveStepContract(script)
runBatchContract(script)

if (process.exitCode) {
    process.exit(process.exitCode)
}
