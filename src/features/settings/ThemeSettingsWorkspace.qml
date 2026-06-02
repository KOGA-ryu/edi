import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../../style"
import "../../components"

Rectangle {
    id: root

    property var controller: null
    property bool ready: false
    property string selectedThemeMode: "dark"
    property string selectedPage: controller ? controller.selectedSettingsPage : "theme"
    property int settingsLabelWidth: 64

    color: UiStyle.colorBase

    function validHex(value) {
        return /^#[0-9a-fA-F]{6}$/.test(String(value || "").trim())
    }

    function fieldTheme() {
        return {
            theme_mode: selectedThemeMode,
            base: baseField.text.trim(),
            surface: surfaceField.text.trim(),
            accent: accentField.text.trim(),
            text: textField.text.trim(),
            ui_font: uiFontNameField.text.trim(),
            code_font: codeFontNameField.text.trim(),
            ui_font_size: Number(uiFontField.text),
            code_font_size: Number(codeFontField.text)
        }
    }

    function fieldsValid() {
        return validHex(baseField.text)
            && validHex(surfaceField.text)
            && validHex(accentField.text)
            && validHex(textField.text)
            && Number(uiFontField.text) >= 9
            && Number(uiFontField.text) <= 28
            && Number(codeFontField.text) >= 9
            && Number(codeFontField.text) <= 28
    }

    function applyPreview() {
        if (!ready || !fieldsValid() || !controller) {
            return
        }
        controller.applyTheme(fieldTheme())
    }

    function loadTheme(theme) {
        var colors = theme && theme.colors ? theme.colors : theme || ({})
        var typography = theme && theme.typography ? theme.typography : theme || ({})
        var mode = String(theme && theme.theme_mode ? theme.theme_mode : "dark")
        selectedThemeMode = ["dark", "light", "system"].indexOf(mode) >= 0 ? mode : "dark"
        baseField.text = String(colors.base || "#333039").toUpperCase()
        surfaceField.text = String(colors.surface || "#373B4F").toUpperCase()
        accentField.text = String(colors.accent || "#D46CA1").toUpperCase()
        textField.text = String(colors.text || "#577DD1").toUpperCase()
        uiFontNameField.text = String(typography.ui_font || "")
        codeFontNameField.text = String(typography.code_font || "")
        uiFontField.text = String(typography.ui_font_size || 13)
        codeFontField.text = String(typography.code_font_size || 12)
    }

    Component.onCompleted: {
        loadTheme(controller ? controller.uiThemeDocument : ({}))
        ready = true
        applyPreview()
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: UiStyle.space8
        spacing: UiStyle.space8

        ScrollView {
            visible: root.selectedPage === "theme"
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            ColumnLayout {
                width: Math.max(Math.min(parent.width - 14, 620), 320)
                spacing: UiStyle.space12

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: UiStyle.space4

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: UiStyle.space8

                        Text {
                            Layout.minimumWidth: root.settingsLabelWidth
                            Layout.preferredWidth: root.settingsLabelWidth
                            Layout.maximumWidth: root.settingsLabelWidth
                            text: "Mode"
                            color: UiStyle.colorTextMuted
                            font.family: UiStyle.fontSans
                            font.pixelSize: UiStyle.fontSizeSm
                        }

                        RowLayout {
                            spacing: UiStyle.space4
                            Repeater {
                                model: ["dark", "light", "system"]
                                delegate: UiButton {
                                    label: modelData
                                    selected: root.selectedThemeMode === modelData
                                    implicitWidth: 66
                                    implicitHeight: 26
                                    onClicked: {
                                        root.selectedThemeMode = modelData
                                        root.applyPreview()
                                    }
                                }
                            }
                        }

                        Item { Layout.fillWidth: true }
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: UiStyle.space4

                    UiSectionHeader {
                        title: "Palette"
                        Layout.fillWidth: true
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: UiStyle.space4

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: UiStyle.space8
                            Text { Layout.minimumWidth: root.settingsLabelWidth; Layout.preferredWidth: root.settingsLabelWidth; Layout.maximumWidth: root.settingsLabelWidth; text: "Base"; color: UiStyle.colorTextMuted; font.family: UiStyle.fontSans; font.pixelSize: UiStyle.fontSizeSm }
                            Rectangle { Layout.preferredWidth: 18; Layout.preferredHeight: 18; radius: 3; color: UiStyle.colorControl; Rectangle { anchors.centerIn: parent; width: 12; height: 12; radius: 2; color: root.validHex(baseField.text) ? baseField.text : UiStyle.colorDanger } }
                            UiTextField { id: baseField; Layout.fillWidth: true; Layout.preferredHeight: 26; onTextChanged: root.applyPreview() }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: UiStyle.space8
                            Text { Layout.minimumWidth: root.settingsLabelWidth; Layout.preferredWidth: root.settingsLabelWidth; Layout.maximumWidth: root.settingsLabelWidth; text: "Surface"; color: UiStyle.colorTextMuted; font.family: UiStyle.fontSans; font.pixelSize: UiStyle.fontSizeSm }
                            Rectangle { Layout.preferredWidth: 18; Layout.preferredHeight: 18; radius: 3; color: UiStyle.colorControl; Rectangle { anchors.centerIn: parent; width: 12; height: 12; radius: 2; color: root.validHex(surfaceField.text) ? surfaceField.text : UiStyle.colorDanger } }
                            UiTextField { id: surfaceField; Layout.fillWidth: true; Layout.preferredHeight: 26; onTextChanged: root.applyPreview() }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: UiStyle.space8
                            Text { Layout.minimumWidth: root.settingsLabelWidth; Layout.preferredWidth: root.settingsLabelWidth; Layout.maximumWidth: root.settingsLabelWidth; text: "Accent"; color: UiStyle.colorTextMuted; font.family: UiStyle.fontSans; font.pixelSize: UiStyle.fontSizeSm }
                            Rectangle { Layout.preferredWidth: 18; Layout.preferredHeight: 18; radius: 3; color: UiStyle.colorControl; Rectangle { anchors.centerIn: parent; width: 12; height: 12; radius: 2; color: root.validHex(accentField.text) ? accentField.text : UiStyle.colorDanger } }
                            UiTextField { id: accentField; Layout.fillWidth: true; Layout.preferredHeight: 26; onTextChanged: root.applyPreview() }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: UiStyle.space8
                            Text { Layout.minimumWidth: root.settingsLabelWidth; Layout.preferredWidth: root.settingsLabelWidth; Layout.maximumWidth: root.settingsLabelWidth; text: "Text"; color: UiStyle.colorTextMuted; font.family: UiStyle.fontSans; font.pixelSize: UiStyle.fontSizeSm }
                            Rectangle { Layout.preferredWidth: 18; Layout.preferredHeight: 18; radius: 3; color: UiStyle.colorControl; Rectangle { anchors.centerIn: parent; width: 12; height: 12; radius: 2; color: root.validHex(textField.text) ? textField.text : UiStyle.colorDanger } }
                            UiTextField { id: textField; Layout.fillWidth: true; Layout.preferredHeight: 26; onTextChanged: root.applyPreview() }
                        }
                    }

                    Text {
                        visible: !root.fieldsValid()
                        Layout.fillWidth: true
                        text: "invalid #RRGGBB or font size"
                        color: UiStyle.colorWarning
                        font.family: UiStyle.fontSans
                        font.pixelSize: UiStyle.fontSizeXs
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: UiStyle.space4

                    UiSectionHeader {
                        title: "Typography"
                        Layout.fillWidth: true
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: UiStyle.space8
                        Text { Layout.minimumWidth: root.settingsLabelWidth; Layout.preferredWidth: root.settingsLabelWidth; Layout.maximumWidth: root.settingsLabelWidth; text: "UI font"; color: UiStyle.colorTextMuted; font.family: UiStyle.fontSans; font.pixelSize: UiStyle.fontSizeSm }
                        UiTextField { id: uiFontNameField; Layout.fillWidth: true; Layout.preferredHeight: 26; placeholderText: "system default"; onTextChanged: root.applyPreview() }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: UiStyle.space8
                        Text { Layout.minimumWidth: root.settingsLabelWidth; Layout.preferredWidth: root.settingsLabelWidth; Layout.maximumWidth: root.settingsLabelWidth; text: "Code font"; color: UiStyle.colorTextMuted; font.family: UiStyle.fontSans; font.pixelSize: UiStyle.fontSizeSm }
                        UiTextField { id: codeFontNameField; Layout.fillWidth: true; Layout.preferredHeight: 26; placeholderText: "system default"; onTextChanged: root.applyPreview() }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: UiStyle.space8
                        Text { Layout.minimumWidth: root.settingsLabelWidth; Layout.preferredWidth: root.settingsLabelWidth; Layout.maximumWidth: root.settingsLabelWidth; text: "UI size"; color: UiStyle.colorTextMuted; font.family: UiStyle.fontSans; font.pixelSize: UiStyle.fontSizeSm }
                        UiTextField { id: uiFontField; Layout.preferredWidth: 76; Layout.preferredHeight: 26; onTextChanged: root.applyPreview() }
                        Text { Layout.fillWidth: true; text: "px"; color: UiStyle.colorTextFaint; font.family: UiStyle.fontSans; font.pixelSize: UiStyle.fontSizeSm }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: UiStyle.space8
                        Text { Layout.minimumWidth: root.settingsLabelWidth; Layout.preferredWidth: root.settingsLabelWidth; Layout.maximumWidth: root.settingsLabelWidth; text: "Code size"; color: UiStyle.colorTextMuted; font.family: UiStyle.fontSans; font.pixelSize: UiStyle.fontSizeSm }
                        UiTextField { id: codeFontField; Layout.preferredWidth: 76; Layout.preferredHeight: 26; onTextChanged: root.applyPreview() }
                        Text { Layout.fillWidth: true; text: "px"; color: UiStyle.colorTextFaint; font.family: UiStyle.fontSans; font.pixelSize: UiStyle.fontSizeSm }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: UiStyle.space6

                    UiButton {
                        label: "Reset"
                        implicitWidth: 76
                        implicitHeight: 26
                        onClicked: {
                            root.loadTheme(root.controller ? root.controller.uiThemeDocument : ({}))
                            root.applyPreview()
                        }
                    }

                    UiButton {
                        label: "Save"
                        enabled: false
                        implicitWidth: 76
                        implicitHeight: 26
                    }

                    Text {
                        Layout.fillWidth: true
                        text: "save disabled"
                        color: UiStyle.colorTextFaint
                        font.family: UiStyle.fontSans
                        font.pixelSize: UiStyle.fontSizeXs
                        horizontalAlignment: Text.AlignRight
                    }
                }
            }
        }

        PanelsSettingsPage {
            controller: root.controller
            visible: root.selectedPage === "panels"
            Layout.fillWidth: true
            Layout.fillHeight: true
        }

        ColumnLayout {
            visible: root.selectedPage !== "theme" && root.selectedPage !== "panels"
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: UiStyle.space4

            UiSectionHeader {
                title: "Reserved Settings Page"
                Layout.fillWidth: true
            }

            UiListRow {
                Layout.fillWidth: true
                label: "Page"
                meta: root.selectedPage
            }

            UiListRow {
                Layout.fillWidth: true
                label: "Status"
                meta: "reserved"
            }

            UiListRow {
                Layout.fillWidth: true
                label: "Writes"
                meta: "disabled"
            }

            Item { Layout.fillHeight: true }
        }
    }
}
