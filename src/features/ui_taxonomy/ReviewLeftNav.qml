import QtQuick
import QtQuick.Layouts
import "../../style"
import "../../components"

ColumnLayout {
    id: root

    property var controller: null
    spacing: UiStyle.space4

    UiSectionHeader { title: "Project"; Layout.fillWidth: true }
    Repeater {
        model: root.controller ? root.controller.leftProjectRows : []
        delegate: UiListRow {
            label: modelData.label
            meta: modelData.meta
            hideMetaBelowWidth: 220
            Layout.fillWidth: true
        }
    }

    UiSectionHeader { title: "Review Subject"; Layout.fillWidth: true }
    UiListRow {
        label: root.controller ? root.controller.selectedSubjectLabel : "Review Subject"
        meta: root.controller ? root.controller.routeStatus(root.controller.rootRouteId) : "pending"
        hideMetaBelowWidth: 220
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
            hideMetaBelowWidth: 220
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
            hideMetaBelowWidth: 220
            selected: root.controller.selectedRouteId === modelData.id
            clickable: true
            indent: UiStyle.space8
            Layout.fillWidth: true
            onClicked: root.controller.selectRoute(modelData.id)
        }
    }

    UiSectionHeader { title: "Settings"; Layout.fillWidth: true }
    UiListRow {
        label: root.controller ? root.controller.settingsNavLabel : "Theme and layout"
        meta: "settings"
        hideMetaBelowWidth: 220
        clickable: true
        Layout.fillWidth: true
        onClicked: root.controller.setActivityMode("settings")
    }

    Item { Layout.fillHeight: true }
}
