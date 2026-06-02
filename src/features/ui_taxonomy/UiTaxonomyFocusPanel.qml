import QtQuick
import QtQuick.Layouts
import "../../style"
import "../../components"

UiPanel {
    id: panel
    property var controller: null
    property var route: controller ? controller.currentRoute() : ({})
    panelColor: UiStyle.colorPanelRaised
    panelRadius: UiStyle.radiusSm
    panelPadding: UiStyle.space8

    ColumnLayout {
        anchors.fill: parent
        spacing: UiStyle.space6

        Text {
            Layout.fillWidth: true
            text: route.summary || ""
            color: UiStyle.colorTextMuted
            font.family: UiStyle.fontSans
            font.pixelSize: UiStyle.fontSizeBody
            wrapMode: Text.WordWrap
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: UiStyle.space2

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
                text: route.purpose || ""
                color: UiStyle.colorText
                font.family: UiStyle.fontSans
                font.pixelSize: UiStyle.fontSizeBody
                wrapMode: Text.WordWrap
            }
        }

        GridLayout {
            Layout.fillWidth: true
            columns: 2
            columnSpacing: UiStyle.space8
            rowSpacing: UiStyle.space4

            Repeater {
                model: route.objects || []
                delegate: Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 24
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
