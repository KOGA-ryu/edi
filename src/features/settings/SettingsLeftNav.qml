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
        { id: "layout", label: "Layout", meta: "planned" },
        { id: "panels", label: "Panels", meta: "resize/save" },
        { id: "features", label: "Features", meta: "planned" },
        { id: "subjects", label: "Review Subjects", meta: "planned" },
        { id: "write_rules", label: "Write Rules", meta: "disabled" }
    ]

    function reviewAvailable() {
        return root.controller && root.controller.hasActivityMode("review")
    }

    function pageVisible(pageId) {
        return pageId !== "subjects" || reviewAvailable()
    }

    UiSectionHeader { title: "Settings"; Layout.fillWidth: true }
    Repeater {
        model: root.pages
        delegate: UiListRow {
            visible: root.pageVisible(modelData.id)
            Layout.fillWidth: true
            Layout.preferredHeight: visible ? implicitHeight : 0
            label: modelData.label
            meta: modelData.meta
            clickable: true
            selected: root.controller && root.controller.selectedSettingsPage === modelData.id
            onClicked: root.controller.setSettingsPage(modelData.id)
        }
    }

    UiSectionHeader { title: "Current Context"; visible: root.reviewAvailable(); Layout.fillWidth: true }
    UiListRow {
        visible: root.reviewAvailable()
        Layout.fillWidth: true
        Layout.preferredHeight: visible ? implicitHeight : 0
        label: "Subject"
        meta: root.controller ? root.controller.selectedSubjectLabel : ""
    }
    UiListRow {
        visible: root.reviewAvailable()
        Layout.fillWidth: true
        Layout.preferredHeight: visible ? implicitHeight : 0
        label: "Route"
        meta: root.controller ? root.controller.currentRoute().label : ""
    }
    UiListRow {
        visible: root.reviewAvailable()
        Layout.fillWidth: true
        Layout.preferredHeight: visible ? implicitHeight : 0
        label: "Writes"
        meta: root.controller && root.controller.writeDisabled ? "disabled" : "enabled"
    }

    UiSectionHeader { title: "Return"; visible: root.reviewAvailable(); Layout.fillWidth: true }
    UiListRow {
        visible: root.reviewAvailable()
        Layout.fillWidth: true
        Layout.preferredHeight: visible ? implicitHeight : 0
        label: "Review workspace"
        meta: "routes"
        clickable: true
        onClicked: root.controller.setActivityMode("review")
    }

    Item { Layout.fillHeight: true }
}
