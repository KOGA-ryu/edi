import QtQuick
import QtQuick.Layouts
import "../style"
import "../features/ui_taxonomy"

Rectangle {
    id: rightPanel

    property string dataUi: "right_panel"
    property string dataState: "secondary"
    property string placementRole: "resizable_right_panel"
    property string surfaceRecipeId: "right_panel_surface"
    property var controller: null

    color: UiStyle.colorPanel
    border.width: UiStyle.borderThin
    border.color: UiStyle.colorBorderMajor

    UiTaxonomyInspector {
        anchors.fill: parent
        anchors.margins: UiStyle.space10
        controller: rightPanel.controller
    }
}

