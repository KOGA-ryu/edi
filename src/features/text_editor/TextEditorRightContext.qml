import QtQuick
import QtQuick.Layouts
import "../../style"
import "../../components"

ColumnLayout {
    id: root

    property string dataUi: "text_editor_right_context"
    property string dataState: "inspector"
    property var controller: null
    property int controllerRevision: controller ? controller.revision : 0

    spacing: UiStyle.space2

    UiSectionHeader { title: "Document"; Layout.fillWidth: true }

    UiListRow {
        Layout.fillWidth: true
        label: "Language"
        meta: root.controller ? root.controller.textEditorLanguage : "text"
        metaMaxWidth: 110
    }

    UiListRow {
        Layout.fillWidth: true
        label: "Lines"
        meta: root.controller ? String(root.controller.textEditorLineCount(root.controllerRevision)) : "1"
        metaMaxWidth: 64
    }

    UiListRow {
        Layout.fillWidth: true
        label: "Chars"
        meta: root.controller ? String(root.controller.textEditorCharCount(root.controllerRevision)) : "0"
        metaMaxWidth: 80
    }

    UiListRow {
        Layout.fillWidth: true
        label: "State"
        meta: root.controller ? root.controller.textEditorModifiedState(root.controllerRevision) : "clean"
        metaMaxWidth: 92
    }

    UiListRow {
        Layout.fillWidth: true
        label: "Open"
        meta: root.controller ? String(root.controller.textEditorDocuments.length) : "1"
        metaMaxWidth: 64
    }

    Item { Layout.fillHeight: true }
}
