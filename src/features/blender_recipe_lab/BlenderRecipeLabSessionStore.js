.pragma library

var state = {
    revision: 0,
    activeLane: "grid",
    activeOperationId: "",
    activeOperationLabel: "",
    activeScriptId: "",
    activeScriptLabel: "",
    activeRecipeNodeId: "",
    asciiPreviewDocumentId: "ascii_preview",
    dryRunStatus: "idle",
    materializeStatus: "idle",
    lastMessage: "ready"
}

function clone(value) {
    return JSON.parse(JSON.stringify(value))
}

function snapshot() {
    return clone(state)
}

function touch(message) {
    state.revision += 1
    if (message !== undefined) {
        state.lastMessage = String(message)
    }
}

function setValue(key, value, message) {
    var nextValue = value === undefined || value === null ? "" : String(value)
    if (state[key] === nextValue) {
        return false
    }
    state[key] = nextValue
    touch(message)
    return true
}

function setActiveLane(lane) {
    return setValue("activeLane", lane || "grid", "lane selected")
}

function setActiveOperation(id, label) {
    var changed = false
    changed = setValue("activeOperationId", id, "operation selected") || changed
    changed = setValue("activeOperationLabel", label || id, "operation selected") || changed
    return changed
}

function setActiveScript(id, label) {
    var changed = false
    changed = setValue("activeScriptId", id, "script selected") || changed
    changed = setValue("activeScriptLabel", label || id, "script selected") || changed
    return changed
}

function setActiveRecipeNode(id) {
    return setValue("activeRecipeNodeId", id, "recipe node selected")
}

function setAsciiPreviewDocument(id) {
    return setValue("asciiPreviewDocumentId", id || "ascii_preview", "ascii preview selected")
}

function setDryRunStatus(status) {
    return setValue("dryRunStatus", status || "idle", "dry run status changed")
}

function setMaterializeStatus(status) {
    return setValue("materializeStatus", status || "idle", "materialize status changed")
}
