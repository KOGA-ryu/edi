import QtQuick

QtObject {
    readonly property var features: [
        { id: "ui_taxonomy", label: "UI Taxonomy", enabled: true, target: "main_workspace" },
        { id: "review_gate", label: "Review Gate", enabled: true, target: "main_workspace" },
        { id: "text_editor", label: "Text Editor", enabled: false, target: "main_workspace" }
    ]
}
