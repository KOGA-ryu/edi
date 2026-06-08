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

function loadModules() {
    const context = {
        Math,
        Number,
        String,
        Array,
        Object,
        JSON,
    }
    vm.createContext(context)
    loadModule(path.join(__dirname, "..", "src", "runtime", "DrawingCanvasToolScript.js"), context)
    context.CanvasToolScript = {
        executionPlan: context.executionPlan,
    }
    loadModule(path.join(__dirname, "..", "src", "runtime", "DrawingCanvasToolScriptDriver.js"), context)
    return context
}

function readFixture(name) {
    return JSON.parse(fs.readFileSync(path.join(__dirname, "fixtures", "drawing_tool_scripts", name), "utf8"))
}

function executionPlan(modules, fixtureName) {
    const plan = modules.CanvasToolScript.executionPlan(readFixture(fixtureName), readFixture("shared_canvas_library.json"))
    expect(plan.ok, `${fixtureName} should build execution plan`)
    return plan
}

function runClickDriverContract(driver) {
    const plan = driver.driverPlan({
        name: "click",
        profile: "profile.json",
        steps: [
            { type: "selectTool", toolId: "line_polyline" },
            { type: "clickCanvas", point: { x: 128, y: 128 } },
        ],
        metricsByMode: { draw_click: { maxRenderRequests: 1 } },
    })

    expect(plan.ok, "driver plan should build")
    expect(plan.plan.ops.length === 4, "select plus click should produce four ops")
    expect(plan.plan.ops[0].op === "setTool", "first op should set tool")
    expect(plan.plan.ops[1].op === "movePointer", "click should move pointer first")
    expect(plan.plan.ops[2].op === "pointerDown", "click should press")
    expect(plan.plan.ops[3].op === "pointerUp", "click should release")
    expect(plan.plan.expectedTelemetryModes.length === 1, "setTool should not add telemetry mode")
    expect(plan.plan.expectedTelemetryModes[0] === "draw_click", "click should expect draw_click telemetry")
}

function runDragInterpolationContract(driver) {
    const ops = driver.stepDriverOps({
        type: "dragHandle",
        object: "latest",
        handleId: "line_end",
        from: { x: 256, y: 128 },
        to: { x: 320, y: 160 },
        pointerMoves: 4,
    })

    expect(ops.length === 8, "drag handle should include target, move, down, four moves, up")
    expect(ops[0].op === "targetHandle", "drag handle should target handle before pointer ops")
    expect(ops[3].op === "pointerMove", "drag should emit pointer moves")
    expect(ops[3].x === 272 && ops[3].y === 136, "first interpolated point should be one quarter")
    expect(ops[6].x === 320 && ops[6].y === 160, "last move should reach target")
    expect(ops[7].op === "pointerUp", "drag should release")
}

function runFixtureDriverContract(modules) {
    const plan = executionPlan(modules, "line_drag_end_handle.json")
    const driverPlan = modules.driverPlan(plan)
    expect(driverPlan.ok, "line drag handle should build driver plan")
    expect(driverPlan.plan.profile === "data/project_profiles/draftsman_drawing_tool_blank.json", "driver plan should preserve profile")
    expect(driverPlan.plan.expectedTelemetryModes.indexOf("draw_click") >= 0, "driver plan should expect draw_click")
    expect(driverPlan.plan.expectedTelemetryModes.indexOf("dragging_handle") >= 0, "driver plan should expect dragging_handle")
    expect(driverPlan.plan.metricsByMode.dragging_handle.maxHitTestsPerPointerMove === 0.25, "driver plan should preserve mode budgets")
    expect(driverPlan.plan.stepMap.length === plan.plan.steps.length, "driver plan should map every semantic step")
    expect(driverPlan.plan.ops.some(op => op.op === "targetHandle" && op.handleId === "line_end"), "driver ops should target line_end handle")
}

function runAllFixtureDriverPlansContract(modules) {
    const names = [
        "line_create_basic.json",
        "line_drag_end_handle.json",
        "line_move_object.json",
        "marquee_select_lines.json",
        "pan_canvas_basic.json",
    ]
    for (const name of names) {
        const plan = executionPlan(modules, name)
        const driverPlan = modules.driverPlan(plan)
        expect(driverPlan.ok, `${name} should build driver plan`)
        expect(driverPlan.plan.ops.length > 0, `${name} should produce driver ops`)
        expect(Object.keys(driverPlan.plan.metricsByMode).length > 0, `${name} should preserve budgets`)
    }
}

function runPanZoomContract(driver) {
    const plan = driver.driverPlan({
        name: "viewport",
        steps: [
            { type: "panCanvas", dx: 32, dy: -16 },
            { type: "zoomCanvas", factor: 1.25, point: { x: 200, y: 180 } },
        ],
    })

    expect(plan.plan.ops[0].op === "wheelPan", "pan should become wheelPan op")
    expect(plan.plan.ops[0].dx === 32 && plan.plan.ops[0].dy === -16, "pan should preserve delta")
    expect(plan.plan.ops[1].op === "wheelZoom", "zoom should become wheelZoom op")
    expect(plan.plan.ops[1].factor === 1.25, "zoom should preserve factor")
    expect(plan.plan.expectedTelemetryModes.indexOf("panning") >= 0, "pan should expect panning telemetry")
    expect(plan.plan.expectedTelemetryModes.indexOf("zooming") >= 0, "zoom should expect zooming telemetry")
}

const modules = loadModules()
runClickDriverContract(modules)
runDragInterpolationContract(modules)
runFixtureDriverContract(modules)
runAllFixtureDriverPlansContract(modules)
runPanZoomContract(modules)

if (process.exitCode) {
    process.exit(process.exitCode)
}
