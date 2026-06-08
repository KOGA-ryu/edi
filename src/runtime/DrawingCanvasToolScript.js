.pragma library

function finiteNumber(value, fallback) {
    var number = Number(value)
    return Number.isFinite(number) ? number : fallback
}

function asArray(value) {
    if (!value) {
        return []
    }
    if (Array.isArray(value)) {
        return value
    }
    if (typeof value.length === "number") {
        var result = []
        for (var index = 0; index < value.length; ++index) {
            result.push(value[index])
        }
        return result
    }
    return []
}

function cloneValue(value) {
    if (value === undefined || value === null) {
        return value
    }
    return JSON.parse(JSON.stringify(value))
}

function objectAssign(base, overlay) {
    var result = cloneValue(base || ({})) || ({})
    var keys = Object.keys(overlay || ({}))
    for (var index = 0; index < keys.length; ++index) {
        result[keys[index]] = cloneValue(overlay[keys[index]])
    }
    return result
}

function composeScript(script, library) {
    var source = script || ({})
    var shared = library || ({})
    var composed = cloneValue(source) || ({})
    composed.fragments = objectAssign(shared.fragments, source.fragments)
    composed.points = objectAssign(shared.points, source.points)
    composed.budgets = objectAssign(shared.budgets, source.budgets)
    return composed
}

function pointFromValue(value, points) {
    if (typeof value === "string") {
        return pointFromValue(points && points[value], points)
    }
    if (!value || typeof value !== "object") {
        return null
    }
    var x = Number(value.x)
    var y = Number(value.y)
    if (!Number.isFinite(x) || !Number.isFinite(y)) {
        return null
    }
    return {
        x: x,
        y: y
    }
}

function pointFromStep(step, field, points) {
    if (step && step[field] !== undefined) {
        return pointFromValue(step[field], points)
    }
    if (field === "point") {
        return pointFromValue(step, points)
    }
    return null
}

function validName(value) {
    return String(value || "").trim().length > 0
}

function validatePointRecord(name, value, failures) {
    if (!pointFromValue(value, {})) {
        failures.push("points." + String(name) + " requires finite x/y")
    }
}

function validateBudgetRecord(name, value, failures) {
    if (!value || typeof value !== "object" || Array.isArray(value)) {
        failures.push("budgets." + String(name) + " must be an object")
    }
}

function validateLibrary(script, failures) {
    var points = script && script.points ? script.points : ({})
    var pointNames = Object.keys(points)
    for (var pointIndex = 0; pointIndex < pointNames.length; ++pointIndex) {
        validatePointRecord(pointNames[pointIndex], points[pointNames[pointIndex]], failures)
    }

    var fragments = script && script.fragments ? script.fragments : ({})
    var fragmentNames = Object.keys(fragments)
    for (var fragmentIndex = 0; fragmentIndex < fragmentNames.length; ++fragmentIndex) {
        if (asArray(fragments[fragmentNames[fragmentIndex]]).length <= 0) {
            failures.push("fragments." + fragmentNames[fragmentIndex] + " requires at least one step")
        }
    }

    var budgets = script && script.budgets ? script.budgets : ({})
    var budgetNames = Object.keys(budgets)
    for (var budgetIndex = 0; budgetIndex < budgetNames.length; ++budgetIndex) {
        validateBudgetRecord(budgetNames[budgetIndex], budgets[budgetNames[budgetIndex]], failures)
    }
}

function validateStep(step, index, script, failures) {
    if (step && step.use !== undefined) {
        var fragmentName = String(step.use || "")
        if (!validName(fragmentName)) {
            failures.push("steps[" + String(index) + "] use requires fragment name")
        } else if (!script.fragments || !script.fragments[fragmentName]) {
            failures.push("steps[" + String(index) + "] references unknown fragment " + fragmentName)
        }
        return
    }

    var type = String(step && step.type || "")
    if (!validName(type)) {
        failures.push("steps[" + String(index) + "] missing type")
        return
    }

    if (type === "selectTool") {
        if (!validName(step.toolId)) {
            failures.push("steps[" + String(index) + "] selectTool requires toolId")
        }
        return
    }

    if (type === "setToolParameter") {
        if (!validName(step.parameter)) {
            failures.push("steps[" + String(index) + "] setToolParameter requires parameter")
        }
        if (!step || step.value === undefined) {
            failures.push("steps[" + String(index) + "] setToolParameter requires value")
        }
        return
    }

    if (type === "clickCanvas") {
        if (!pointFromStep(step, "point", script.points)) {
            failures.push("steps[" + String(index) + "] clickCanvas requires finite point or x/y")
        }
        return
    }

    if (type === "dragHandle") {
        if (!validName(step.object || step.objectId)) {
            failures.push("steps[" + String(index) + "] dragHandle requires object")
        }
        if (!validName(step.handleId)) {
            failures.push("steps[" + String(index) + "] dragHandle requires handleId")
        }
        if (!pointFromStep(step, "to", script.points)) {
            failures.push("steps[" + String(index) + "] dragHandle requires to point")
        }
        return
    }

    if (type === "dragObject") {
        if (!validName(step.object || step.objectId)) {
            failures.push("steps[" + String(index) + "] dragObject requires object")
        }
        if (!pointFromStep(step, "to", script.points)
                && (!Number.isFinite(Number(step.dx)) || !Number.isFinite(Number(step.dy)))) {
            failures.push("steps[" + String(index) + "] dragObject requires to point or finite dx/dy")
        }
        return
    }

    if (type === "marqueeSelect") {
        if (!pointFromStep(step, "from", script.points) || !pointFromStep(step, "to", script.points)) {
            failures.push("steps[" + String(index) + "] marqueeSelect requires from/to points")
        }
        return
    }

    if (type === "panCanvas") {
        if (!Number.isFinite(Number(step.dx)) || !Number.isFinite(Number(step.dy))) {
            failures.push("steps[" + String(index) + "] panCanvas requires finite dx/dy")
        }
        return
    }

    if (type === "zoomCanvas") {
        if (!Number.isFinite(Number(step.factor))) {
            failures.push("steps[" + String(index) + "] zoomCanvas requires finite factor")
        }
        return
    }

    failures.push("steps[" + String(index) + "] unknown type " + type)
}

function validateExpectedBudgets(script, failures) {
    var modes = script && script.expect && script.expect.metricsByMode ? script.expect.metricsByMode : ({})
    var names = Object.keys(modes)
    for (var index = 0; index < names.length; ++index) {
        var value = modes[names[index]]
        if (typeof value === "string") {
            if (!script.budgets || !script.budgets[value]) {
                failures.push("expect.metricsByMode." + names[index] + " references unknown budget " + value)
            }
        } else if (!value || typeof value !== "object" || Array.isArray(value)) {
            failures.push("expect.metricsByMode." + names[index] + " must be a budget object or name")
        }
    }
}

function validateScript(script, library) {
    var source = composeScript(script, library)
    var failures = []
    if (!source || typeof source !== "object") {
        return {
            ok: false,
            failures: ["script must be an object"]
        }
    }
    if (!validName(source.name)) {
        failures.push("script missing name")
    }
    validateLibrary(source, failures)
    var steps = asArray(source.steps)
    if (steps.length <= 0) {
        failures.push("script requires at least one step")
    }
    for (var index = 0; index < steps.length; ++index) {
        validateStep(steps[index], index, source, failures)
    }
    validateExpectedBudgets(source, failures)
    return {
        ok: failures.length === 0,
        failures: failures
    }
}

function normalizeStep(step, script) {
    var type = String(step && step.type || "")
    if (type === "selectTool") {
        return {
            type: "selectTool",
            toolId: String(step.toolId || "")
        }
    }
    if (type === "clickCanvas") {
        return {
            type: "clickCanvas",
            point: pointFromStep(step, "point", script.points)
        }
    }
    if (type === "setToolParameter") {
        return {
            type: "setToolParameter",
            parameter: String(step.parameter || ""),
            value: cloneValue(step.value)
        }
    }
    if (type === "dragHandle") {
        return {
            type: "dragHandle",
            object: String(step.object || step.objectId || ""),
            handleId: String(step.handleId || ""),
            from: pointFromStep(step, "from", script.points),
            to: pointFromStep(step, "to", script.points),
            pointerMoves: Math.max(1, Math.round(finiteNumber(step.pointerMoves, 8)))
        }
    }
    if (type === "dragObject") {
        return {
            type: "dragObject",
            object: String(step.object || step.objectId || ""),
            from: pointFromStep(step, "from", script.points),
            to: pointFromStep(step, "to", script.points),
            dx: Number.isFinite(Number(step.dx)) ? Number(step.dx) : 0,
            dy: Number.isFinite(Number(step.dy)) ? Number(step.dy) : 0,
            pointerMoves: Math.max(1, Math.round(finiteNumber(step.pointerMoves, 8)))
        }
    }
    if (type === "marqueeSelect") {
        return {
            type: "marqueeSelect",
            from: pointFromStep(step, "from", script.points),
            to: pointFromStep(step, "to", script.points),
            pointerMoves: Math.max(1, Math.round(finiteNumber(step.pointerMoves, 8)))
        }
    }
    if (type === "panCanvas") {
        return {
            type: "panCanvas",
            dx: finiteNumber(step.dx, 0),
            dy: finiteNumber(step.dy, 0)
        }
    }
    if (type === "zoomCanvas") {
        return {
            type: "zoomCanvas",
            factor: finiteNumber(step.factor, 1),
            point: pointFromStep(step, "point", script.points)
        }
    }
    return cloneValue(step)
}

function expandStepList(steps, script, stack, failures) {
    var result = []
    var list = asArray(steps)
    for (var index = 0; index < list.length; ++index) {
        var step = list[index] || ({})
        if (step.use !== undefined) {
            var fragmentName = String(step.use || "")
            if (stack.indexOf(fragmentName) >= 0) {
                failures.push("fragment cycle detected at " + fragmentName)
                continue
            }
            var fragment = script.fragments && script.fragments[fragmentName]
            if (!fragment) {
                failures.push("unknown fragment " + fragmentName)
                continue
            }
            result = result.concat(expandStepList(fragment, script, stack.concat([fragmentName]), failures))
        } else {
            result.push(normalizeStep(step, script))
        }
    }
    return result
}

function expandedSteps(script, library) {
    var source = composeScript(script, library)
    var failures = []
    var steps = expandStepList(source.steps, source, [], failures)
    return {
        ok: failures.length === 0,
        steps: steps,
        failures: failures
    }
}

function metricsBudgetsByMode(script, library) {
    var source = composeScript(script, library)
    var modes = source && source.expect && source.expect.metricsByMode ? source.expect.metricsByMode : ({})
    var result = {}
    var names = Object.keys(modes)
    for (var index = 0; index < names.length; ++index) {
        var value = modes[names[index]]
        result[names[index]] = typeof value === "string"
                ? cloneValue(source.budgets && source.budgets[value] || ({}))
                : cloneValue(value || ({}))
    }
    return result
}

function executionPlan(script, library) {
    var source = composeScript(script, library)
    var validation = validateScript(source)
    if (!validation.ok) {
        return {
            ok: false,
            failures: validation.failures,
            plan: {}
        }
    }
    var expanded = expandedSteps(source)
    if (!expanded.ok) {
        return {
            ok: false,
            failures: expanded.failures,
            plan: {}
        }
    }
    return {
        ok: true,
        failures: [],
        plan: {
            name: String(source.name || ""),
            profile: String(source.profile || ""),
            steps: expanded.steps,
            expect: cloneValue(source.expect || ({})) || ({}),
            metricsByMode: metricsBudgetsByMode(source)
        }
    }
}
