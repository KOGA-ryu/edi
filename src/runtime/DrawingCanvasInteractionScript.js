.pragma library
.import "DrawingCanvasInteractionMetrics.js" as CanvasInteractionMetrics

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

function expectedMetrics(fixture) {
    return fixture && fixture.expect && fixture.expect.metrics
            ? fixture.expect.metrics
            : ({})
}

function validateStep(step, index, failures) {
    var type = String(step && step.type || "")
    if (type.length === 0) {
        failures.push("steps[" + String(index) + "] missing type")
        return
    }
    if (type === "pointerDown" || type === "pointerMove" || type === "pointerUp") {
        if (!Number.isFinite(Number(step.x)) || !Number.isFinite(Number(step.y))) {
            failures.push("steps[" + String(index) + "] " + type + " requires finite x/y")
        }
        return
    }
    if (type === "select" && String(step.objectId || "").length === 0) {
        failures.push("steps[" + String(index) + "] select requires objectId")
        return
    }
    if (type === "key" && String(step.key || "").length === 0) {
        failures.push("steps[" + String(index) + "] key requires key")
    }
}

function validateFixture(fixture) {
    var failures = []
    if (!fixture || typeof fixture !== "object") {
        return {
            ok: false,
            failures: ["fixture must be an object"]
        }
    }
    if (String(fixture.name || "").length === 0) {
        failures.push("fixture missing name")
    }
    var steps = asArray(fixture.steps)
    if (steps.length <= 0) {
        failures.push("fixture requires at least one step")
    }
    for (var index = 0; index < steps.length; ++index) {
        validateStep(steps[index], index, failures)
    }
    var metrics = expectedMetrics(fixture)
    if (String(metrics.mode || "").length === 0) {
        failures.push("fixture expect.metrics.mode is required")
    }
    return {
        ok: failures.length === 0,
        failures: failures
    }
}

function pointerMoveCount(fixture) {
    var steps = asArray(fixture && fixture.steps)
    var count = 0
    for (var index = 0; index < steps.length; ++index) {
        if (String(steps[index] && steps[index].type || "") === "pointerMove") {
            ++count
        }
    }
    return count
}

function actualRecord(fixture) {
    if (fixture && fixture.actual && fixture.actual.metrics) {
        return fixture.actual.metrics
    }
    return fixture && fixture.metrics ? fixture.metrics : ({})
}

function compareExpectedState(fixture) {
    var failures = []
    var expectState = fixture && fixture.expect && fixture.expect.state ? fixture.expect.state : ({})
    var actualState = fixture && fixture.actual && fixture.actual.state ? fixture.actual.state : ({})
    var keys = Object.keys(expectState)
    for (var index = 0; index < keys.length; ++index) {
        var key = keys[index]
        var expected = expectState[key]
        var actual = actualState[key]
        if (Array.isArray(expected)) {
            if (asArray(actual).map(String).join(",") !== expected.map(String).join(",")) {
                failures.push("state." + key + " expected " + JSON.stringify(expected) + ", got " + JSON.stringify(actual))
            }
        } else if (typeof expected === "number") {
            if (finiteNumber(actual, Number.NaN) !== expected) {
                failures.push("state." + key + " expected " + String(expected) + ", got " + String(actual))
            }
        } else if (String(actual) !== String(expected)) {
            failures.push("state." + key + " expected " + String(expected) + ", got " + String(actual))
        }
    }
    return failures
}

function evaluateFixture(fixture) {
    var validation = validateFixture(fixture)
    if (!validation.ok) {
        return validation
    }
    var record = actualRecord(fixture)
    var budget = expectedMetrics(fixture)
    var budgetResult = CanvasInteractionMetrics.assertWithinBudget(record, budget)
    var failures = []
    if (!budgetResult.ok) {
        failures = failures.concat(budgetResult.failures)
    }
    var expectedMoves = pointerMoveCount(fixture)
    if (budget.assertPointerMovesMatchSteps === true && finiteNumber(record.pointerMoves, -1) !== expectedMoves) {
        failures.push("pointerMoves expected to match script pointerMove count " + String(expectedMoves) + ", got " + String(record.pointerMoves))
    }
    failures = failures.concat(compareExpectedState(fixture))
    return {
        ok: failures.length === 0,
        failures: failures
    }
}

function evaluateFixtures(fixtures) {
    var list = asArray(fixtures)
    var failures = []
    for (var index = 0; index < list.length; ++index) {
        var result = evaluateFixture(list[index])
        if (!result.ok) {
            failures.push({
                name: String(list[index] && list[index].name || "fixture_" + String(index)),
                failures: result.failures
            })
        }
    }
    return {
        ok: failures.length === 0,
        failures: failures
    }
}
