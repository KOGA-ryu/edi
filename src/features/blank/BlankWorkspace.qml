import QtQuick
import "../../style"

Rectangle {
    id: blankWorkspace

    property string dataUi: "blank_workspace"
    property string dataState: "blank"
    property string placementRole: "empty_canvas"
    property string surfaceRecipeId: "blank_canvas_surface"
    property var controller: null

    color: UiStyle.colorWorkspace
    border.width: UiStyle.borderNone
}
