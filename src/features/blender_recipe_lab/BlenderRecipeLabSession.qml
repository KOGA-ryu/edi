import QtQuick
import "BlenderRecipeLabSessionStore.js" as Store

QtObject {
    id: session

    property int revision: 0
    property string activeLane: "grid"
    property string activeOperationId: ""
    property string activeOperationLabel: ""
    property string activeScriptId: ""
    property string activeScriptLabel: ""
    property string activeRecipeNodeId: ""
    property string asciiPreviewDocumentId: "ascii_preview"
    property string dryRunStatus: "idle"
    property string materializeStatus: "idle"
    property string lastMessage: "ready"

    signal changed()

    function syncFromStore() {
        var next = Store.snapshot()
        revision = Number(next.revision || 0)
        activeLane = String(next.activeLane || "grid")
        activeOperationId = String(next.activeOperationId || "")
        activeOperationLabel = String(next.activeOperationLabel || "")
        activeScriptId = String(next.activeScriptId || "")
        activeScriptLabel = String(next.activeScriptLabel || "")
        activeRecipeNodeId = String(next.activeRecipeNodeId || "")
        asciiPreviewDocumentId = String(next.asciiPreviewDocumentId || "ascii_preview")
        dryRunStatus = String(next.dryRunStatus || "idle")
        materializeStatus = String(next.materializeStatus || "idle")
        lastMessage = String(next.lastMessage || "ready")
    }

    function setActiveLane(lane) {
        Store.setActiveLane(lane)
        syncFromStore()
        changed()
    }

    function setActiveOperation(id, label) {
        Store.setActiveOperation(id, label)
        syncFromStore()
        changed()
    }

    function setActiveScript(id, label) {
        Store.setActiveScript(id, label)
        syncFromStore()
        changed()
    }

    function setActiveRecipeNode(id) {
        Store.setActiveRecipeNode(id)
        syncFromStore()
        changed()
    }

    function setAsciiPreviewDocument(id) {
        Store.setAsciiPreviewDocument(id)
        syncFromStore()
        changed()
    }

    function setDryRunStatus(status) {
        Store.setDryRunStatus(status)
        syncFromStore()
        changed()
    }

    function setMaterializeStatus(status) {
        Store.setMaterializeStatus(status)
        syncFromStore()
        changed()
    }

    Component.onCompleted: syncFromStore()
}
