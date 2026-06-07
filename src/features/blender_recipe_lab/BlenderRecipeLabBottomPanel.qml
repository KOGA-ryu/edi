import QtQuick
import "../../style"
import "../text_editor"

Rectangle {
    id: panel

    property var controller: null

    color: UiStyle.colorBottomPanel
    border.width: UiStyle.borderNone

    TextEditorWorkspace {
        anchors.fill: parent
        controller: panel.controller
    }
}
