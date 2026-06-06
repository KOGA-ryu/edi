import QtQuick
import QtQuick.Layouts
import "../../components"
import "../../style"

ColumnLayout {
    id: root

    property string dataUi: "text_editor_left_panel"
    property string dataState: "local_draft"
    property var controller: null

    spacing: 0

    UiSectionHeader {
        title: "Documents"
        Layout.fillWidth: true
    }

    Repeater {
        model: root.controller ? root.controller.textEditorDocuments : []

        delegate: UiListRow {
            Layout.fillWidth: true
            label: (modelData.name || "untitled.txt")
                + (root.controller && root.controller.textEditorDocumentModified(modelData) ? " *" : "")
            meta: ""
            selected: root.controller && modelData.id === root.controller.activeTextEditorDocumentId
            clickable: true
            metaMaxWidth: 80
            onClicked: if (root.controller) root.controller.selectTextEditorDocument(modelData.id)
        }
    }

    UiTextField {
        id: renameField
        Layout.fillWidth: true
        Layout.preferredHeight: 24
        Layout.topMargin: UiStyle.space4
        visible: root.controller && root.controller.textEditorRenameActive
        text: root.controller ? root.controller.textEditorDocumentName : ""
        selectByMouse: true
        onAccepted: if (root.controller) root.controller.applyTextEditorRename(text)
        Keys.onEscapePressed: if (root.controller) root.controller.cancelTextEditorRename()
        onVisibleChanged: {
            if (visible) {
                text = root.controller ? root.controller.textEditorDocumentName : ""
                forceActiveFocus()
                selectAll()
            }
        }
    }

    RowLayout {
        Layout.fillWidth: true
        Layout.topMargin: UiStyle.space4
        Layout.bottomMargin: UiStyle.space4
        spacing: UiStyle.space4

        UiIconButton {
            Layout.preferredWidth: 28
            Layout.preferredHeight: 24
            label: "New"
            iconText: "+"
            tooltip: "New document"
            onClicked: if (root.controller) root.controller.newTextEditorDocument()
        }

        UiIconButton {
            Layout.preferredWidth: 28
            Layout.preferredHeight: 24
            label: "Rename"
            iconText: "N"
            tooltip: "Rename document"
            selected: root.controller && root.controller.textEditorRenameActive
            onClicked: if (root.controller) root.controller.renameActiveTextEditorDocument()
        }

        UiIconButton {
            Layout.preferredWidth: 28
            Layout.preferredHeight: 24
            label: "Dup"
            iconText: "D"
            tooltip: "Duplicate document"
            onClicked: if (root.controller) root.controller.duplicateTextEditorDocument()
        }

        UiIconButton {
            Layout.preferredWidth: 28
            Layout.preferredHeight: 24
            label: "Close"
            iconText: "X"
            tooltip: "Close document"
            enabled: root.controller && root.controller.textEditorDocuments.length > 1
            onClicked: if (root.controller) root.controller.closeActiveTextEditorDocument()
        }

        Item { Layout.fillWidth: true }
    }
}
