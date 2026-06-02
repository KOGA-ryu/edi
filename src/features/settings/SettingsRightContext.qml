import QtQuick
import QtQuick.Layouts
import "../../style"
import "../../components"

ColumnLayout {
    id: root

    property var controller: null
    spacing: UiStyle.space4

    function currentPageId() {
        return root.controller ? root.controller.selectedSettingsPage : "theme"
    }

    function reviewAvailable() {
        return root.controller && root.controller.hasActivityMode("review")
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
        return "planned"
    }

    function pageSource(pageId) {
        if (pageId === "theme") return root.controller ? root.controller.uiThemePath : ""
        if (pageId === "panels") return root.controller ? root.controller.shellLayoutPath : ""
        if (pageId === "subjects" && root.reviewAvailable()) return root.controller ? root.controller.selectedSubjectId : ""
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
        return "planned"
    }

    UiListRow {
        Layout.fillWidth: true
        Layout.preferredHeight: 22
        label: "Page"
        meta: root.pageLabel(root.currentPageId())
        metaMaxWidth: Math.max(64, Math.min(220, width * 0.56))
    }
    UiListRow {
        Layout.fillWidth: true
        Layout.preferredHeight: 22
        label: "Id"
        meta: root.currentPageId()
        metaMaxWidth: Math.max(64, Math.min(220, width * 0.56))
    }
    UiListRow {
        Layout.fillWidth: true
        Layout.preferredHeight: 22
        label: "Status"
        meta: root.pageStatus(root.currentPageId())
        metaMaxWidth: Math.max(64, Math.min(220, width * 0.56))
    }
    UiListRow {
        Layout.fillWidth: true
        Layout.preferredHeight: 22
        label: "Writes"
        meta: root.controller && root.controller.writeDisabled ? "disabled" : "enabled"
        metaMaxWidth: Math.max(64, Math.min(180, width * 0.48))
    }

    UiCodeRefRow {
        Layout.fillWidth: true
        path: root.pageSource(root.currentPageId()) || root.pageContract(root.currentPageId())
        role: root.pageSource(root.currentPageId()) ? "file" : "contract"
    }
    UiListRow {
        Layout.fillWidth: true
        Layout.preferredHeight: 22
        label: "Persistence"
        meta: root.pagePersistence(root.currentPageId())
        metaMaxWidth: Math.max(64, Math.min(220, width * 0.56))
    }
    UiListRow {
        visible: root.currentPageId() === "panels"
        Layout.fillWidth: true
        Layout.preferredHeight: 22
        label: "Left"
        meta: root.controller ? root.controller.panelStateLabel("left") : ""
        metaMaxWidth: Math.max(64, Math.min(220, width * 0.56))
    }
    UiListRow {
        visible: root.currentPageId() === "panels"
        Layout.fillWidth: true
        Layout.preferredHeight: 22
        label: "Right"
        meta: root.controller ? root.controller.panelStateLabel("right") : ""
        metaMaxWidth: Math.max(64, Math.min(220, width * 0.56))
    }
    UiListRow {
        visible: root.currentPageId() === "panels"
        Layout.fillWidth: true
        Layout.preferredHeight: 22
        label: "Bottom"
        meta: root.controller ? root.controller.panelStateLabel("bottom") : ""
        metaMaxWidth: Math.max(64, Math.min(220, width * 0.56))
    }

    UiListRow {
        visible: root.reviewAvailable()
        Layout.fillWidth: true
        Layout.preferredHeight: visible ? 22 : 0
        label: "Subject"
        meta: root.controller ? root.controller.selectedSubjectLabel : ""
        metaMaxWidth: Math.max(64, Math.min(220, width * 0.56))
    }
    UiListRow {
        visible: root.reviewAvailable()
        Layout.fillWidth: true
        Layout.preferredHeight: visible ? 22 : 0
        label: "Route"
        meta: root.controller ? root.controller.currentRoute().label : ""
        metaMaxWidth: Math.max(64, Math.min(220, width * 0.56))
    }

    Item { Layout.fillHeight: true }
}
