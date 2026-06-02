import QtQuick
import QtQuick.Layouts
import "../style"
import "../components"
import "../features/ui_taxonomy"
import "../features/settings"

Rectangle {
    id: rightPanel

    property string dataUi: "right_panel"
    property string dataState: "secondary"
    property string placementRole: "resizable_right_panel"
    property string surfaceRecipeId: "right_panel_surface"
    property var controller: null

    color: UiStyle.colorPanelAlt
    border.width: UiStyle.borderNone

    ReviewRightContext {
        anchors.fill: parent
        anchors.margins: UiStyle.space6
        visible: !rightPanel.controller || rightPanel.controller.activityMode === "review"
        controller: rightPanel.controller
    }

    SettingsRightContext {
        anchors.fill: parent
        anchors.margins: UiStyle.space6
        visible: rightPanel.controller && rightPanel.controller.activityMode === "settings"
        controller: rightPanel.controller
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: UiStyle.space6
        visible: rightPanel.controller && rightPanel.controller.activityMode !== "review" && rightPanel.controller.activityMode !== "settings"
        spacing: UiStyle.space4

        UiSectionHeader { title: "Context"; Layout.fillWidth: true }
        UiListRow {
            Layout.fillWidth: true
            label: rightPanel.controller ? rightPanel.controller.activityMode : "binder"
            meta: "reserved"
        }
        Item { Layout.fillHeight: true }
    }
}
