import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../../style"
import "../../components"

Item {
    id: drawingLeftPanel

    property string dataUi: "drawing_tool_left_panel"
    property string dataState: "draftsman_native_drawing"
    property var controller: null

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
                    label: shortToolLabel(mode.id, mode.label),
                    tooltip: toolTooltip(mode.id, mode.label),
                    action: "tool"
                })
            }
        }
        return rows
    }

    function shortToolLabel(toolId, fallback) {
        var id = String(toolId || "")
        if (id === "select_move") {
            return "Select"
        }
        if (id === "anchor_points") {
            return "Point"
        }
        if (id === "line_polyline") {
            return "Line"
        }
        if (id === "circle_arc") {
            return "Circle"
        }
        if (id === "rectangle_polygon") {
            return "Rect"
        }
        if (id === "regular_polygon") {
            return "Polygon"
        }
        if (id === "image_reference_frame") {
            return "Image"
        }
        if (id === "ascii_crop_frame") {
            return "ASCII"
        }
        return String(fallback || id)
    }

    function toolTooltip(toolId, fallback) {
        var id = String(toolId || "")
        if (id === "select_move") {
            return "Select and move generated canvas objects."
        }
        if (id === "anchor_points") {
            return "Place a point marker."
        }
        if (id === "line_polyline") {
            return "Draw straight lines or polyline variants."
        }
        if (id === "circle_arc") {
            return "Draw circles or arcs."
        }
        if (id === "rectangle_polygon") {
            return "Draw two-corner rectangles."
        }
        if (id === "regular_polygon") {
            return "Draw regular polygons using side count and rotation."
        }
        if (id === "image_reference_frame") {
            return "Place an image reference frame."
        }
        if (id === "ascii_crop_frame") {
            return "Mark an ASCII crop/export region."
        }
        return String(fallback || id)
    }

    function isSelected(row) {
        if (!drawingLeftPanel.controller || !row) {
            return false
        }
        return String(drawingLeftPanel.controller.selectedDrawingToolId || "") === String(row.id || "")
    }

    function handleRowClicked(row) {
        if (!drawingLeftPanel.controller || !row) {
            return
        }
        if (row.action === "tool") {
            drawingLeftPanel.controller.selectDrawingTool(row.id)
        }
    }

    ColumnLayout {
        id: content
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        spacing: UiStyle.space2

        Repeater {
            model: toolRows()
            delegate: UiListRow {
                Layout.fillWidth: true
                label: modelData.label
                meta: ""
                tooltip: modelData.tooltip
                selected: isSelected(modelData)
                clickable: true
                onClicked: handleRowClicked(modelData)
            }
        }
    }
}
