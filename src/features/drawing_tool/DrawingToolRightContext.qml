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
        if (toolId === "regular_polygon") {
            return String(drawingRightContext.controller.drawingRegularPolygonSides || 6) + " sides"
        }
        if (toolId === "anchor_points") {
            return "Point"
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
        if (toolId === "regular_polygon") {
            rows.push({ id: "regular_polygon", kind: "tool", label: "Polygon", value: "regular_polygon", tooltip: "Regular polygon tool" })
            return rows
        }
        if (toolId === "anchor_points") {
            rows.push({ id: "anchor_point", kind: "point_variant", label: "Point", value: "point", tooltip: "Place a simple point marker" })
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
        if (kind === "point_variant") {
            return true
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

    function optionRows() {
        if (!drawingRightContext.controller) {
            return []
        }
        var toolId = selectedToolId()
        if (toolId === "circle_arc") {
            return [
                { id: "arc_start", label: "Start", field: "circle_arc_start_angle_deg", value: String(Number(drawingRightContext.controller.drawingCircleArcStartAngleDeg || 0)), suffix: "deg", visible: drawingRightContext.controller.drawingCircleArcMode === "arc" },
                { id: "arc_end", label: "End", field: "circle_arc_end_angle_deg", value: String(Number(drawingRightContext.controller.drawingCircleArcEndAngleDeg || 90)), suffix: "deg", visible: drawingRightContext.controller.drawingCircleArcMode === "arc" }
            ]
        }
        if (toolId === "regular_polygon") {
            return [
                { id: "polygon_sides", label: "Sides", field: "regular_polygon_sides", value: String(Number(drawingRightContext.controller.drawingRegularPolygonSides || 6)), suffix: "", visible: true },
                { id: "polygon_rotation", label: "Rotate", field: "regular_polygon_rotation_deg", value: String(Number(drawingRightContext.controller.drawingRegularPolygonRotationDeg || 30)), suffix: "deg", visible: true }
            ]
        }
        return []
    }

    function visibleOptionCount() {
        var rows = optionRows()
        var count = 0
        for (var index = 0; index < rows.length; ++index) {
            if (rows[index].visible !== false) {
                count += 1
            }
        }
        return count
    }

    function optionHelpText() {
        var toolId = selectedToolId()
        if (toolId === "line_polyline") {
            return drawingRightContext.controller && drawingRightContext.controller.drawingLineVariant === "polyline"
                ? "Creates connected path objects from two clicks for now."
                : "Creates one straight segment from two clicks."
        }
        if (toolId === "circle_arc") {
            return drawingRightContext.controller && drawingRightContext.controller.drawingCircleArcMode === "arc"
                ? "Arc uses center, radius, start angle, and end angle."
                : "Circle uses center and radius."
        }
        if (toolId === "regular_polygon") {
            return "Polygon uses center, radius, side count, and rotation."
        }
        if (toolId === "rectangle_polygon") {
            return "Rect uses two corners."
        }
        if (toolId === "image_reference_frame") {
            return "Image frame reserves source-image bounds."
        }
        if (toolId === "ascii_crop_frame") {
            return "ASCII crop marks the future export region."
        }
        if (toolId === "anchor_points") {
            return "Point places a simple marker on the canvas."
        }
        return ""
    }

    function setNumericOption(field, value) {
        if (!drawingRightContext.controller) {
            return
        }
        drawingRightContext.controller.updateDrawingToolParameterField(String(field || ""), value)
    }

    component OptionField: RowLayout {
        property string label: ""
        property string field: ""
        property string valueText: ""
        property string suffix: ""

        spacing: UiStyle.space6

        Text {
            Layout.fillWidth: true
            text: parent.label
            color: UiStyle.colorTextMuted
            font.family: UiStyle.fontSans
            font.pixelSize: UiStyle.fontSizeSm
            elide: Text.ElideRight
        }

        UiTextField {
            Layout.preferredWidth: 74
            Layout.preferredHeight: 24
            text: parent.valueText
            property string lastCommittedText: parent.valueText

            function commit() {
                if (text === lastCommittedText) {
                    return
                }
                lastCommittedText = text
                drawingRightContext.setNumericOption(parent.field, text)
            }

            onAccepted: commit()
            onEditingFinished: commit()
        }

        Text {
            Layout.preferredWidth: parent.suffix.length > 0 ? 28 : 0
            visible: parent.suffix.length > 0
            text: parent.suffix
            color: UiStyle.colorTextFaint
            font.family: UiStyle.fontSans
            font.pixelSize: UiStyle.fontSizeXs
            elide: Text.ElideRight
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
            spacing: UiStyle.space8

            UiPanel {
                Layout.fillWidth: true
                Layout.preferredHeight: Math.max(132, 104 + (variantRows().length > 0 ? 34 : 0) + visibleOptionCount() * 32)

                ColumnLayout {
                    anchors.fill: parent
                    spacing: UiStyle.space6

                    UiSectionHeader {
                        title: "Tool"
                        Layout.fillWidth: true
                    }
                    UiListRow {
                        Layout.fillWidth: true
                        label: "Tool"
                        meta: selectedToolLabel()
                        metaMaxWidth: 360
                    }

                    UiSectionHeader {
                        title: "Variant"
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
                                Layout.preferredWidth: Math.max(64, implicitWidth)
                                Layout.preferredHeight: 24
                                label: modelData.label
                                tooltip: modelData.tooltip
                                selected: isVariantSelected(modelData)
                                onClicked: setVariant(modelData)
                            }
                        }
                    }

                    Repeater {
                        model: optionRows()
                        delegate: OptionField {
                            Layout.fillWidth: true
                            visible: modelData.visible !== false
                            label: modelData.label
                            field: modelData.field
                            valueText: modelData.value
                            suffix: modelData.suffix
                        }
                    }

                    Text {
                        Layout.fillWidth: true
                        visible: optionHelpText().length > 0
                        text: optionHelpText()
                        color: UiStyle.colorTextFaint
                        font.family: UiStyle.fontSans
                        font.pixelSize: UiStyle.fontSizeXs
                        wrapMode: Text.WordWrap
                    }
                }
            }
        }
    }
}
