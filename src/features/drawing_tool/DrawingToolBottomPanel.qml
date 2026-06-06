import QtQuick
import "../../style"

Rectangle {
    id: drawingBottomPanel

    property string dataUi: "drawing_tool_bottom_panel"
    property string dataState: "intentionally_blank"
    property var controller: null

    color: UiStyle.colorTransparent
    border.width: UiStyle.borderNone
}
