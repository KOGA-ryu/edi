import QtQuick
import QtQuick.Layouts
import "../style"
import "../components"
import "../features/blank"
import "../features/ui_taxonomy"
import "../features/settings"
import "../features/csv_map_editor"

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
        visible: rightPanel.controller && rightPanel.controller.activityMode === "review"
        controller: rightPanel.controller
    }

    SettingsRightContext {
        anchors.fill: parent
        anchors.margins: UiStyle.space6
        visible: rightPanel.controller && rightPanel.controller.activityMode === "settings"
        controller: rightPanel.controller
    }

    CsvMapEditorRightContext {
        anchors.fill: parent
        anchors.margins: UiStyle.space6
        visible: rightPanel.controller && rightPanel.controller.activityMode === "map_editor"
        controller: rightPanel.controller
    }

    BlankPanel {
        anchors.fill: parent
        anchors.margins: UiStyle.space6
        visible: rightPanel.controller
            && rightPanel.controller.activityMode !== "review"
            && rightPanel.controller.activityMode !== "settings"
            && rightPanel.controller.activityMode !== "map_editor"
        controller: rightPanel.controller
    }
}
