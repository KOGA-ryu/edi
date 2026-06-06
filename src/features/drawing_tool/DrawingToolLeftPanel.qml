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

    function sectionCollapsed(sectionId) {
        return collapsedSections[String(sectionId)] === true
    }

    function toggleSection(sectionId) {
        var next = Object.assign({}, collapsedSections)
        next[String(sectionId)] = !sectionCollapsed(sectionId)
        collapsedSections = next
    }

    ColumnLayout {
        id: content
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        spacing: UiStyle.space6

        Repeater {
            model: drawingLeftPanel.controller ? drawingLeftPanel.controller.drawingSidebarSections : []

            delegate: ColumnLayout {
                id: sectionGroup
                property var section: modelData
                property string sectionId: modelData.id
                property var rows: drawingLeftPanel.controller ? drawingLeftPanel.controller.drawingSidebarRows(section, drawingLeftPanel.controller.revision) : []
                Layout.fillWidth: true
                spacing: UiStyle.space2

                UiListRow {
                    Layout.fillWidth: true
                    label: (drawingLeftPanel.sectionCollapsed(sectionGroup.sectionId) ? "▸ " : "▾ ") + modelData.title
                    meta: String(sectionGroup.rows.length) + " " + modelData.hint
                    selected: false
                    clickable: true
                    metaMaxWidth: 84
                    onClicked: drawingLeftPanel.toggleSection(sectionGroup.sectionId)
                }

                Repeater {
                    model: drawingLeftPanel.sectionCollapsed(sectionGroup.sectionId) ? [] : sectionGroup.rows
                    delegate: UiListRow {
                        Layout.fillWidth: true
                        label: modelData.label
                        meta: modelData.meta
                        indent: UiStyle.space10
                        selected: drawingLeftPanel.controller ? drawingLeftPanel.controller.drawingSidebarRowSelected(sectionGroup.section, modelData, drawingLeftPanel.controller.revision) : false
                        clickable: drawingLeftPanel.controller ? drawingLeftPanel.controller.drawingSidebarRowClickable(sectionGroup.section, drawingLeftPanel.controller.revision) : false
                        onClicked: if (drawingLeftPanel.controller) drawingLeftPanel.controller.drawingSidebarRowClicked(sectionGroup.section, modelData)
                    }
                }
            }
        }
    }
}
