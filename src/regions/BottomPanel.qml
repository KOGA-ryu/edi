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
            Layout.preferredHeight: UiStyle.toolbarHeight
            color: UiStyle.colorPanelAlt
            border.width: UiStyle.borderNone

            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: 1
                color: UiStyle.colorBorderMinor
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: UiStyle.space10
                anchors.rightMargin: UiStyle.space10
                spacing: UiStyle.space6

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
            Layout.margins: UiStyle.space10
            spacing: UiStyle.space4

            UiListRow {
                label: "Selected route"
                meta: bottomPanel.controller ? bottomPanel.controller.currentRoute().label : ""
                Layout.fillWidth: true
            }
            UiListRow {
                label: "Breadcrumb"
                meta: bottomPanel.controller ? bottomPanel.controller.breadcrumbText(bottomPanel.controller.selectedRouteId) : ""
                Layout.fillWidth: true
            }
            UiListRow {
                label: "Notes in memory"
                meta: bottomPanel.controller ? String(bottomPanel.controller.allNotes(bottomPanel.controller.revision).length) : "0"
                Layout.fillWidth: true
            }
            UiListRow {
                label: "Persistence"
                meta: bottomPanel.controller && bottomPanel.controller.writeDisabled ? "disabled" : "enabled"
                Layout.fillWidth: true
            }
        }
    }
}
