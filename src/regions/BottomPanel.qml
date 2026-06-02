import QtQuick
import QtQuick.Layouts
import "../style"
import "../components"

Rectangle {
    id: bottomPanel

    property string dataUi: "bottom_panel"
    property string dataState: "secondary"
    property string placementRole: "docked_bottom_panel"
    property string surfaceRecipeId: "bottom_panel_surface"
    property var controller: null

    color: UiStyle.colorBottomPanel
    border.width: UiStyle.borderNone

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 32
            visible: bottomPanel.controller && bottomPanel.controller.shelfTabs.length > 0
            color: UiStyle.colorPanelAlt
            border.width: UiStyle.borderNone

            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: 1
                color: UiStyle.colorPanelRaised
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: UiStyle.space8
                anchors.rightMargin: UiStyle.space8
                spacing: UiStyle.space2

                Repeater {
                    model: bottomPanel.controller ? bottomPanel.controller.shelfTabs : []
                    delegate: UiTab {
                        label: modelData
                        active: bottomPanel.controller.selectedShelfTab === modelData
                        clickable: true
                        onClicked: bottomPanel.controller.setShelfTab(modelData)
                    }
                }
                Item { Layout.fillWidth: true }
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: UiStyle.space6
            visible: bottomPanel.controller && bottomPanel.controller.activityMode === "review"
            spacing: UiStyle.space0

            UiListRow {
                label: "Selected route"
                meta: bottomPanel.controller ? bottomPanel.controller.currentRoute().label : ""
                metaMaxWidth: 360
                Layout.fillWidth: true
                Layout.preferredHeight: 20
            }
            UiListRow {
                label: "Breadcrumb"
                meta: bottomPanel.controller ? bottomPanel.controller.breadcrumbText(bottomPanel.controller.selectedRouteId) : ""
                metaMaxWidth: 420
                Layout.fillWidth: true
                Layout.preferredHeight: 20
            }
            UiListRow {
                label: "Notes in memory"
                meta: bottomPanel.controller ? String(bottomPanel.controller.allNotes(bottomPanel.controller.revision).length) : "0"
                metaMaxWidth: 120
                Layout.fillWidth: true
                Layout.preferredHeight: 20
            }
            UiListRow {
                label: "Persistence"
                meta: bottomPanel.controller && bottomPanel.controller.writeDisabled ? "disabled" : "enabled"
                metaMaxWidth: 160
                Layout.fillWidth: true
                Layout.preferredHeight: 20
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: !bottomPanel.controller || bottomPanel.controller.activityMode !== "review"
        }
    }
}
