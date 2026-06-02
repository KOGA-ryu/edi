import QtQuick
import QtQuick.Layouts
import "../style"
import "../features/blank"
import "../features/ui_taxonomy"
import "../features/settings"

Rectangle {
    id: workspace

    property string dataUi: "main_workspace"
    property string dataState: "focused"
    property string placementRole: "dominant_center"
    property string surfaceRecipeId: "main_workspace_surface"
    property var controller: null

    color: UiStyle.colorWorkspace
    border.width: UiStyle.borderNone

    UiTaxonomyWorkspace {
        anchors.fill: parent
        visible: workspace.controller && workspace.controller.activityMode === "review"
        controller: workspace.controller
    }

    ThemeSettingsWorkspace {
        anchors.fill: parent
        visible: workspace.controller && workspace.controller.activityMode === "settings"
        controller: workspace.controller
    }

    BlankWorkspace {
        anchors.fill: parent
        visible: !workspace.controller
            || (workspace.controller.activityMode !== "review"
                && workspace.controller.activityMode !== "settings")
        controller: workspace.controller
    }
}
