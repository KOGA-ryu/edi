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
    property var collapsedSections: ({
        format: true,
        precision: true
    })

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

    function selectedToolId() {
        return drawingRightContext.controller ? String(drawingRightContext.controller.selectedDrawingToolId || "") : ""
    }

    function selectedTool() {
        return drawingRightContext.controller ? drawingRightContext.controller.selectedDrawingTool() : ({})
    }

    function pendingActive() {
        return !!(drawingRightContext.controller && drawingRightContext.controller.drawingPendingShapeActive)
    }

    function selectedGeneratedObject() {
        if (!drawingRightContext.controller) {
            return ({})
        }
        var object = drawingRightContext.controller.selectedDrawingObject() || ({})
        return String(object.id || "").indexOf("script_") === 0 ? object : ({})
    }

    function selectedGeneratedObjectActive() {
        return String(selectedGeneratedObject().id || "").length > 0
    }

    function selectedGeneratedObjectLabel() {
        var object = selectedGeneratedObject()
        return String(object.label || object.kind || object.id || "object")
    }

    function selectedGeneratedObjectMeta() {
        var object = selectedGeneratedObject()
        var kind = String(object.kind || "object")
        var id = String(object.id || "")
        return id.length > 0 ? kind + " / " + id : kind
    }

    function compactNumber(value) {
        var number = Number(value)
        if (!Number.isFinite(number)) {
            return "0"
        }
        var rounded = Math.round(number * 1000) / 1000
        return String(rounded)
    }

    function objectPointValue(object, pointField, index, fallbackField) {
        var point = asArray(object[pointField])
        if (point.length > index) {
            return compactNumber(point[index])
        }
        return compactNumber(Number(object[fallbackField] || 0) * Number(drawingRightContext.controller ? drawingRightContext.controller.drawingCanvasSizePx : 512))
    }

    function objectRectValue(object, index, fallbackField) {
        var rect = asArray(object.rect_px)
        if (rect.length > index) {
            return compactNumber(rect[index])
        }
        return compactNumber(Number(object[fallbackField] || 0) * Number(drawingRightContext.controller ? drawingRightContext.controller.drawingCanvasSizePx : 512))
    }

    function objectEditRows() {
        var object = selectedGeneratedObject()
        var kind = String(object.kind || "")
        if (kind === "point" || kind === "tone_probe") {
            return [
                { label: "x", field: "x_px", value: objectPointValue(object, "point_px", 0, "x"), suffix: "px" },
                { label: "y", field: "y_px", value: objectPointValue(object, "point_px", 1, "y"), suffix: "px" }
            ]
        }
        if (kind === "line" || kind === "glyph_baseline") {
            return [
                { label: "x1", field: "x1_px", value: objectPointValue(object, "from_px", 0, "x1"), suffix: "px" },
                { label: "y1", field: "y1_px", value: objectPointValue(object, "from_px", 1, "y1"), suffix: "px" },
                { label: "x2", field: "x2_px", value: objectPointValue(object, "to_px", 0, "x2"), suffix: "px" },
                { label: "y2", field: "y2_px", value: objectPointValue(object, "to_px", 1, "y2"), suffix: "px" }
            ]
        }
        if (kind === "circle" || kind === "arc") {
            var rows = [
                { label: "cx", field: "cx_px", value: objectPointValue(object, "center_px", 0, "cx"), suffix: "px" },
                { label: "cy", field: "cy_px", value: objectPointValue(object, "center_px", 1, "cy"), suffix: "px" },
                { label: "r", field: "radius_px", value: compactNumber(object.radius_px || 0), suffix: "px" }
            ]
            if (kind === "arc") {
                rows.push({ label: "start", field: "start_angle_deg", value: compactNumber(object.start_angle_deg || 0), suffix: "deg" })
                rows.push({ label: "end", field: "end_angle_deg", value: compactNumber(object.end_angle_deg || 0), suffix: "deg" })
            }
            return rows
        }
        if (kind === "rectangle" || kind === "image_reference_frame" || kind === "ascii_crop_frame" || kind === "ascii_cell_region") {
            return [
                { label: "x", field: "x_px", value: objectRectValue(object, 0, "x"), suffix: "px" },
                { label: "y", field: "y_px", value: objectRectValue(object, 1, "y"), suffix: "px" },
                { label: "w", field: "width_px", value: objectRectValue(object, 2, "width"), suffix: "px" },
                { label: "h", field: "height_px", value: objectRectValue(object, 3, "height"), suffix: "px" }
            ]
        }
        if (kind === "polygon") {
            return [
                { label: "cx", field: "cx_px", value: objectPointValue(object, "center_px", 0, "cx"), suffix: "px" },
                { label: "cy", field: "cy_px", value: objectPointValue(object, "center_px", 1, "cy"), suffix: "px" },
                { label: "r", field: "radius_px", value: compactNumber(object.radius_px || 0), suffix: "px" },
                { label: "sides", field: "sides", value: compactNumber(object.sides || 6), suffix: "" },
                { label: "rot", field: "rotation_deg", value: compactNumber(object.rotation_deg || 0), suffix: "deg" }
            ]
        }
        return []
    }

    function metadataFieldValue(field) {
        var object = selectedGeneratedObject()
        var fieldId = String(field || "")
        if (fieldId === "tags") {
            return asArray(object.tags).join(", ")
        }
        return String(object[fieldId] || "")
    }

    function metadataRows() {
        return [
            { label: "role", field: "role", value: metadataFieldValue("role"), placeholder: "wall, panel, cutout" },
            { label: "material", field: "material", value: metadataFieldValue("material"), placeholder: "stone, metal, glass" },
            { label: "group", field: "export_group", value: metadataFieldValue("export_group"), placeholder: "room_a, collision, shell" },
            { label: "tags", field: "tags", value: metadataFieldValue("tags"), placeholder: "comma, separated" }
        ]
    }

    function metadataPresetRows() {
        return drawingRightContext.controller && typeof drawingRightContext.controller.drawingMetadataPresetRows === "function"
            ? drawingRightContext.controller.drawingMetadataPresetRows(drawingRightContext.controller.revision)
            : []
    }

    function setObjectField(field, value) {
        if (drawingRightContext.controller) {
            drawingRightContext.controller.updateSelectedDrawingObjectField(String(field || ""), value)
        }
    }

    function setObjectMetadataField(field, value) {
        if (drawingRightContext.controller) {
            drawingRightContext.controller.updateSelectedDrawingObjectMetadataField(String(field || ""), value)
        }
    }

    function metadataTags() {
        return asArray(selectedGeneratedObject().tags)
    }

    function metadataPresetSelected(row, option) {
        var field = String(row.field || "")
        var value = String(option || "")
        if (field === "tags") {
            return metadataTags().indexOf(value) >= 0
        }
        return metadataFieldValue(field) === value
    }

    function applyMetadataPreset(row, option) {
        var field = String(row.field || "")
        var value = String(option || "")
        if (!field.length || !value.length) {
            return
        }
        if (field === "tags") {
            var tags = metadataTags().slice()
            var index = tags.indexOf(value)
            if (index >= 0) {
                tags.splice(index, 1)
            } else {
                tags.push(value)
            }
            setObjectMetadataField(field, tags.join(", "))
            return
        }
        setObjectMetadataField(field, metadataFieldValue(field) === value ? "" : value)
    }

    function selectedToolLabel() {
        return shortToolLabel(selectedToolId(), (selectedTool() || {}).label)
    }

    function sectionCollapsed(sectionId) {
        return collapsedSections[String(sectionId)] === true
    }

    function toggleSection(sectionId) {
        var next = Object.assign({}, collapsedSections)
        next[String(sectionId)] = !sectionCollapsed(sectionId)
        collapsedSections = next
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

    function toolRows() {
        if (!drawingRightContext.controller) {
            return []
        }
        var rows = []
        var modes = asArray(drawingRightContext.controller.drawingToolModes)
        for (var index = 0; index < modes.length; ++index) {
            var mode = modes[index]
            if (isWorkingToolId(mode.id)) {
                rows.push({
                    id: String(mode.id),
                    label: shortToolLabel(mode.id, mode.label),
                    tooltip: toolTooltip(mode.id, mode.label)
                })
            }
        }
        return rows
    }

    function variantRows() {
        if (!drawingRightContext.controller) {
            return []
        }
        var rows = []
        var toolId = selectedToolId()
        if (toolId === "line_polyline") {
            rows.push({ id: "line_straight", label: "Straight", tooltip: "Draw straight line segments." })
            rows.push({ id: "line_polyline", label: "Polyline", tooltip: "Draw connected polyline segments." })
            rows.push({ id: "line_arrow", label: "Arrow", tooltip: "Arrow variant state. Arrowhead rendering is a later core pass." })
            return rows
        }
        if (toolId === "circle_arc") {
            rows.push({ id: "circle_full", label: "Full", tooltip: "Draw full circles." })
            rows.push({ id: "circle_arc", label: "Arc", tooltip: "Draw arc objects using start/end angles." })
            return rows
        }
        if (toolId === "rectangle_polygon") {
            rows.push({ id: "rect_box", label: "Box", tooltip: "Draw a two-corner rectangle." })
            rows.push({ id: "rect_rounded", label: "Rounded", tooltip: "Rounded rectangle variant state. Rounded rendering is a later core pass." })
            rows.push({ id: "rect_frame", label: "Frame", tooltip: "Frame rectangle variant state for drafting bounds." })
            return rows
        }
        if (toolId === "regular_polygon") {
            rows.push({ id: "polygon_triangle", label: "Triangle", tooltip: "Draw a three-sided polygon." })
            rows.push({ id: "polygon_hex", label: "Hex", tooltip: "Draw a six-sided polygon." })
            rows.push({ id: "polygon_free", label: "Free", tooltip: "Use custom side count and rotation." })
            return rows
        }
        if (toolId === "image_reference_frame") {
            rows.push({ id: "image_frame", label: "Frame", tooltip: "Place an image reference frame." })
            rows.push({ id: "image_crop", label: "Crop", tooltip: "Image crop-box variant state for later image binding." })
            return rows
        }
        if (toolId === "ascii_crop_frame") {
            rows.push({ id: "ascii_crop", label: "Crop", tooltip: "Mark an ASCII export crop region." })
            rows.push({ id: "ascii_glyph_block", label: "Glyph", tooltip: "Glyph block variant state for later ASCII tooling." })
            return rows
        }
        return []
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
        if (toolId === "regular_polygon" && String(drawingRightContext.controller.selectedDrawingVariantId || "") === "polygon_free") {
            return [
                { id: "polygon_sides", label: "Sides", field: "regular_polygon_sides", value: String(Number(drawingRightContext.controller.drawingRegularPolygonSides || 6)), suffix: "", visible: true },
                { id: "polygon_rotation", label: "Rotate", field: "regular_polygon_rotation_deg", value: String(Number(drawingRightContext.controller.drawingRegularPolygonRotationDeg || 30)), suffix: "deg", visible: true }
            ]
        }
        return []
    }

    function visibleOptionRows() {
        var result = []
        var rows = optionRows()
        for (var index = 0; index < rows.length; ++index) {
            if (rows[index].visible !== false) {
                result.push(rows[index])
            }
        }
        return result
    }

    function activeVariantText() {
        var rows = variantRows()
        var selectedVariantId = String(drawingRightContext.controller ? drawingRightContext.controller.selectedDrawingVariantId : "")
        for (var index = 0; index < rows.length; ++index) {
            if (String(rows[index].id || "") === selectedVariantId) {
                return String(rows[index].label || "")
            }
        }
        var toolId = selectedToolId()
        if (toolId === "line_polyline") {
            return drawingRightContext.controller && drawingRightContext.controller.drawingLineVariant === "polyline" ? "Polyline" : "Straight"
        }
        if (toolId === "circle_arc") {
            return drawingRightContext.controller && drawingRightContext.controller.drawingCircleArcMode === "arc" ? "Arc" : "Circle"
        }
        if (toolId === "regular_polygon") {
            var polygonVariant = String(drawingRightContext.controller ? drawingRightContext.controller.selectedDrawingVariantId : "")
            if (polygonVariant === "polygon_triangle") {
                return "Triangle"
            }
            if (polygonVariant === "polygon_hex") {
                return "Hex"
            }
            return String(drawingRightContext.controller ? drawingRightContext.controller.drawingRegularPolygonSides : 6) + " sides"
        }
        return selectedToolLabel()
    }

    function isVariantSelected(variant) {
        if (!drawingRightContext.controller || !variant) {
            return false
        }
        return String(drawingRightContext.controller.selectedDrawingVariantId || "") === String(variant.id || "")
    }

    function setVariant(variant) {
        if (!drawingRightContext.controller || !variant) {
            return
        }
        drawingRightContext.controller.setSelectedDrawingVariantId(String(variant.id || ""))
    }

    function setNumericOption(field, value) {
        if (drawingRightContext.controller) {
            drawingRightContext.controller.updateDrawingToolParameterField(String(field || ""), value)
        }
    }

    function setStyleField(fieldId, value) {
        if (!drawingRightContext.controller) {
            return
        }
        if (fieldId === "stroke") {
            drawingRightContext.controller.setDrawingStrokeColor(value)
        } else if (fieldId === "fill") {
            drawingRightContext.controller.setDrawingFillColor(value)
        } else if (fieldId === "width") {
            drawingRightContext.controller.setDrawingLineThickness(value)
        } else if (fieldId === "opacity") {
            drawingRightContext.controller.setDrawingStrokeOpacity(value)
        }
    }

    function styleLineStyle() {
        return String(drawingRightContext.controller ? drawingRightContext.controller.drawingLineStyle : "solid")
    }

    function snapEnabled() {
        return !!(drawingRightContext.controller && drawingRightContext.controller.drawingSnapGridEnabled)
    }

    function gridVisible() {
        return !!(drawingRightContext.controller && drawingRightContext.controller.drawingGridVisible)
    }

    function objectSnapEnabled() {
        return !!(drawingRightContext.controller && drawingRightContext.controller.drawingObjectSnapEnabled)
    }

    function swatchColor(value, fallback) {
        var raw = String(value || "").trim()
        return raw.length > 0 ? raw : fallback
    }

    component SectionHeader: Rectangle {
        property string sectionId: ""
        property string title: ""
        property string meta: ""

        Layout.fillWidth: true
        implicitHeight: 24
        color: UiStyle.colorTransparent
        border.width: UiStyle.borderNone

        RowLayout {
            anchors.fill: parent
            spacing: UiStyle.space6

            Text {
                Layout.fillWidth: true
                text: (drawingRightContext.sectionCollapsed(parent.parent.sectionId) ? "+ " : "- ") + parent.parent.title
                color: UiStyle.colorText
                font.family: UiStyle.fontSans
                font.pixelSize: UiStyle.fontSizeSm
                font.weight: UiStyle.fontWeightSemiBold
                elide: Text.ElideRight
            }

            Text {
                text: parent.parent.meta
                visible: parent.parent.meta.length > 0
                color: UiStyle.colorTextFaint
                font.family: UiStyle.fontSans
                font.pixelSize: UiStyle.fontSizeXs
                elide: Text.ElideRight
            }
        }

        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onClicked: drawingRightContext.toggleSection(parent.sectionId)
        }
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

    component ColorField: RowLayout {
        property string fieldId: ""
        property string valueText: ""
        property string fallbackColor: UiStyle.colorControl

        spacing: UiStyle.space4

        Rectangle {
            Layout.preferredWidth: 20
            Layout.preferredHeight: 20
            radius: UiStyle.radiusSm
            color: drawingRightContext.swatchColor(parent.valueText, parent.fallbackColor)
            border.width: UiStyle.borderThin
            border.color: UiStyle.colorPanelRaised
        }

        UiTextField {
            Layout.fillWidth: true
            Layout.preferredHeight: 24
            text: parent.valueText
            property string lastCommittedText: parent.valueText

            function commit() {
                if (text === lastCommittedText) {
                    return
                }
                lastCommittedText = text
                drawingRightContext.setStyleField(parent.fieldId, text)
            }

            onAccepted: commit()
            onEditingFinished: commit()
        }
    }

    component StyleNumberField: UiTextField {
        property string fieldId: ""
        property string valueText: ""

        Layout.preferredWidth: 56
        Layout.preferredHeight: 24
        horizontalAlignment: TextInput.AlignHCenter
        text: valueText
        property string lastCommittedText: valueText

        function commit() {
            if (text === lastCommittedText) {
                return
            }
            lastCommittedText = text
            drawingRightContext.setStyleField(fieldId, text)
        }

        onAccepted: commit()
        onEditingFinished: commit()
    }

    component ObjectNumberField: RowLayout {
        property string label: ""
        property string field: ""
        property string valueText: ""
        property string suffix: ""

        spacing: UiStyle.space4

        Text {
            Layout.preferredWidth: 36
            text: parent.label
            color: UiStyle.colorTextMuted
            font.family: UiStyle.fontSans
            font.pixelSize: UiStyle.fontSizeXs
            elide: Text.ElideRight
        }

        UiTextField {
            Layout.fillWidth: true
            Layout.preferredHeight: 22
            property string boundValueText: parent.valueText
            property string lastCommittedText: boundValueText
            text: boundValueText

            onBoundValueTextChanged: {
                text = boundValueText
                lastCommittedText = boundValueText
            }

            function commit() {
                if (text === lastCommittedText) {
                    return
                }
                lastCommittedText = text
                drawingRightContext.setObjectField(parent.field, text)
            }

            onAccepted: commit()
            onEditingFinished: commit()
        }

        Text {
            Layout.preferredWidth: parent.suffix.length > 0 ? 24 : 0
            visible: parent.suffix.length > 0
            text: parent.suffix
            color: UiStyle.colorTextFaint
            font.family: UiStyle.fontSans
            font.pixelSize: UiStyle.fontSizeXs
            elide: Text.ElideRight
        }
    }

    component ObjectMetadataField: RowLayout {
        property string label: ""
        property string field: ""
        property string valueText: ""
        property string placeholder: ""

        spacing: UiStyle.space4

        Text {
            Layout.preferredWidth: 52
            text: parent.label
            color: UiStyle.colorTextMuted
            font.family: UiStyle.fontSans
            font.pixelSize: UiStyle.fontSizeXs
            elide: Text.ElideRight
        }

        UiTextField {
            Layout.fillWidth: true
            Layout.preferredHeight: 22
            placeholderText: parent.placeholder
            property string boundValueText: parent.valueText
            property string lastCommittedText: boundValueText
            text: boundValueText

            onBoundValueTextChanged: {
                text = boundValueText
                lastCommittedText = boundValueText
            }

            function commit() {
                if (text === lastCommittedText) {
                    return
                }
                lastCommittedText = text
                drawingRightContext.setObjectMetadataField(parent.field, text)
            }

            onAccepted: commit()
            onEditingFinished: commit()
        }
    }

    component MetadataPresetRow: RowLayout {
        id: presetRow
        property var rowData: ({})

        spacing: UiStyle.space4

        Text {
            Layout.preferredWidth: 42
            text: String(presetRow.rowData.label || "")
            color: UiStyle.colorTextMuted
            font.family: UiStyle.fontSans
            font.pixelSize: UiStyle.fontSizeXs
            elide: Text.ElideRight
        }

        Repeater {
            model: asArray(presetRow.rowData.options)
            delegate: UiButton {
                Layout.preferredWidth: Math.max(46, Math.min(72, String(modelData || "").length * 8 + 18))
                Layout.preferredHeight: 22
                label: String(modelData || "")
                tooltip: "Set " + String(presetRow.rowData.field || "metadata") + " to " + String(modelData || "")
                selected: drawingRightContext.metadataPresetSelected(presetRow.rowData, modelData)
                onClicked: drawingRightContext.applyMetadataPreset(presetRow.rowData, modelData)
            }
        }
    }

    component ObjectIntentField: ColumnLayout {
        property string valueText: ""

        spacing: UiStyle.space4

        Text {
            Layout.fillWidth: true
            text: "intent"
            color: UiStyle.colorTextMuted
            font.family: UiStyle.fontSans
            font.pixelSize: UiStyle.fontSizeXs
            elide: Text.ElideRight
        }

        UiTextArea {
            Layout.fillWidth: true
            Layout.preferredHeight: 58
            placeholderText: "why this object exists"
            property string boundValueText: parent.valueText
            property string lastCommittedText: boundValueText
            text: boundValueText

            onBoundValueTextChanged: {
                text = boundValueText
                lastCommittedText = boundValueText
            }

            function commit() {
                if (text === lastCommittedText) {
                    return
                }
                lastCommittedText = text
                drawingRightContext.setObjectMetadataField("intent", text)
            }

            onActiveFocusChanged: if (!activeFocus) commit()
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
                Layout.preferredHeight: drawingRightContext.sectionCollapsed("tools") ? 40 : 44 + toolRows().length * 26 + (drawingRightContext.pendingActive() ? 30 : 0)
                panelPadding: UiStyle.space8

                ColumnLayout {
                    anchors.fill: parent
                    spacing: UiStyle.space4

                    SectionHeader {
                        sectionId: "tools"
                        title: "Tools"
                        meta: selectedToolLabel()
                    }

                    Repeater {
                        model: drawingRightContext.sectionCollapsed("tools") ? [] : toolRows()
                        delegate: UiListRow {
                            Layout.fillWidth: true
                            label: modelData.label
                            tooltip: modelData.tooltip
                            selected: selectedToolId() === modelData.id
                            clickable: true
                            onClicked: drawingRightContext.controller.selectDrawingTool(modelData.id)
                        }
                    }

                    UiButton {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 24
                        visible: drawingRightContext.pendingActive() && !drawingRightContext.sectionCollapsed("tools")
                        label: "Cancel"
                        tooltip: "Cancel the active pending shape."
                        onClicked: drawingRightContext.controller.cancelDrawingPendingShape()
                    }
                }
            }

            UiPanel {
                Layout.fillWidth: true
                Layout.preferredHeight: 68 + drawingRightContext.objectEditRows().length * 26
                panelPadding: UiStyle.space8
                visible: drawingRightContext.selectedGeneratedObjectActive()

                ColumnLayout {
                    anchors.fill: parent
                    spacing: UiStyle.space4

                    Text {
                        Layout.fillWidth: true
                        text: drawingRightContext.selectedGeneratedObjectLabel()
                        color: UiStyle.colorText
                        font.family: UiStyle.fontSans
                        font.pixelSize: UiStyle.fontSizeSm
                        font.weight: UiStyle.fontWeightSemiBold
                        elide: Text.ElideRight
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: UiStyle.space4

                        Text {
                            Layout.fillWidth: true
                            text: drawingRightContext.selectedGeneratedObjectMeta()
                            color: UiStyle.colorTextFaint
                            font.family: UiStyle.fontSans
                            font.pixelSize: UiStyle.fontSizeXs
                            elide: Text.ElideRight
                        }

                        UiButton {
                            Layout.preferredWidth: 48
                            Layout.preferredHeight: 22
                            label: "Clear"
                            tooltip: "Clear selected object."
                            onClicked: drawingRightContext.controller.clearDrawingObjectSelection()
                        }

                        UiButton {
                            Layout.preferredWidth: 48
                            Layout.preferredHeight: 22
                            label: "Delete"
                            tooltip: "Delete selected generated object."
                            onClicked: drawingRightContext.controller.deleteSelectedDrawingObject()
                        }
                    }

                    Repeater {
                        model: drawingRightContext.objectEditRows()
                        delegate: ObjectNumberField {
                            Layout.fillWidth: true
                            label: modelData.label
                            field: modelData.field
                            valueText: modelData.value
                            suffix: modelData.suffix
                        }
                    }
                }
            }

            UiPanel {
                Layout.fillWidth: true
                Layout.preferredHeight: 302
                panelPadding: UiStyle.space8
                visible: drawingRightContext.selectedGeneratedObjectActive()

                ColumnLayout {
                    anchors.fill: parent
                    spacing: UiStyle.space4

                    Text {
                        Layout.fillWidth: true
                        text: "Metadata"
                        color: UiStyle.colorText
                        font.family: UiStyle.fontSans
                        font.pixelSize: UiStyle.fontSizeSm
                        font.weight: UiStyle.fontWeightSemiBold
                        elide: Text.ElideRight
                    }

                    Repeater {
                        model: metadataPresetRows()
                        delegate: MetadataPresetRow {
                            Layout.fillWidth: true
                            rowData: modelData
                        }
                    }

                    Repeater {
                        model: metadataRows()
                        delegate: ObjectMetadataField {
                            Layout.fillWidth: true
                            label: modelData.label
                            field: modelData.field
                            valueText: modelData.value
                            placeholder: modelData.placeholder
                        }
                    }

                    ObjectIntentField {
                        Layout.fillWidth: true
                        valueText: metadataFieldValue("intent")
                    }
                }
            }

            UiPanel {
                Layout.fillWidth: true
                Layout.preferredHeight: drawingRightContext.sectionCollapsed("variants") ? 40 : 74
                panelPadding: UiStyle.space8
                visible: variantRows().length > 0

                ColumnLayout {
                    anchors.fill: parent
                    spacing: UiStyle.space6

                    SectionHeader {
                        sectionId: "variants"
                        title: "Variant"
                        meta: activeVariantText()
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: UiStyle.space4
                        visible: !drawingRightContext.sectionCollapsed("variants") && variantRows().length > 0

                        Repeater {
                            model: variantRows()
                            delegate: UiButton {
                                Layout.preferredWidth: Math.max(58, implicitWidth)
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

            UiPanel {
                Layout.fillWidth: true
                Layout.preferredHeight: drawingRightContext.sectionCollapsed("params") ? 40 : 40 + visibleOptionRows().length * 30
                panelPadding: UiStyle.space8
                visible: visibleOptionRows().length > 0

                ColumnLayout {
                    anchors.fill: parent
                    spacing: UiStyle.space6

                    SectionHeader {
                        sectionId: "params"
                        title: "Params"
                    }

                    Repeater {
                        model: drawingRightContext.sectionCollapsed("params") ? [] : visibleOptionRows()
                        delegate: OptionField {
                            Layout.fillWidth: true
                            label: modelData.label
                            field: modelData.field
                            valueText: modelData.value
                            suffix: modelData.suffix
                        }
                    }
                }
            }

            UiPanel {
                Layout.fillWidth: true
                Layout.preferredHeight: drawingRightContext.sectionCollapsed("format") ? 40 : 168
                panelPadding: UiStyle.space8

                ColumnLayout {
                    anchors.fill: parent
                    spacing: UiStyle.space6

                    SectionHeader {
                        sectionId: "format"
                        title: "Format"
                        meta: styleLineStyle()
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        visible: !drawingRightContext.sectionCollapsed("format")
                        spacing: UiStyle.space6

                        ColorField {
                            Layout.fillWidth: true
                            fieldId: "stroke"
                            valueText: String(drawingRightContext.controller ? (drawingRightContext.controller.drawingStrokeColor || "") : "")
                            fallbackColor: UiStyle.colorAccent
                        }

                        ColorField {
                            Layout.fillWidth: true
                            fieldId: "fill"
                            valueText: String(drawingRightContext.controller ? (drawingRightContext.controller.drawingFillColor || "") : "")
                            fallbackColor: UiStyle.colorControl
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: UiStyle.space6

                            StyleNumberField {
                                fieldId: "width"
                                valueText: String(Number(drawingRightContext.controller ? drawingRightContext.controller.drawingLineThickness : 2))
                            }

                            StyleNumberField {
                                fieldId: "opacity"
                                valueText: String(Number(drawingRightContext.controller ? drawingRightContext.controller.drawingStrokeOpacity : 1))
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: UiStyle.space4

                            UiButton {
                                Layout.preferredWidth: 48
                                Layout.preferredHeight: 24
                                label: "Line"
                                selected: drawingRightContext.styleLineStyle() === "solid"
                                onClicked: if (drawingRightContext.controller) drawingRightContext.controller.setDrawingLineStyle("solid")
                            }

                            UiButton {
                                Layout.preferredWidth: 48
                                Layout.preferredHeight: 24
                                label: "Dash"
                                selected: drawingRightContext.styleLineStyle() === "dashed"
                                onClicked: if (drawingRightContext.controller) drawingRightContext.controller.setDrawingLineStyle("dashed")
                            }

                            UiButton {
                                Layout.preferredWidth: 44
                                Layout.preferredHeight: 24
                                label: "Dot"
                                selected: drawingRightContext.styleLineStyle() === "dotted"
                                onClicked: if (drawingRightContext.controller) drawingRightContext.controller.setDrawingLineStyle("dotted")
                            }
                        }
                    }
                }
            }

            UiPanel {
                Layout.fillWidth: true
                Layout.preferredHeight: drawingRightContext.sectionCollapsed("precision") ? 40 : 142
                panelPadding: UiStyle.space8

                ColumnLayout {
                    anchors.fill: parent
                    spacing: UiStyle.space6

                    SectionHeader {
                        sectionId: "precision"
                        title: "Precision"
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: UiStyle.space6
                        visible: !drawingRightContext.sectionCollapsed("precision")

                        UiToggle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 24
                            label: "Snap"
                            checked: drawingRightContext.snapEnabled()
                            onToggled: if (drawingRightContext.controller) drawingRightContext.controller.setDrawingSnapGrid(checked)
                        }

                        UiToggle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 24
                            label: "Grid"
                            checked: drawingRightContext.gridVisible()
                            onToggled: if (drawingRightContext.controller) drawingRightContext.controller.setDrawingGridVisible(checked)
                        }

                        UiToggle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 24
                            label: "Obj"
                            checked: drawingRightContext.objectSnapEnabled()
                            onToggled: if (drawingRightContext.controller) drawingRightContext.controller.setDrawingObjectSnapEnabled(checked)
                        }
                    }
                }
            }
        }
    }
}
