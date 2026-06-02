import QtQuick
import QtQuick.Layouts
import "../../style"
import "../../components"

ColumnLayout {
    id: root

    property var controller: null
    spacing: UiStyle.space8

    readonly property var pages: [
        { id: "theme", label: "Theme", meta: "colors/fonts" },
        { id: "layout", label: "Layout", meta: "reserved" },
        { id: "panels", label: "Panels", meta: "resize/save" },
        { id: "features", label: "Features", meta: "reserved" },
        { id: "subjects", label: "Review Subjects", meta: "reserved" },
        { id: "write_rules", label: "Write Rules", meta: "disabled" }
    ]

    UiSectionHeader { title: "Settings"; Layout.fillWidth: true }
    Repeater {
        model: root.pages
        delegate: UiListRow {
            Layout.fillWidth: true
            label: modelData.label
            meta: modelData.meta
            clickable: true
            selected: root.controller && root.controller.selectedSettingsPage === modelData.id
            onClicked: root.controller.setSettingsPage(modelData.id)
        }
    }

    UiSectionHeader { title: "Current Context"; Layout.fillWidth: true }
    UiListRow {
        Layout.fillWidth: true
        label: "Subject"
        meta: root.controller ? root.controller.selectedSubjectLabel : ""
    }
    UiListRow {
        Layout.fillWidth: true
        label: "Route"
        meta: root.controller ? root.controller.currentRoute().label : ""
    }
    UiListRow {
        Layout.fillWidth: true
        label: "Writes"
        meta: root.controller && root.controller.writeDisabled ? "disabled" : "enabled"
    }

    UiSectionHeader { title: "Return"; Layout.fillWidth: true }
    UiListRow {
        Layout.fillWidth: true
        label: "Review workspace"
        meta: "routes"
        clickable: true
        onClicked: root.controller.setActivityMode("review")
    }

    Item { Layout.fillHeight: true }
}
