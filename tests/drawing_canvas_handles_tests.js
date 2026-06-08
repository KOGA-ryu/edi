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

function loadHandleModule() {
    const modulePath = path.join(__dirname, "..", "src", "runtime", "DrawingCanvasHandles.js")
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

function settings(overrides = {}) {
    return {
        canvasSizePx: 512,
        rotateHandleOffsetPx: 28,
        handleHitTolerancePx: 14,
        rotateHandleHitTolerancePx: 18,
        shiftConstrain: false,
        angleSnapDeg: 15,
        ...overrides,
    }
}

function fields(plan) {
    return plan.updates.map(update => update.field)
}

function updateValue(plan, field) {
    const update = plan.updates.find(candidate => candidate.field === field)
    return update ? update.value : undefined
}

function runLineContract(handles) {
    const object = { id: "script_line_01", kind: "line", x1: 0.25, y1: 0.125, x2: 0.75, y2: 0.875 }
    const result = handles.handlesForObject(object, settings())
    expect(result.length === 2, "line should return exactly two handles")
    expect(result[0].id === "line_start", "line start id should be stable")
    expect(result[1].id === "line_end", "line end id should be stable")
    expect(result[0].field === "x1_y1", "line start should expose x1/y1 field identity")
    expect(result[1].updateFields.join(",") === "x2_px,y2_px", "line end update fields should target x2/y2 pixels")

    const plan = handles.handleUpdatePlan(object, "line_start", { x: 0.5, y: 0.25 }, settings())
    expect(plan.ok, "line start should produce an update plan")
    expect(fields(plan).join(",") === "x1_px,y1_px", "line start update plan should target x1/y1 pixels")
    expectNear(updateValue(plan, "x1_px"), 256, "line start x update should use canvas pixels")
    expectNear(updateValue(plan, "y1_px"), 128, "line start y update should use canvas pixels")
}

function runPointContract(handles) {
    const object = { id: "script_point_01", kind: "point", x: 0.25, y: 0.5 }
    const result = handles.handlesForObject(object, settings())
    expect(result.length === 1, "point should return one handle")
    expect(result[0].id === "point_position", "point handle id should be stable")
    expect(result[0].updateFields.join(",") === "x_px,y_px", "point handle should map to x/y pixel fields")

    const plan = handles.handleUpdatePlan(object, "point_position", { x: 0.75, y: 0.25 }, settings())
    expect(plan.ok, "point handle should produce update plan")
    expectNear(updateValue(plan, "x_px"), 384, "point x update should use canvas pixels")
    expectNear(updateValue(plan, "y_px"), 128, "point y update should use canvas pixels")
}

function runCircleContract(handles) {
    const object = { id: "script_circle_01", kind: "circle", cx: 0.4, cy: 0.5, radius: 0.2 }
    const result = handles.handlesForObject(object, settings())
    expect(result.length === 2, "circle should return center and radius handles")
    expect(result[0].id === "circle_center", "circle center id should be stable")
    expect(result[1].id === "circle_radius", "circle radius id should be stable")
    expectNear(result[1].x, 0.6, "circle radius handle should sit at cx + radius")
    expectNear(result[1].y, 0.5, "circle radius handle should preserve cy")

    const centerPlan = handles.handleUpdatePlan(object, "circle_center", { x: 0.25, y: 0.25 }, settings())
    expect(centerPlan.ok, "circle center should produce update plan")
    expect(fields(centerPlan).join(",") === "cx_px,cy_px", "circle center should update cx/cy")

    const radiusPlan = handles.handleUpdatePlan(object, "circle_radius", { x: 0.4, y: 0.7 }, settings())
    expect(radiusPlan.ok, "circle radius should produce update plan")
    expectNear(updateValue(radiusPlan, "radius_px"), 102.4, "circle radius should derive pixel distance from center")
}

function runRectangleContract(handles) {
    const object = { id: "script_rect_01", kind: "rectangle", x: 0.25, y: 0.25, width: 0.25, height: 0.125, rotation_deg: 90 }
    const corners = handles.rotatedRectCorners(object)
    expect(corners.length === 4, "rectangle should return four rotated corners")
    expect(corners[0].id === "rect_nw", "rectangle nw corner id should be stable")
    expectNear(corners[0].x, 0.4375, "rotated rectangle nw x should be deterministic")
    expectNear(corners[0].y, 0.1875, "rotated rectangle nw y should be deterministic")

    const result = handles.handlesForObject(object, settings())
    expect(result.length === 5, "rectangle should return four corners plus rotate handle")
    const rotate = result.find(handle => handle.id === "rect_rotate")
    expect(rotate && rotate.role === "rotate", "rectangle rotate handle should expose rotate role")
    expect(rotate.updateFields.join(",") === "rotation_deg", "rectangle rotate handle should update rotation_deg")

    const cornerPlan = handles.handleUpdatePlan(
        { id: "script_rect_02", kind: "rectangle", x: 0.25, y: 0.25, width: 0.25, height: 0.25, rotation_deg: 0 },
        "rect_se",
        { x: 0.75, y: 0.625 },
        settings()
    )
    expect(cornerPlan.ok, "rectangle corner should produce update plan")
    expect(fields(cornerPlan).join(",") === "x_px,y_px,width_px,height_px", "rectangle corner plan should update legal fields")
    expectNear(updateValue(cornerPlan, "x_px"), 128, "rectangle corner plan should keep fixed x origin")
    expectNear(updateValue(cornerPlan, "y_px"), 128, "rectangle corner plan should keep fixed y origin")
    expectNear(updateValue(cornerPlan, "width_px"), 256, "rectangle corner plan should update width")
    expectNear(updateValue(cornerPlan, "height_px"), 192, "rectangle corner plan should update height")

    const rotatePlan = handles.handleUpdatePlan(object, "rect_rotate", { x: 0.6, y: 0.3125 }, settings({ shiftConstrain: true, angleSnapDeg: 15 }))
    expect(rotatePlan.ok, "rectangle rotate should produce update plan")
    expect(fields(rotatePlan).join(",") === "rotation_deg", "rectangle rotate plan should update rotation_deg")
    expectNear(updateValue(rotatePlan, "rotation_deg"), 90, "rectangle rotate plan should snap degrees when constrained")
}

function runReadOnlyContract(handles) {
    const polygon = { id: "script_poly_01", kind: "polygon", points: [[0.1, 0.1], [0.2, 0.1], [0.2, 0.2]] }
    const result = handles.handlesForObject(polygon, settings())
    expect(result.length === 3, "polygon should expose read-only vertex handles")
    expect(result.every(handle => handle.readOnly === true), "polygon vertex handles should be read-only")
    const plan = handles.handleUpdatePlan(polygon, result[0].id, { x: 0.4, y: 0.4 }, settings())
    expect(!plan.ok && plan.updates.length === 0, "read-only handles should not produce update plans")
}

function runHitContract(handles) {
    const object = { id: "script_line_01", kind: "line", x1: 0.25, y1: 0.25, x2: 0.75, y2: 0.25 }
    const hit = handles.hitHandleAt(object, 129, 129, { x: 0, y: 0, size: 512 }, settings())
    expect(hit.ok, "hitHandleAt should hit nearest handle within tolerance")
    expect(hit.id === "line_start", "hitHandleAt should prefer nearest handle")
    expect(hit.kind === "handle", "hitHandleAt should report handle kind")

    const miss = handles.hitHandleAt(object, 64, 400, { x: 0, y: 0, size: 512 }, settings())
    expect(!miss.ok, "hitHandleAt miss should report not ok")
    expect(miss.id === "", "hitHandleAt miss should return empty id")
    expect(miss.kind === "none", "hitHandleAt miss should report none kind")

    const rect = { id: "script_rect_01", kind: "rectangle", x: 0.25, y: 0.25, width: 0.25, height: 0.25, rotation_deg: 0 }
    const rotate = handles.rotatedRectRotationHandle(rect, settings())
    const rotateHit = handles.hitHandleAt(rect, rotate.x * 512 + 17, rotate.y * 512, { x: 0, y: 0, size: 512 }, settings())
    expect(rotateHit.ok && rotateHit.id === "rect_rotate", "rotate handle should use rotate hit tolerance")
}

const handles = loadHandleModule()
runLineContract(handles)
runPointContract(handles)
runCircleContract(handles)
runRectangleContract(handles)
runReadOnlyContract(handles)
runHitContract(handles)

if (process.exitCode) {
    process.exit(process.exitCode)
}
