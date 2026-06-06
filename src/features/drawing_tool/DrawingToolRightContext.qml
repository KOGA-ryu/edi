import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../../style"
import "../../components"

Item {
    id: drawingRightContext

    property string dataUi: "drawing_tool_right_context"
    property string dataState: "draftsman_native_drawing"
    property var controller: null

    function selectedToolId() {
        return drawingRightContext.controller ? String(drawingRightContext.controller.selectedDrawingToolId || "") : ""
    }

    function selectedTool() {
        return drawingRightContext.controller ? drawingRightContext.controller.selectedDrawingTool() : ({})
    }

    function selectedToolLabel() {
        return String((selectedTool() || {}).label || selectedToolId() || "")
    }

    function activeVariantLabel() {
        if (!drawingRightContext.controller) {
            return ""
        }
        var toolId = selectedToolId()
        if (toolId === "line_polyline") {
            return drawingRightContext.controller.drawingLineVariant === "polyline" ? "Polyline" : "Straight"
        }
        if (toolId === "circle_arc") {
            return drawingRightContext.controller.drawingCircleArcMode === "arc" ? "Arc" : "Circle"
        }
        if (toolId === "rectangle_polygon") {
            return "Rect"
        }
        if (toolId === "image_reference_frame") {
            return "Image Frame"
        }
        if (toolId === "ascii_crop_frame") {
            return "ASCII Crop"
        }
        return ""
    }

    function variantRows() {
        if (!drawingRightContext.controller) {
            return []
        }
        var rows = []
        var toolId = selectedToolId()
        if (toolId === "line_polyline") {
            rows.push({ id: "line_straight", kind: "line_variant", label: "Straight", value: "straight", tooltip: "Draw straight line segments" })
            rows.push({ id: "line_polyline", kind: "line_variant", label: "Polyline", value: "polyline", tooltip: "Switch to polyline entry mode" })
            return rows
        }
        if (toolId === "circle_arc") {
            rows.push({ id: "circle_mode", kind: "circle_arc", label: "Circle", value: "circle", tooltip: "Circle mode" })
            rows.push({ id: "arc_mode", kind: "circle_arc", label: "Arc", value: "arc", tooltip: "Arc mode" })
            return rows
        }
        if (toolId === "rectangle_polygon" || toolId === "image_reference_frame" || toolId === "ascii_crop_frame") {
            rows.push({ id: "rectangle_polygon", kind: "tool", label: "Rect", value: "rectangle_polygon", tooltip: "Rectangle tool" })
            rows.push({ id: "image_reference_frame", kind: "tool", label: "Image Frame", value: "image_reference_frame", tooltip: "Image frame tool" })
            rows.push({ id: "ascii_crop_frame", kind: "tool", label: "ASCII Crop", value: "ascii_crop_frame", tooltip: "ASCII crop frame" })
            return rows
        }
        return []
    }

    function isVariantSelected(variant) {
        if (!drawingRightContext.controller || !variant) {
            return false
        }
        var kind = String(variant.kind || "")
        if (kind === "line_variant") {
            return String(drawingRightContext.controller.drawingLineVariant || "straight") === String(variant.value || "")
        }
        if (kind === "circle_arc") {
            return String(drawingRightContext.controller.drawingCircleArcMode || "circle") === String(variant.value || "")
        }
        if (kind === "tool") {
            return selectedToolId() === String(variant.value || "")
        }
        return false
    }

    function setVariant(variant) {
        if (!drawingRightContext.controller || !variant) {
            return
        }
        var kind = String(variant.kind || "")
        var value = String(variant.value || "")
        if (kind === "line_variant") {
            drawingRightContext.controller.setDrawingLineVariant(value)
            return
        }
        if (kind === "circle_arc") {
            drawingRightContext.controller.setDrawingCircleArcMode(value)
            return
        }
        if (kind === "tool") {
            drawingRightContext.controller.selectDrawingTool(value)
        }
    }

    Flickable {
        anchors.fill: parent
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        contentWidth: width
        contentHeight: content.implicitHeight

        ScrollBar.vertical: ScrollBar {
            policy: ScrollBar.AsNeeded
            width: 5
        }

        ColumnLayout {
            id: content
            width: parent.width
            spacing: UiStyle.space10

            UiPanel {
                Layout.fillWidth: true
                Layout.preferredHeight: Math.max(84, 46 + (variantRows().length > 0 ? variantRows().length * 34 : 0))

                ColumnLayout {
                    anchors.fill: parent
                    spacing: UiStyle.space6

                    UiSectionHeader {
                        title: "Selected Tool"
                        Layout.fillWidth: true
                    }
                    UiListRow {
                        Layout.fillWidth: true
                        label: "Tool"
                        meta: selectedToolLabel()
                        metaMaxWidth: 360
                    }
                    UiListRow {
                        Layout.fillWidth: true
                        label: "Variant"
                        meta: activeVariantLabel() || ""
                        metaMaxWidth: 360
                        visible: activeVariantLabel().length > 0
                    }

                    UiSectionHeader {
                        title: "Tool Variants"
                        Layout.fillWidth: true
                        visible: variantRows().length > 0
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: UiStyle.space4
                        visible: variantRows().length > 0

                        Repeater {
                            model: variantRows()
                            delegate: UiButton {
                                Layout.preferredWidth: Math.max(72, implicitWidth)
                                Layout.preferredHeight: 24
                                label: modelData.label
                                tooltip: modelData.tooltip
                                selected: isVariantSelected(modelData)
                                onClicked: setVariant(modelData)
                            }
                        }
                    }
                }
            }
        }
    }
}
