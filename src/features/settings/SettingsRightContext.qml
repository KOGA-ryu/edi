import QtQuick
import QtQuick.Layouts
import "../../style"
import "../../components"

ColumnLayout {
    id: root

    property var controller: null
    spacing: UiStyle.space8

    function currentPageId() {
        return root.controller ? root.controller.selectedSettingsPage : "theme"
    }

    function pageLabel(pageId) {
        if (pageId === "layout") return "Layout"
        if (pageId === "panels") return "Panels"
        if (pageId === "features") return "Features"
        if (pageId === "subjects") return "Review Subjects"
        if (pageId === "write_rules") return "Write Rules"
        return "Theme"
    }

    function pageStatus(pageId) {
        if (pageId === "theme") return "preview only"
        if (pageId === "panels") return root.controller && root.controller.shellLayoutDirty ? "unsaved" : "saved"
        if (pageId === "write_rules") return "disabled"
        return "reserved"
    }

    function pageSource(pageId) {
        if (pageId === "theme") return root.controller ? root.controller.uiThemePath : ""
        if (pageId === "panels") return root.controller ? root.controller.shellLayoutPath : ""
        if (pageId === "subjects") return root.controller ? root.controller.selectedSubjectId : ""
        return ""
    }

    function pageContract(pageId) {
        if (pageId === "theme") return "data/ui_theme.json"
        if (pageId === "layout" || pageId === "panels") return "data/shell_layout.json"
        if (pageId === "features") return "feature registry"
        if (pageId === "subjects") return "data/review_subjects/*.json"
        if (pageId === "write_rules") return "write policy"
        return "settings"
    }

    function pagePersistence(pageId) {
        if (pageId === "theme") return "disabled"
        if (pageId === "panels") return root.controller && root.controller.shellLayoutDirty ? "pending save" : "saved"
        if (pageId === "write_rules") return "disabled"
        return "reserved"
    }

    UiSectionHeader { title: "Selected Page"; Layout.fillWidth: true }
    UiListRow {
        Layout.fillWidth: true
        label: "Page"
        meta: root.pageLabel(root.currentPageId())
    }
    UiListRow {
        Layout.fillWidth: true
        label: "Id"
        meta: root.currentPageId()
    }
    UiListRow {
        Layout.fillWidth: true
        label: "Status"
        meta: root.pageStatus(root.currentPageId())
    }
    UiListRow {
        Layout.fillWidth: true
        label: "Writes"
        meta: root.controller && root.controller.writeDisabled ? "disabled" : "enabled"
    }

    UiSectionHeader { title: "Source"; Layout.fillWidth: true }
    UiCodeRefRow {
        Layout.fillWidth: true
        path: root.pageSource(root.currentPageId()) || root.pageContract(root.currentPageId())
        role: root.pageSource(root.currentPageId()) ? "file" : "contract"
    }
    UiListRow {
        Layout.fillWidth: true
        label: "Persistence"
        meta: root.pagePersistence(root.currentPageId())
    }

    UiSectionHeader {
        visible: root.currentPageId() === "panels"
        title: "Panel State"
        Layout.fillWidth: true
    }
    UiListRow {
        visible: root.currentPageId() === "panels"
        Layout.fillWidth: true
        label: "Left"
        meta: root.controller ? root.controller.panelStateLabel("left") : ""
    }
    UiListRow {
        visible: root.currentPageId() === "panels"
        Layout.fillWidth: true
        label: "Right"
        meta: root.controller ? root.controller.panelStateLabel("right") : ""
    }
    UiListRow {
        visible: root.currentPageId() === "panels"
        Layout.fillWidth: true
        label: "Bottom"
        meta: root.controller ? root.controller.panelStateLabel("bottom") : ""
    }

    UiSectionHeader { title: "Review Context"; Layout.fillWidth: true }
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

    Item { Layout.fillHeight: true }
}
