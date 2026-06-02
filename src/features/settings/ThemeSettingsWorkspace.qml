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

    color: UiStyle.colorWorkspace

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
        anchors.margins: UiStyle.space12
        spacing: UiStyle.space10

        RowLayout {
            Layout.fillWidth: true
            spacing: UiStyle.space10

            Text {
                Layout.fillWidth: true
                text: "Settings"
                color: UiStyle.colorText
                font.family: UiStyle.fontSans
                font.pixelSize: 22
                font.weight: UiStyle.fontWeightSemiBold
            }

            UiStatusChip {
                status: "pending"
                label: root.selectedPage === "panels"
                    ? (root.controller && root.controller.shellLayoutDirty ? "unsaved" : "saved")
                    : "preview only"
            }
        }

        ScrollView {
            visible: root.selectedPage === "theme"
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            ColumnLayout {
                width: Math.max(parent.width - 18, 520)
                spacing: UiStyle.space10

                UiPanel {
                    Layout.fillWidth: true
                    implicitHeight: 154

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: UiStyle.space12
                        spacing: UiStyle.space8

                        UiSectionHeader {
                            title: "Theme Source"
                            Layout.fillWidth: true
                        }

                        UiListRow {
                            Layout.fillWidth: true
                            label: "Loaded file"
                            meta: root.controller ? root.controller.uiThemePath : ""
                        }

                        UiListRow {
                            Layout.fillWidth: true
                            label: "Persistence"
                            meta: "disabled"
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            Text {
                                Layout.preferredWidth: 120
                                text: "Mode"
                                color: UiStyle.colorTextMuted
                                font.family: UiStyle.fontSans
                                font.pixelSize: UiStyle.fontSizeBody
                            }
                            RowLayout {
                                spacing: UiStyle.space6
                                Repeater {
                                    model: ["dark", "light", "system"]
                                    delegate: UiButton {
                                        label: modelData
                                        selected: root.selectedThemeMode === modelData
                                        implicitWidth: 72
                                        onClicked: {
                                            root.selectedThemeMode = modelData
                                            root.applyPreview()
                                        }
                                    }
                                }
                            }
                            Text {
                                Layout.fillWidth: true
                                text: "stored for parity"
                                color: UiStyle.colorTextFaint
                                font.family: UiStyle.fontSans
                                font.pixelSize: UiStyle.fontSizeSm
                            }
                        }
                    }
                }

                UiPanel {
                    Layout.fillWidth: true
                    implicitHeight: 244

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: UiStyle.space12
                        spacing: UiStyle.space8

                        UiSectionHeader {
                            title: "Four Color Palette"
                            Layout.fillWidth: true
                        }

                        GridLayout {
                            Layout.fillWidth: true
                            columns: 3
                            columnSpacing: UiStyle.space10
                            rowSpacing: UiStyle.space8

                            Text { text: "Base"; color: UiStyle.colorTextMuted; font.family: UiStyle.fontSans; font.pixelSize: UiStyle.fontSizeBody }
                            Rectangle { Layout.preferredWidth: 28; Layout.preferredHeight: 28; radius: UiStyle.radiusSm; color: root.validHex(baseField.text) ? baseField.text : UiStyle.colorDanger; border.width: UiStyle.borderThin; border.color: UiStyle.colorBorderMajor }
                            UiTextField { id: baseField; Layout.fillWidth: true; onTextChanged: root.applyPreview() }

                            Text { text: "Surface"; color: UiStyle.colorTextMuted; font.family: UiStyle.fontSans; font.pixelSize: UiStyle.fontSizeBody }
                            Rectangle { Layout.preferredWidth: 28; Layout.preferredHeight: 28; radius: UiStyle.radiusSm; color: root.validHex(surfaceField.text) ? surfaceField.text : UiStyle.colorDanger; border.width: UiStyle.borderThin; border.color: UiStyle.colorBorderMajor }
                            UiTextField { id: surfaceField; Layout.fillWidth: true; onTextChanged: root.applyPreview() }

                            Text { text: "Accent"; color: UiStyle.colorTextMuted; font.family: UiStyle.fontSans; font.pixelSize: UiStyle.fontSizeBody }
                            Rectangle { Layout.preferredWidth: 28; Layout.preferredHeight: 28; radius: UiStyle.radiusSm; color: root.validHex(accentField.text) ? accentField.text : UiStyle.colorDanger; border.width: UiStyle.borderThin; border.color: UiStyle.colorBorderMajor }
                            UiTextField { id: accentField; Layout.fillWidth: true; onTextChanged: root.applyPreview() }

                            Text { text: "Text"; color: UiStyle.colorTextMuted; font.family: UiStyle.fontSans; font.pixelSize: UiStyle.fontSizeBody }
                            Rectangle { Layout.preferredWidth: 28; Layout.preferredHeight: 28; radius: UiStyle.radiusSm; color: root.validHex(textField.text) ? textField.text : UiStyle.colorDanger; border.width: UiStyle.borderThin; border.color: UiStyle.colorBorderMajor }
                            UiTextField { id: textField; Layout.fillWidth: true; onTextChanged: root.applyPreview() }
                        }

                        Text {
                            Layout.fillWidth: true
                            text: root.fieldsValid() ? "Valid #RRGGBB palette. Derived controls, borders, hover, selection, and muted text update from these four colors." : "Use #RRGGBB values for all colors."
                            color: root.fieldsValid() ? UiStyle.colorSuccess : UiStyle.colorWarning
                            font.family: UiStyle.fontSans
                            font.pixelSize: UiStyle.fontSizeSm
                            wrapMode: Text.WordWrap
                        }
                    }
                }

                UiPanel {
                    Layout.fillWidth: true
                    implicitHeight: 220

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: UiStyle.space12
                        spacing: UiStyle.space8

                        UiSectionHeader {
                            title: "Typography"
                            Layout.fillWidth: true
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            Text { Layout.preferredWidth: 120; text: "UI font"; color: UiStyle.colorTextMuted; font.family: UiStyle.fontSans; font.pixelSize: UiStyle.fontSizeBody }
                            UiTextField { id: uiFontNameField; Layout.fillWidth: true; placeholderText: "system default"; onTextChanged: root.applyPreview() }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            Text { Layout.preferredWidth: 120; text: "Code font"; color: UiStyle.colorTextMuted; font.family: UiStyle.fontSans; font.pixelSize: UiStyle.fontSizeBody }
                            UiTextField { id: codeFontNameField; Layout.fillWidth: true; placeholderText: "system default"; onTextChanged: root.applyPreview() }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            Text { Layout.preferredWidth: 120; text: "UI font size"; color: UiStyle.colorTextMuted; font.family: UiStyle.fontSans; font.pixelSize: UiStyle.fontSizeBody }
                            UiTextField { id: uiFontField; Layout.preferredWidth: 92; onTextChanged: root.applyPreview() }
                            Text { Layout.fillWidth: true; text: "px"; color: UiStyle.colorTextFaint; font.family: UiStyle.fontSans; font.pixelSize: UiStyle.fontSizeBody }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            Text { Layout.preferredWidth: 120; text: "Code font size"; color: UiStyle.colorTextMuted; font.family: UiStyle.fontSans; font.pixelSize: UiStyle.fontSizeBody }
                            UiTextField { id: codeFontField; Layout.preferredWidth: 92; onTextChanged: root.applyPreview() }
                            Text { Layout.fillWidth: true; text: "px"; color: UiStyle.colorTextFaint; font.family: UiStyle.fontSans; font.pixelSize: UiStyle.fontSizeBody }
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: UiStyle.space8

                    UiButton {
                        label: "Reset Loaded"
                        implicitWidth: 120
                        onClicked: {
                            root.loadTheme(root.controller ? root.controller.uiThemeDocument : ({}))
                            root.applyPreview()
                        }
                    }

                    UiButton {
                        label: "Save Theme"
                        enabled: false
                        implicitWidth: 120
                    }

                    Text {
                        Layout.fillWidth: true
                        text: "Save disabled until receipt and backup behavior are approved."
                        color: UiStyle.colorTextFaint
                        font.family: UiStyle.fontSans
                        font.pixelSize: UiStyle.fontSizeSm
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

        UiPanel {
            visible: root.selectedPage !== "theme" && root.selectedPage !== "panels"
            Layout.fillWidth: true
            Layout.fillHeight: true

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: UiStyle.space12
                spacing: UiStyle.space8

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
}
