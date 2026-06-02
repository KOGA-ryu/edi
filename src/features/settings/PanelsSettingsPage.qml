import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../../style"
import "../../components"

ScrollView {
    id: root

    property var controller: null
    property int windowWidth: controller && controller.targetRoot ? controller.targetRoot.width : 0
    property int windowHeight: controller && controller.targetRoot ? controller.targetRoot.height : 0

    clip: true

    function saveLabel() {
        if (!controller) {
            return "Save Layout"
        }
        if (!controller.shellLayoutSaveOk) {
            return "Save Failed"
        }
        return controller.shellLayoutDirty ? "Save Layout" : "Saved"
    }

    function panelState(panelId) {
        if (!controller) {
            return "visible"
        }
        windowWidth
        windowHeight
        return controller.panelState(panelId)
    }

    function panelMeta(panelId) {
        if (!controller) {
            return ""
        }
        windowWidth
        windowHeight
        return controller.panelStateLabel(panelId) + " / " + controller.panelStateDetail(panelId)
    }

    function panelStateRowLabel(panelId, label) {
        return label + ": " + panelState(panelId).replace("_", "-")
    }

    function panelStateRowMeta(panelId) {
        if (!controller) {
            return ""
        }
        windowWidth
        windowHeight
        return controller.panelStateDetail(panelId)
    }

    function panelButtonLabel(panelId) {
        return controller && controller.panelManualCollapsed(panelId) ? "Show" : "Hide"
    }

    ColumnLayout {
        width: Math.max(parent.width - 18, 520)
        spacing: UiStyle.space10

        UiPanel {
            Layout.fillWidth: true
            implicitHeight: 164

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: UiStyle.space12
                spacing: UiStyle.space8

                UiSectionHeader {
                    title: "Current Panel State"
                    Layout.fillWidth: true
                }

                UiListRow {
                    Layout.fillWidth: true
                    label: root.panelStateRowLabel("left", "Left")
                    meta: root.panelStateRowMeta("left")
                }

                UiListRow {
                    Layout.fillWidth: true
                    label: root.panelStateRowLabel("right", "Right")
                    meta: root.panelStateRowMeta("right")
                }

                UiListRow {
                    Layout.fillWidth: true
                    label: root.panelStateRowLabel("bottom", "Bottom")
                    meta: root.panelStateRowMeta("bottom")
                }
            }
        }

        UiPanel {
            Layout.fillWidth: true
            implicitHeight: 152

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: UiStyle.space12
                spacing: UiStyle.space8

                UiSectionHeader {
                    title: "Layout Source"
                    Layout.fillWidth: true
                }

                UiListRow {
                    Layout.fillWidth: true
                    label: "Loaded file"
                    meta: root.controller ? root.controller.shellLayoutPath : ""
                }

                UiListRow {
                    Layout.fillWidth: true
                    label: "Persistence"
                    meta: root.controller && root.controller.shellLayoutDirty ? "unsaved" : "saved"
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: UiStyle.space8

                    UiButton {
                        label: "Reset Layout"
                        implicitWidth: 120
                        onClicked: root.controller.resetShellLayout()
                    }

                    UiButton {
                        label: root.saveLabel()
                        enabled: root.controller && root.controller.shellLayoutDirty
                        implicitWidth: 120
                        onClicked: root.controller.saveShellLayout()
                    }

                    Text {
                        Layout.fillWidth: true
                        text: "Drag splitters or use controls below."
                        color: UiStyle.colorTextFaint
                        font.family: UiStyle.fontSans
                        font.pixelSize: UiStyle.fontSizeSm
                        horizontalAlignment: Text.AlignRight
                    }
                }
            }
        }

        UiPanel {
            Layout.fillWidth: true
            implicitHeight: 142

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: UiStyle.space12
                spacing: UiStyle.space8

                UiSectionHeader {
                    title: "Layout Presets"
                    Layout.fillWidth: true
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: UiStyle.space8

                    UiButton {
                        label: "Full"
                        implicitWidth: 96
                        onClicked: root.controller.applyLayoutPreset("full")
                    }

                    UiButton {
                        label: "Focus"
                        implicitWidth: 96
                        onClicked: root.controller.applyLayoutPreset("focus")
                    }

                    UiButton {
                        label: "Review"
                        implicitWidth: 96
                        onClicked: root.controller.applyLayoutPreset("review")
                    }

                    UiButton {
                        label: "Tiny"
                        implicitWidth: 96
                        onClicked: root.controller.applyLayoutPreset("tiny")
                    }
                }

                Text {
                    Layout.fillWidth: true
                    text: "Presets change the working layout only. Use Save Layout to make one the startup layout."
                    color: UiStyle.colorTextFaint
                    font.family: UiStyle.fontSans
                    font.pixelSize: UiStyle.fontSizeSm
                    wrapMode: Text.WordWrap
                }
            }
        }

        UiPanel {
            Layout.fillWidth: true
            implicitHeight: 208

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: UiStyle.space12
                spacing: UiStyle.space8

                UiSectionHeader {
                    title: "Panel Visibility"
                    Layout.fillWidth: true
                }

                RowLayout {
                    Layout.fillWidth: true
                    Text {
                        Layout.fillWidth: true
                        text: "Left panel"
                        color: UiStyle.colorTextMuted
                        font.family: UiStyle.fontSans
                        font.pixelSize: UiStyle.fontSizeBody
                    }
                    UiListRow {
                        label: root.panelState("left")
                        meta: root.panelMeta("left")
                        Layout.preferredWidth: 310
                    }
                    UiButton {
                        label: root.panelButtonLabel("left")
                        selected: root.panelState("left") === "visible"
                        implicitWidth: 96
                        onClicked: root.controller.toggleLeftPanel()
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    Text {
                        Layout.fillWidth: true
                        text: "Right panel"
                        color: UiStyle.colorTextMuted
                        font.family: UiStyle.fontSans
                        font.pixelSize: UiStyle.fontSizeBody
                    }
                    UiListRow {
                        label: root.panelState("right")
                        meta: root.panelMeta("right")
                        Layout.preferredWidth: 310
                    }
                    UiButton {
                        label: root.panelButtonLabel("right")
                        selected: root.panelState("right") === "visible"
                        implicitWidth: 96
                        onClicked: root.controller.toggleRightPanel()
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    Text {
                        Layout.fillWidth: true
                        text: "Bottom panel"
                        color: UiStyle.colorTextMuted
                        font.family: UiStyle.fontSans
                        font.pixelSize: UiStyle.fontSizeBody
                    }
                    UiListRow {
                        label: root.panelState("bottom")
                        meta: root.panelMeta("bottom")
                        Layout.preferredWidth: 310
                    }
                    UiButton {
                        label: root.panelButtonLabel("bottom")
                        selected: root.panelState("bottom") === "visible"
                        implicitWidth: 96
                        onClicked: root.controller.toggleBottomPanel()
                    }
                }
            }
        }

        UiPanel {
            Layout.fillWidth: true
            implicitHeight: 184

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: UiStyle.space12
                spacing: UiStyle.space8

                UiSectionHeader {
                    title: "Panel Sizes"
                    Layout.fillWidth: true
                }

                RowLayout {
                    Layout.fillWidth: true
                    Text {
                        Layout.fillWidth: true
                        text: "Left width"
                        color: UiStyle.colorTextMuted
                        font.family: UiStyle.fontSans
                        font.pixelSize: UiStyle.fontSizeBody
                    }
                    UiButton {
                        label: "-"
                        implicitWidth: 42
                        onClicked: root.controller.setLeftPanelWidth(root.controller.leftPanelWidth - 20)
                    }
                    UiListRow {
                        label: String(root.controller ? root.controller.leftPanelWidth : 0) + " px"
                        meta: "180-520"
                        Layout.preferredWidth: 170
                    }
                    UiButton {
                        label: "+"
                        implicitWidth: 42
                        onClicked: root.controller.setLeftPanelWidth(root.controller.leftPanelWidth + 20)
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    Text {
                        Layout.fillWidth: true
                        text: "Right width"
                        color: UiStyle.colorTextMuted
                        font.family: UiStyle.fontSans
                        font.pixelSize: UiStyle.fontSizeBody
                    }
                    UiButton {
                        label: "-"
                        implicitWidth: 42
                        onClicked: root.controller.setRightPanelWidth(root.controller.rightPanelWidth - 20)
                    }
                    UiListRow {
                        label: String(root.controller ? root.controller.rightPanelWidth : 0) + " px"
                        meta: "240-460"
                        Layout.preferredWidth: 170
                    }
                    UiButton {
                        label: "+"
                        implicitWidth: 42
                        onClicked: root.controller.setRightPanelWidth(root.controller.rightPanelWidth + 20)
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    Text {
                        Layout.fillWidth: true
                        text: "Bottom height"
                        color: UiStyle.colorTextMuted
                        font.family: UiStyle.fontSans
                        font.pixelSize: UiStyle.fontSizeBody
                    }
                    UiButton {
                        label: "-"
                        implicitWidth: 42
                        onClicked: root.controller.setBottomPanelHeight(root.controller.bottomPanelHeight - 20)
                    }
                    UiListRow {
                        label: String(root.controller ? root.controller.bottomPanelHeight : 0) + " px"
                        meta: "96-360"
                        Layout.preferredWidth: 170
                    }
                    UiButton {
                        label: "+"
                        implicitWidth: 42
                        onClicked: root.controller.setBottomPanelHeight(root.controller.bottomPanelHeight + 20)
                    }
                }
            }
        }

        Item { Layout.preferredHeight: UiStyle.space10 }
    }
}
