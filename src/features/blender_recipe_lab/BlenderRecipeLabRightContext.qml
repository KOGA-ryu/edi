import QtQuick
import "../../style"
import "../drawing_tool"

Rectangle {
    id: context

    property var controller: null

    color: UiStyle.colorPanelAlt
    border.width: UiStyle.borderNone

    DrawingToolRightContext {
        anchors.fill: parent
        controller: context.controller
    }
}
