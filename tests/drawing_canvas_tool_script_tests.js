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

function loadToolScriptModule() {
    const modulePath = path.join(__dirname, "..", "src", "runtime", "DrawingCanvasToolScript.js")
    const source = fs.readFileSync(modulePath, "utf8").replace(".pragma library", "")
    const context = {
        Math,
        Number,
        String,
        Array,
        Object,
        JSON,
    }
    vm.createContext(context)
    vm.runInContext(source, context, { filename: modulePath })
    return context
}

function readFixture(name) {
    return JSON.parse(fs.readFileSync(path.join(__dirname, "fixtures", "drawing_tool_scripts", name), "utf8"))
}

function readWorkflowManifest() {
    const manifest = readFixture("workflow_manifest.json")
    return Array.isArray(manifest.workflows) ? manifest.workflows : []
}

function runValidationContract(toolScript) {
    const missing = toolScript.validateScript({})
    expect(!missing.ok, "empty script should fail validation")
    expect(missing.failures.includes("script missing name"), "validation should require name")
    expect(missing.failures.includes("script requires at least one step"), "validation should require steps")

    const invalid = toolScript.validateScript({
        name: "bad_drag",
        steps: [
            { type: "dragHandle", object: "latest", to: { x: 1, y: 2 } },
            { use: "missingFragment" },
        ],
    })
    expect(!invalid.ok, "invalid script should fail")
    expect(invalid.failures.some(message => message.indexOf("dragHandle requires handleId") >= 0), "validation should explain missing handleId")
    expect(invalid.failures.some(message => message.indexOf("unknown fragment") >= 0), "validation should explain missing fragment")

    const invalidParameter = toolScript.validateScript({
        name: "bad_parameter",
        steps: [
            { type: "setToolParameter", parameter: "circle_arc_mode", value: "banana" },
        ],
    })
    expect(!invalidParameter.ok, "invalid tool parameter should fail")
    expect(invalidParameter.failures.some(message => message.indexOf("invalid circle_arc_mode") >= 0), "validation should explain invalid tool parameter")
}

function runCompositionContract(toolScript) {
    const library = readFixture("shared_canvas_library.json")
    const script = readFixture("line_create_basic.json")
    const plan = toolScript.executionPlan(script, library)
    expect(plan.ok, "line create script should build execution plan")
    expect(plan.plan.steps.length === 3, "fragment expansion should produce three concrete steps")
    expect(plan.plan.steps[0].type === "selectTool", "first expanded step should select tool")
    expect(plan.plan.steps[1].point.x === 128, "named point should resolve x")
    expect(plan.plan.steps[2].point.y === 128, "named point should resolve y")
    expect(plan.plan.metricsByMode.draw_click.maxRenderRequests === 1, "named budget should resolve")
}

function runDragHandleContract(toolScript) {
    const library = readFixture("shared_canvas_library.json")
    const script = readFixture("line_drag_end_handle.json")
    const plan = toolScript.executionPlan(script, library)
    expect(plan.ok, "line drag handle script should build execution plan")
    expect(plan.plan.steps.length === 5, "line drag handle should expand create line, select move, and drag")
    const drag = plan.plan.steps[4]
    expect(drag.type === "dragHandle", "last step should drag handle")
    expect(drag.object === "latest", "drag handle should preserve object alias")
    expect(drag.handleId === "line_end", "drag handle should preserve handle id")
    expect(drag.to.x === 320 && drag.to.y === 160, "drag handle target should resolve")
    expect(drag.pointerMoves === 8, "drag handle should preserve pointer move count")
    expect(plan.plan.metricsByMode.dragging_handle.maxMutationsPerPointerMove === 2.1, "handle drag budget should resolve")
}

function runToolParameterContract(toolScript) {
    const library = readFixture("shared_canvas_library.json")
    const script = readFixture("arc_create_basic.json")
    const plan = toolScript.executionPlan(script, library)
    expect(plan.ok, "arc script should build execution plan")
    expect(plan.plan.steps.length === 6, "arc fragment should expand parameter steps and clicks")
    expect(plan.plan.steps[1].type === "setToolParameter", "arc mode should use parameter step")
    expect(plan.plan.steps[1].parameter === "circle_arc_mode", "arc mode parameter should be explicit")
    expect(plan.plan.steps[1].value === "arc", "arc mode value should be preserved")
    expect(plan.plan.steps[2].value === 15, "arc start angle should be preserved")
    expect(plan.plan.steps[3].value === 120, "arc end angle should be preserved")
}

function runAllFixturePlansContract(toolScript) {
    const library = readFixture("shared_canvas_library.json")
    const names = readWorkflowManifest()
    expect(names.length >= 13, "workflow manifest should include the drawing control workflows")
    for (const name of names) {
        const plan = toolScript.executionPlan(readFixture(name), library)
        expect(plan.ok, `${name} should build execution plan: ${plan.failures.join(", ")}`)
        expect(plan.plan.steps.length > 0, `${name} should contain executable steps`)
        expect(Object.keys(plan.plan.metricsByMode).length > 0, `${name} should expose per-mode budgets`)
    }
}

function runCycleContract(toolScript) {
    const script = {
        name: "cycle",
        fragments: {
            a: [{ use: "b" }],
            b: [{ use: "a" }],
        },
        steps: [{ use: "a" }],
    }
    const validation = toolScript.validateScript(script)
    expect(validation.ok, "cycle is an expansion failure, not a shape validation failure")
    const expanded = toolScript.expandedSteps(script)
    expect(!expanded.ok, "fragment cycle should fail expansion")
    expect(expanded.failures.some(message => message.indexOf("fragment cycle") >= 0), "cycle failure should be explicit")
}

const toolScript = loadToolScriptModule()
runValidationContract(toolScript)
runCompositionContract(toolScript)
runDragHandleContract(toolScript)
runToolParameterContract(toolScript)
runAllFixturePlansContract(toolScript)
runCycleContract(toolScript)

if (process.exitCode) {
    process.exit(process.exitCode)
}
