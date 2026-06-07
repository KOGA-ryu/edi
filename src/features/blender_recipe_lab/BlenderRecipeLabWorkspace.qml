import QtQuick
import "../../style"
import "../drawing_tool"

Rectangle {
    id: workspace

    property var controller: null
    readonly property alias blenderSession: session

    color: UiStyle.colorWorkspace
    border.width: UiStyle.borderNone

    BlenderRecipeLabSession {
        id: session
    }

    DrawingToolWorkspace {
        anchors.fill: parent
        controller: workspace.controller
    }
}
