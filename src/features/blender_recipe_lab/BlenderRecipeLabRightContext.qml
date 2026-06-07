import QtQuick
import "../../style"
import "../drawing_tool"

Rectangle {
    id: context

    property var controller: null
    readonly property alias blenderSession: session

    color: UiStyle.colorPanelAlt
    border.width: UiStyle.borderNone

    BlenderRecipeLabSession {
        id: session
    }

    DrawingToolRightContext {
        anchors.fill: parent
        controller: context.controller
    }
}
