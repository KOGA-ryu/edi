import QtQuick
import QtQuick.Layouts
import "../../style"
import "../../components"

UiPanel {
    id: panel
    property var controller: null
    property var route: controller ? controller.currentRoute() : ({})
    panelColor: UiStyle.colorPanelRaised
    panelBorder: UiStyle.colorBorderMinor
    panelRadius: UiStyle.radiusMd

    ColumnLayout {
        anchors.fill: parent
        spacing: UiStyle.space10

        RowLayout {
            Layout.fillWidth: true
            spacing: UiStyle.space10
            Text {
                Layout.fillWidth: true
                text: route.label || "Route"
                color: UiStyle.colorText
                font.family: UiStyle.fontSans
                font.pixelSize: 20
                font.weight: UiStyle.fontWeightSemiBold
                elide: Text.ElideRight
            }
            UiStatusChip {
                status: controller ? controller.routeStatus(route.id) : "pending"
            }
        }

        Text {
            Layout.fillWidth: true
            text: route.summary || ""
            color: UiStyle.colorTextMuted
            font.family: UiStyle.fontSans
            font.pixelSize: UiStyle.fontSizeBody
            wrapMode: Text.WordWrap
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 88
            color: UiStyle.colorWorkspaceBody
            radius: UiStyle.radiusMd
            border.width: UiStyle.borderThin
            border.color: UiStyle.colorBorderMinor
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: UiStyle.space10
                spacing: UiStyle.space6
                Text {
                    Layout.fillWidth: true
                    text: "Purpose"
                    color: UiStyle.colorAccent
                    font.family: UiStyle.fontSans
                    font.pixelSize: UiStyle.fontSizeXs
                    font.weight: UiStyle.fontWeightSemiBold
                }
                Text {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    text: route.purpose || ""
                    color: UiStyle.colorText
                    font.family: UiStyle.fontSans
                    font.pixelSize: UiStyle.fontSizeBody
                    wrapMode: Text.WordWrap
                }
            }
        }

        GridLayout {
            Layout.fillWidth: true
            columns: 2
            columnSpacing: UiStyle.space8
            rowSpacing: UiStyle.space8

            Repeater {
                model: route.objects || []
                delegate: Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: UiStyle.rowHeight
                    color: UiStyle.colorControl
                    radius: UiStyle.radiusSm
                    Text {
                        anchors.fill: parent
                        anchors.leftMargin: UiStyle.space8
                        anchors.rightMargin: UiStyle.space8
                        text: modelData
                        color: UiStyle.colorTextMuted
                        font.family: UiStyle.fontSans
                        font.pixelSize: UiStyle.fontSizeSm
                        verticalAlignment: Text.AlignVCenter
                        elide: Text.ElideRight
                    }
                }
            }
        }
    }
}
