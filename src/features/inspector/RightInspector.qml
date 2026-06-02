import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../../style"
import "../../components"

ScrollView {
    id: inspector

    property var controller: null
    property var document: ({})

    clip: true

    function asArray(value) {
        if (!value) {
            return []
        }
        if (Array.isArray(value)) {
            return value
        }
        if (typeof value.length === "number") {
            var result = []
            for (var index = 0; index < value.length; ++index) {
                result.push(value[index])
            }
            return result
        }
        return []
    }

    function textValue(value) {
        return String(value === undefined || value === null ? "" : value)
    }

    function itemPath(item) {
        return typeof item === "string" ? item : textValue(item.path || item.label || item.value)
    }

    function itemRole(item) {
        return typeof item === "string" ? "source" : textValue(item.role || "source")
    }

    function actionSelected(action) {
        return action && action.kind === "status" && inspector.document && action.value === inspector.document.status
    }

    function sectionItems(section) {
        if (!section) {
            return []
        }
        if (section.type === "rows") {
            return inspector.asArray(section.rows)
        }
        if (section.type === "actions") {
            return inspector.asArray(section.actions)
        }
        return inspector.asArray(section.items)
    }

    function sectionHasContent(section) {
        if (!section || section.visible === false) {
            return false
        }
        if (section.type === "text") {
            var items = inspector.asArray(section.items)
            for (var index = 0; index < items.length; ++index) {
                if (inspector.textValue(items[index].value).length > 0) {
                    return true
                }
            }
            return false
        }
        return inspector.sectionItems(section).length > 0
    }

    ColumnLayout {
        width: Math.max(parent.width - 12, 148)
        spacing: UiStyle.space8

        Repeater {
            model: inspector.asArray(inspector.document.sections)

            delegate: ColumnLayout {
                id: section

                property var sectionData: modelData

                visible: inspector.sectionHasContent(sectionData)
                Layout.fillWidth: true
                spacing: UiStyle.space2

                ColumnLayout {
                    visible: section.sectionData.type === "rows"
                    Layout.fillWidth: true
                    spacing: UiStyle.space0

                    Repeater {
                        model: inspector.asArray(section.sectionData.rows)
                        delegate: UiListRow {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 22
                            label: inspector.textValue(modelData.label)
                            meta: inspector.textValue(modelData.value)
                            metaMaxWidth: Math.max(64, Math.min(220, width * 0.56))
                            hideMetaBelowWidth: 0
                        }
                    }
                }

                ColumnLayout {
                    visible: section.sectionData.type === "text"
                    Layout.fillWidth: true
                    spacing: UiStyle.space6

                    Repeater {
                        model: inspector.asArray(section.sectionData.items)
                        delegate: ColumnLayout {
                            visible: inspector.textValue(modelData.value).length > 0
                            Layout.fillWidth: true
                            spacing: UiStyle.space2

                            Text {
                                Layout.fillWidth: true
                                text: inspector.textValue(modelData.label)
                                color: UiStyle.colorAccent
                                font.family: UiStyle.fontSans
                                font.pixelSize: UiStyle.fontSizeXs
                                font.weight: UiStyle.fontWeightSemiBold
                                elide: Text.ElideRight
                            }

                            Text {
                                Layout.fillWidth: true
                                text: inspector.textValue(modelData.value)
                                color: UiStyle.colorTextMuted
                                font.family: UiStyle.fontSans
                                font.pixelSize: UiStyle.fontSizeSm
                                wrapMode: Text.WordWrap
                            }
                        }
                    }
                }

                ColumnLayout {
                    visible: section.sectionData.type === "code_refs"
                    Layout.fillWidth: true
                    spacing: UiStyle.space0

                    Repeater {
                        model: inspector.asArray(section.sectionData.items)
                        delegate: UiCodeRefRow {
                            Layout.fillWidth: true
                            path: inspector.itemPath(modelData)
                            role: inspector.itemRole(modelData)
                        }
                    }
                }

                ColumnLayout {
                    visible: section.sectionData.type === "notes"
                    Layout.fillWidth: true
                    spacing: UiStyle.space4

                    Repeater {
                        model: inspector.asArray(section.sectionData.items)
                        delegate: UiNoteCard {
                            Layout.fillWidth: true
                            status: inspector.textValue(modelData.status || "pending")
                            body: inspector.textValue(modelData.body)
                            createdAt: inspector.textValue(modelData.createdAt)
                        }
                    }
                }

                GridLayout {
                    visible: section.sectionData.type === "actions"
                    Layout.fillWidth: true
                    columns: width > 220 ? 2 : 1
                    columnSpacing: UiStyle.space4
                    rowSpacing: UiStyle.space4

                    Repeater {
                        model: inspector.asArray(section.sectionData.actions)
                        delegate: UiButton {
                            Layout.fillWidth: true
                            label: inspector.textValue(modelData.label)
                            selected: inspector.actionSelected(modelData)
                            enabled: inspector.controller !== null
                            onClicked: inspector.controller.runInspectorAction(modelData.id, inspector.document.targetId)
                        }
                    }
                }

                Item {
                    Layout.fillWidth: true
                    Layout.preferredHeight: UiStyle.space2
                }
            }
        }

        Item { Layout.fillHeight: true }
    }
}
