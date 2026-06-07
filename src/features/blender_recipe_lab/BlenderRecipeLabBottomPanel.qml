import QtQuick
import "../../style"
import "../text_editor"

Rectangle {
    id: panel

    property var controller: null
    readonly property alias blenderSession: session

    color: UiStyle.colorBottomPanel
    border.width: UiStyle.borderNone

    BlenderRecipeLabSession {
        id: session
    }

    TextEditorWorkspace {
        anchors.fill: parent
        controller: panel.controller
    }
}
