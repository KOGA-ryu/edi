import QtQuick
import QtQuick.Layouts
import "../style"
import "../components"

Rectangle {
    id: leftPanel

    property string dataUi: "left_panel"
    property string dataState: "secondary"
    property string placementRole: "resizable_left_panel"
    property string surfaceRecipeId: "left_panel_surface"
    property var controller: null

    color: UiStyle.colorPanel
    border.width: UiStyle.borderThin
    border.color: UiStyle.colorBorderMajor

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: UiStyle.space10
        spacing: UiStyle.space8

        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: UiStyle.toolbarHeight
            spacing: UiStyle.space8

            Text {
                Layout.fillWidth: true
                text: "Draftsman"
                color: UiStyle.colorText
                font.family: UiStyle.fontSans
                font.pixelSize: UiStyle.fontSizeTitle
                font.weight: UiStyle.fontWeightSemiBold
                elide: Text.ElideRight
            }

            UiButton { label: "New"; implicitWidth: 54; enabled: false }
        }

        UiSectionHeader { title: "Project"; Layout.fillWidth: true }
        UiListRow { label: "Project slot"; meta: "blank"; Layout.fillWidth: true }
        UiListRow { label: "Scratch"; meta: "workflow"; Layout.fillWidth: true }
        UiListRow { label: "Final"; meta: "workflow"; Layout.fillWidth: true }

        UiSectionHeader { title: "Review Subject"; Layout.fillWidth: true }
        UiListRow {
            label: "Draftsman UI Taxonomy"
            meta: leftPanel.controller ? leftPanel.controller.routeStatus("draftsman_ui") : "pending"
            selected: leftPanel.controller && leftPanel.controller.selectedRouteId === "draftsman_ui"
            clickable: true
            Layout.fillWidth: true
            onClicked: leftPanel.controller.selectRoute("draftsman_ui")
        }

        UiSectionHeader { title: "UI Regions"; Layout.fillWidth: true }
        Repeater {
            model: leftPanel.controller ? leftPanel.controller.childRoutes("draftsman_ui", leftPanel.controller.revision) : []
            delegate: UiListRow {
                label: modelData.label
                meta: leftPanel.controller.routeStatus(modelData.id)
                selected: leftPanel.controller.selectedRouteId === modelData.id
                clickable: true
                Layout.fillWidth: true
                onClicked: leftPanel.controller.selectRoute(modelData.id)
            }
        }

        UiSectionHeader { title: "Nearby Routes"; Layout.fillWidth: true }
        Repeater {
            model: leftPanel.controller ? leftPanel.controller.siblingRoutes(leftPanel.controller.selectedRouteId) : []
            delegate: UiListRow {
                label: modelData.label
                meta: modelData.type
                selected: leftPanel.controller.selectedRouteId === modelData.id
                clickable: true
                indent: UiStyle.space8
                Layout.fillWidth: true
                onClicked: leftPanel.controller.selectRoute(modelData.id)
            }
        }

        UiSectionHeader { title: "Settings"; Layout.fillWidth: true }
        UiListRow {
            label: "Theme and layout"
            meta: "reserved"
            clickable: true
            Layout.fillWidth: true
            onClicked: leftPanel.controller.selectRoute("settings")
        }

        Item { Layout.fillHeight: true }
    }
}
