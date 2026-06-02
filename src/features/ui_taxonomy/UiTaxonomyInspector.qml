import QtQuick
import QtQuick.Layouts
import "../../style"
import "../../components"

ColumnLayout {
    id: root
    property var controller: null
    property var route: controller ? controller.currentRoute() : ({})
    spacing: UiStyle.space4

    UiSectionHeader { title: "Route Facts"; Layout.fillWidth: true }
    UiListRow { label: "Route"; meta: root.route.label || ""; metaMaxWidth: 150; Layout.fillWidth: true }
    UiListRow { label: "Type"; meta: root.route.type || ""; metaMaxWidth: 110; Layout.fillWidth: true }
    UiListRow { label: "Status"; meta: root.controller ? root.controller.routeStatus(root.route.id) : "pending"; metaMaxWidth: 110; Layout.fillWidth: true }
    UiListRow { label: "Children"; meta: String((root.route.children || []).length); metaMaxWidth: 60; Layout.fillWidth: true }
    UiListRow { label: "Notes"; meta: root.controller ? String(root.controller.noteCount(root.route.id)) : "0"; metaMaxWidth: 60; Layout.fillWidth: true }

    UiSectionHeader { title: "Code Refs"; Layout.fillWidth: true }
    Repeater {
        model: root.route.codeRefs || []
        delegate: UiCodeRefRow {
            Layout.fillWidth: true
            path: modelData
            role: "source"
        }
    }

    UiSectionHeader { title: "Recent Notes"; Layout.fillWidth: true }
    Repeater {
        model: root.controller ? root.controller.routeNotes(root.route.id, root.controller.revision).slice(-2) : []
        delegate: UiNoteCard {
            Layout.fillWidth: true
            status: modelData.status
            body: modelData.body
            createdAt: modelData.createdAt
        }
    }

    UiSectionHeader { title: "Receipt"; Layout.fillWidth: true }
    UiListRow { label: "Writes"; meta: root.controller && root.controller.writeDisabled ? "disabled" : "enabled"; metaMaxWidth: 90; Layout.fillWidth: true }
    UiListRow { label: "Storage"; meta: "in memory"; metaMaxWidth: 100; Layout.fillWidth: true }
    UiListRow { label: "Subject"; meta: root.controller ? root.controller.selectedSubjectId : ""; metaMaxWidth: 150; Layout.fillWidth: true }

    Item { Layout.fillHeight: true }
}
