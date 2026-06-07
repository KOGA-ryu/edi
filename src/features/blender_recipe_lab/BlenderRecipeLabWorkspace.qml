import QtQuick
import "../../style"
import "../drawing_tool"

Rectangle {
    id: workspace

    property var controller: null

    color: UiStyle.colorWorkspace
    border.width: UiStyle.borderNone

    DrawingToolWorkspace {
        anchors.fill: parent
        controller: workspace.controller
    }
}
