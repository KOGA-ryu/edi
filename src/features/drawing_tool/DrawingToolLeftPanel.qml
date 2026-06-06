import QtQuick
import QtQuick.Layouts
import "../../style"
import "../../components"

Item {
    id: drawingLeftPanel

    property string dataUi: "drawing_tool_left_panel"
    property string dataState: "draftsman_native_drawing"
    property var controller: null
    property var collapsedSections: ({})

    implicitHeight: content.implicitHeight

    function asArray(value) {
        if (!value) {
            return []
        }
        if (Array.isArray(value)) {
            return value
        }
        return []
    }

    function isWorkingToolId(toolId) {
        return ["select_move", "anchor_points", "line_polyline", "circle_arc", "rectangle_polygon", "regular_polygon", "image_reference_frame", "ascii_crop_frame"].indexOf(String(toolId || "")) >= 0
    }

    function sectionCollapsed(sectionId) {
        return collapsedSections[String(sectionId)] === true
    }

    function setSectionCollapsed(sectionId, collapsed) {
        var next = Object.assign({}, collapsedSections)
        next[String(sectionId)] = !!collapsed
        collapsedSections = next
    }

    function toggleSection(sectionId) {
        setSectionCollapsed(sectionId, !sectionCollapsed(sectionId))
    }

    function toolRows() {
        if (!drawingLeftPanel.controller) {
            return []
        }
        var rows = []
        var modes = asArray(drawingLeftPanel.controller.drawingToolModes)
        for (var index = 0; index < modes.length; ++index) {
            var mode = modes[index]
            if (isWorkingToolId(mode.id)) {
                rows.push({
                    id: String(mode.id),
                    label: String(mode.label || mode.id),
                    meta: String(mode.meta || ""),
                    action: "tool"
                })
            }
        }
        return rows
    }

    function layerRows() {
        if (!drawingLeftPanel.controller) {
            return []
        }
        return asArray(drawingLeftPanel.controller.drawingLayerStack)
    }

    function assetRows() {
        var result = []
        if (!drawingLeftPanel.controller) {
            return result
        }
        var sources = asArray(drawingLeftPanel.controller.drawingAssetSources)
        for (var sourceIndex = 0; sourceIndex < sources.length; ++sourceIndex) {
            var source = sources[sourceIndex]
            result.push({
                id: String(source.id || ""),
                label: String(source.label || source.id || "asset"),
                meta: String(source.meta || "asset"),
                action: ""
            })
        }
        var imageTools = asArray(drawingLeftPanel.controller.drawingImageTools)
        for (var toolIndex = 0; toolIndex < imageTools.length; ++toolIndex) {
            var tool = imageTools[toolIndex]
            result.push({
                id: String(tool.id || ""),
                label: String(tool.label || tool.id || "tool"),
                meta: String(tool.meta || "tool"),
                action: "external_tool"
            })
        }
        return result
    }

    function sectionRows(sectionId) {
        if (sectionId === "tools") {
            return toolRows()
        }
        if (sectionId === "layers") {
            return layerRows()
        }
        if (sectionId === "assets") {
            return assetRows()
        }
        return []
    }

    function isSelected(sectionId, row) {
        if (!drawingLeftPanel.controller) {
            return false
        }
        if (!row) {
            return false
        }
        if (sectionId === "tools") {
            return String(drawingLeftPanel.controller.selectedDrawingToolId || "") === String(row.id || "")
        }
        if (sectionId === "layers") {
            return String(drawingLeftPanel.controller.selectedDrawingLayerId || "") === String(row.id || "")
        }
        return false
    }

    function handleRowClicked(sectionId, row) {
        if (!drawingLeftPanel.controller || !row) {
            return
        }
        if (row.action === "tool") {
            drawingLeftPanel.controller.selectDrawingTool(row.id)
            return
        }
        if (row.action === "external_tool") {
            drawingLeftPanel.controller.selectDrawingExternalTool(row.id)
            return
        }
        if (sectionId === "layers") {
            drawingLeftPanel.controller.selectDrawingLayer(row.id)
        }
    }

    ColumnLayout {
        id: content
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        spacing: UiStyle.space6

        Repeater {
            model: [
                { id: "tools", title: "Tools", hint: "work" },
                { id: "layers", title: "Layers", hint: "stack" },
                { id: "assets", title: "Assets", hint: "tools" }
            ]

            delegate: ColumnLayout {
                id: sectionGroup
                property var section: modelData
                Layout.fillWidth: true
                spacing: UiStyle.space2
                property var rows: sectionRows(section.id)

                UiListRow {
                    Layout.fillWidth: true
                    label: (drawingLeftPanel.sectionCollapsed(section.id) ? "▸ " : "▾ ") + section.title
                    meta: String(rows.length) + " " + section.hint
                    selected: false
                    clickable: rows.length > 0
                    metaMaxWidth: 84
                    onClicked: drawingLeftPanel.toggleSection(section.id)
                }

                Repeater {
                    model: drawingLeftPanel.sectionCollapsed(section.id) ? [] : sectionGroup.rows
                    delegate: UiListRow {
                        Layout.fillWidth: true
                        Layout.leftMargin: UiStyle.space2
                        label: modelData.label
                        meta: modelData.meta
                        selected: isSelected(section.id, modelData)
                        clickable: section.id === "layers" || String(modelData.action || "").length > 0
                        onClicked: handleRowClicked(section.id, modelData)
                    }
                }
            }
        }
    }
}
