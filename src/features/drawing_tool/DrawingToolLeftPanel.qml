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

    function sectionRows(sectionId) {
        if (sectionId === "tools") {
            return toolRows()
        }
        return []
    }

    function isSelected(sectionId, row) {
        if (!drawingLeftPanel.controller || !row) {
            return false
        }
        if (sectionId === "tools") {
            return String(drawingLeftPanel.controller.selectedDrawingToolId || "") === String(row.id || "")
        }
        return false
    }

    function handleRowClicked(sectionId, row) {
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
        spacing: UiStyle.space6

        Repeater {
            model: [
                { id: "tools", title: "Tools", hint: "drawing" }
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
                        clickable: String(modelData.action || "").length > 0
                        onClicked: handleRowClicked(section.id, modelData)
                    }
                }
            }
        }
    }
}
