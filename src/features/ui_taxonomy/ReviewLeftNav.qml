import QtQuick
import QtQuick.Layouts
import "../../style"
import "../../components"

ColumnLayout {
    id: root

    property var controller: null
    spacing: UiStyle.space8

    UiSectionHeader { title: "Project"; Layout.fillWidth: true }
    UiListRow { label: "Project slot"; meta: "blank"; Layout.fillWidth: true }
    UiListRow { label: "Scratch"; meta: "workflow"; Layout.fillWidth: true }
    UiListRow { label: "Final"; meta: "workflow"; Layout.fillWidth: true }

    UiSectionHeader { title: "Review Subject"; Layout.fillWidth: true }
    UiListRow {
        label: "Draftsman UI Taxonomy"
        meta: root.controller ? root.controller.routeStatus(root.controller.rootRouteId) : "pending"
        selected: root.controller && root.controller.selectedRouteId === root.controller.rootRouteId
        clickable: true
        Layout.fillWidth: true
        onClicked: root.controller.selectRoute(root.controller.rootRouteId)
    }

    UiSectionHeader { title: "UI Regions"; Layout.fillWidth: true }
    Repeater {
        model: root.controller ? root.controller.childRoutes(root.controller.rootRouteId, root.controller.revision) : []
        delegate: UiListRow {
            label: modelData.label
            meta: root.controller.routeStatus(modelData.id)
            selected: root.controller.selectedRouteId === modelData.id
            clickable: true
            Layout.fillWidth: true
            onClicked: root.controller.selectRoute(modelData.id)
        }
    }

    UiSectionHeader { title: "Nearby Routes"; Layout.fillWidth: true }
    Repeater {
        model: root.controller ? root.controller.siblingRoutes(root.controller.selectedRouteId) : []
        delegate: UiListRow {
            label: modelData.label
            meta: modelData.type
            selected: root.controller.selectedRouteId === modelData.id
            clickable: true
            indent: UiStyle.space8
            Layout.fillWidth: true
            onClicked: root.controller.selectRoute(modelData.id)
        }
    }

    UiSectionHeader { title: "Settings"; Layout.fillWidth: true }
    UiListRow {
        label: "Theme and layout"
        meta: "settings"
        clickable: true
        Layout.fillWidth: true
        onClicked: root.controller.setActivityMode("settings")
    }

    Item { Layout.fillHeight: true }
}
