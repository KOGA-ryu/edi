import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../../components"
import "../../style"

Rectangle {
    id: root

    property string dataUi: "text_editor_workspace"
    property string dataState: "local_draft"
    property string placementRole: "editor_surface"
    property string surfaceRecipeId: "text_editor_surface"
    property var controller: null
    property int controllerRevision: controller ? controller.revision : 0
    property int commandRevision: controller ? controller.textEditorCommandRevision : 0
    property bool findActive: false

    color: UiStyle.colorWorkspace
    border.width: UiStyle.borderNone

    function syncController() {
        if (!root.controller || !editor) {
            return
        }
        root.controller.updateTextEditorState(
            editor.text,
            editor.cursorPosition,
            editor.selectionStart,
            editor.selectionEnd)
    }

    function clearEditor() {
        editor.text = ""
        editor.forceActiveFocus()
        root.syncController()
    }

    function undoEditor() {
        editor.undo()
        editor.forceActiveFocus()
    }

    function redoEditor() {
        editor.redo()
        editor.forceActiveFocus()
    }

    function selectAllEditor() {
        editor.selectAll()
        editor.forceActiveFocus()
        root.syncController()
    }

    function selectFindMatch(direction) {
        var query = findField.text
        if (!query.length || !editor.text.length) {
            return
        }

        var haystack = editor.text.toLowerCase()
        var needle = query.toLowerCase()
        var index = -1
        if (direction < 0) {
            var reverseStart = Math.max(0, editor.selectionStart - 1)
            index = haystack.lastIndexOf(needle, reverseStart)
            if (index < 0) {
                index = haystack.lastIndexOf(needle)
            }
        } else {
            var forwardStart = Math.max(0, editor.selectionEnd)
            index = haystack.indexOf(needle, forwardStart)
            if (index < 0) {
                index = haystack.indexOf(needle)
            }
        }

        if (index >= 0) {
            editor.forceActiveFocus()
            editor.select(index, index + query.length)
            root.syncController()
        }
    }

    function toggleFind() {
        findActive = !findActive
        if (findActive) {
            findField.forceActiveFocus()
            findField.selectAll()
        } else {
            editor.forceActiveFocus()
        }
    }

    function runRequestedCommand(command) {
        if (command === "undo") {
            root.undoEditor()
        } else if (command === "redo") {
            root.redoEditor()
        } else if (command === "select_all") {
            root.selectAllEditor()
        } else if (command === "clear") {
            root.clearEditor()
        } else if (command === "find") {
            if (!root.findActive) {
                root.toggleFind()
            } else {
                findField.forceActiveFocus()
                findField.selectAll()
            }
        }
    }

    onCommandRevisionChanged: {
        if (root.controller) {
            root.runRequestedCommand(root.controller.textEditorRequestedCommand)
        }
    }

    Shortcut {
        sequences: [StandardKey.Undo]
        onActivated: root.undoEditor()
    }

    Shortcut {
        sequences: [StandardKey.Redo]
        onActivated: root.redoEditor()
    }

    Shortcut {
        sequences: [StandardKey.SelectAll]
        onActivated: root.selectAllEditor()
    }

    Shortcut {
        sequences: [StandardKey.Find]
        onActivated: if (!root.findActive) root.toggleFind()
    }

    Shortcut {
        sequence: "Esc"
        enabled: root.findActive
        onActivated: root.toggleFind()
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: UiStyle.space6
        spacing: UiStyle.space4

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 26
            color: UiStyle.colorTransparent
            border.width: UiStyle.borderNone

            RowLayout {
                anchors.fill: parent
                spacing: UiStyle.space4

                Repeater {
                    model: root.controller ? root.controller.textEditorDocuments : []

                    delegate: UiTab {
                        Layout.preferredWidth: Math.min(144, Math.max(76, implicitWidth))
                        Layout.preferredHeight: 24
                        label: modelData.name || "untitled.txt"
                        active: root.controller && modelData.id === root.controller.activeTextEditorDocumentId
                        clickable: true
                        tooltip: (modelData.name || "untitled.txt") + " - "
                            + (root.controller ? root.controller.textEditorDocumentState(modelData) : "clean")
                        onClicked: if (root.controller) root.controller.selectTextEditorDocument(modelData.id)
                    }
                }

                Text {
                    Layout.fillWidth: true
                    text: root.controller && root.controller.textEditorModified ? "modified" : ""
                    color: UiStyle.colorPending
                    font.family: UiStyle.fontSans
                    font.pixelSize: UiStyle.fontSizeXs
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                }

                UiIconButton {
                    Layout.preferredWidth: 28
                    Layout.preferredHeight: 24
                    label: "Undo"
                    iconText: "U"
                    tooltip: "Undo"
                    onClicked: root.undoEditor()
                }

                UiIconButton {
                    Layout.preferredWidth: 28
                    Layout.preferredHeight: 24
                    label: "Redo"
                    iconText: "R"
                    tooltip: "Redo"
                    onClicked: root.redoEditor()
                }

                UiIconButton {
                    Layout.preferredWidth: 28
                    Layout.preferredHeight: 24
                    label: "Select All"
                    iconText: "A"
                    tooltip: "Select all"
                    onClicked: root.selectAllEditor()
                }

                UiIconButton {
                    Layout.preferredWidth: 28
                    Layout.preferredHeight: 24
                    label: "Clear"
                    iconText: "X"
                    tooltip: "Clear document text"
                    onClicked: root.clearEditor()
                }

                UiIconButton {
                    Layout.preferredWidth: 28
                    Layout.preferredHeight: 24
                    label: "Find"
                    iconText: "F"
                    tooltip: "Find text"
                    selected: root.findActive
                    onClicked: root.toggleFind()
                }

                UiTextField {
                    id: findField
                    Layout.preferredWidth: 138
                    Layout.preferredHeight: 24
                    visible: root.findActive
                    placeholderText: "find"
                    selectByMouse: true
                    onAccepted: root.selectFindMatch(1)
                    Keys.onEscapePressed: root.toggleFind()
                }

                UiButton {
                    Layout.preferredWidth: 36
                    Layout.preferredHeight: 24
                    visible: root.findActive
                    label: "Prev"
                    onClicked: root.selectFindMatch(-1)
                }

                UiButton {
                    Layout.preferredWidth: 36
                    Layout.preferredHeight: 24
                    visible: root.findActive
                    label: "Next"
                    onClicked: root.selectFindMatch(1)
                }

                UiIconButton {
                    Layout.preferredWidth: 28
                    Layout.preferredHeight: 24
                    label: "Wrap"
                    iconText: "W"
                    tooltip: "Toggle line wrap"
                    selected: root.controller && root.controller.textEditorWrapEnabled
                    onClicked: if (root.controller) root.controller.toggleTextEditorWrap()
                }

                UiIconButton {
                    Layout.preferredWidth: 28
                    Layout.preferredHeight: 24
                    label: "Lines"
                    iconText: "#"
                    tooltip: "Toggle line numbers"
                    selected: root.controller && root.controller.textEditorLineNumbersVisible
                    onClicked: if (root.controller) root.controller.toggleTextEditorLineNumbers()
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: UiStyle.mix(UiStyle.colorWorkspace, UiStyle.colorSurface, 0.42)
            radius: UiStyle.radiusSm
            border.width: UiStyle.borderNone
            clip: true

            RowLayout {
                anchors.fill: parent
                spacing: 0

                Rectangle {
                    Layout.preferredWidth: root.controller && root.controller.textEditorLineNumbersVisible ? 32 : 0
                    Layout.fillHeight: true
                    visible: root.controller && root.controller.textEditorLineNumbersVisible
                    color: UiStyle.mix(UiStyle.colorWorkspace, UiStyle.colorSurface, 0.30)
                    border.width: UiStyle.borderNone

                    Text {
                        anchors.top: parent.top
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.topMargin: UiStyle.space8
                        anchors.leftMargin: UiStyle.space4
                        anchors.rightMargin: UiStyle.space4
                        text: root.controller ? String(root.controller.textEditorLineCount(root.controller.revision)) : "1"
                        color: UiStyle.colorTextFaint
                        font.family: UiStyle.fontMono
                        font.pixelSize: UiStyle.fontSizeEditor
                        horizontalAlignment: Text.AlignRight
                    }
                }

                ScrollView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true

                    TextArea {
                        id: editor
                        text: root.controller ? root.controller.textEditorText : ""
                        placeholderText: "scratch text"
                        color: UiStyle.colorText
                        placeholderTextColor: UiStyle.colorTextFaint
                        selectedTextColor: UiStyle.colorWindow
                        selectionColor: UiStyle.colorAccent
                        font.family: UiStyle.fontMono
                        font.pixelSize: UiStyle.fontSizeEditor
                        wrapMode: root.controller && root.controller.textEditorWrapEnabled ? TextArea.Wrap : TextArea.NoWrap
                        selectByMouse: true
                        persistentSelection: true
                        background: Rectangle {
                            color: UiStyle.mix(UiStyle.colorWorkspace, UiStyle.colorSurface, 0.42)
                            border.width: UiStyle.borderNone
                        }
                        onTextChanged: root.syncController()
                        onCursorPositionChanged: root.syncController()
                        onSelectionStartChanged: root.syncController()
                        onSelectionEndChanged: root.syncController()
                        Component.onCompleted: root.syncController()
                    }
                }
            }
        }

        Text {
            Layout.fillWidth: true
            text: root.controller
                ? "line " + String(root.controller.textEditorCursorLine(root.controller.revision))
                    + " col " + String(root.controller.textEditorCursorColumn(root.controller.revision))
                    + " | selected " + String(root.controller.textEditorSelectionLength(root.controller.revision))
                : "line 1 col 1 | selected 0"
            color: UiStyle.colorTextFaint
            font.family: UiStyle.fontMono
            font.pixelSize: UiStyle.fontSizeXs
            elide: Text.ElideRight
        }
    }
}
