import QtQuick
import QtQuick.Layouts
import "../../style"
import "../../components"

ColumnLayout {
    id: root

    property string dataUi: "text_editor_bottom_panel"
    property string dataState: "shelf"
    property var controller: null
    property int controllerRevision: controller ? controller.revision : 0
    readonly property bool hasCloseEvent: controller && controller.textEditorCloseStatus.length > 0

    spacing: UiStyle.space0

    Repeater {
        model: hasCloseEvent
            ? [{ label: "Close", value: root.controller.textEditorCloseStatus }]
            : []

        delegate: UiListRow {
            Layout.fillWidth: true
            Layout.preferredHeight: 20
            label: modelData.label
            meta: modelData.value
            metaMaxWidth: 520
        }
    }

    Item { Layout.fillHeight: true }
}
