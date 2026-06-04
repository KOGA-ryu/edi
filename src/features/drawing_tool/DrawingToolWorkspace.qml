import QtQuick
import "../../style"

Rectangle {
    id: drawingWorkspace

    property string dataUi: "drawing_tool_workspace"
    property string dataState: "blank"
    property string placementRole: "drawing_canvas_host"
    property string surfaceRecipeId: "drawing_tool_blank_surface"
    property var controller: null

    color: UiStyle.colorWorkspace
    border.width: UiStyle.borderNone
}
