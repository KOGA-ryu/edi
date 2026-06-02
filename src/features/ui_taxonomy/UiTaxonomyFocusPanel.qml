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
    panelPadding: UiStyle.space6

    ColumnLayout {
        anchors.fill: parent
        spacing: UiStyle.space4

        Text {
            Layout.fillWidth: true
            text: route.summary || ""
            color: UiStyle.colorTextMuted
            font.family: UiStyle.fontSans
            font.pixelSize: UiStyle.fontSizeSm
            wrapMode: Text.WordWrap
            maximumLineCount: 2
            elide: Text.ElideRight
        }

        Text {
            Layout.fillWidth: true
            text: route.purpose || ""
            color: UiStyle.colorText
            font.family: UiStyle.fontSans
            font.pixelSize: UiStyle.fontSizeSm
            wrapMode: Text.WordWrap
            maximumLineCount: 2
            elide: Text.ElideRight
        }

        GridLayout {
            Layout.fillWidth: true
            columns: width > 540 ? 3 : 2
            columnSpacing: UiStyle.space6
            rowSpacing: UiStyle.space2

            Repeater {
                model: route.objects || []
                delegate: Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 18
                    color: UiStyle.colorTransparent
                    radius: UiStyle.radiusSm
                    Text {
                        anchors.fill: parent
                        anchors.leftMargin: UiStyle.space2
                        anchors.rightMargin: UiStyle.space2
                        text: modelData
                        color: UiStyle.colorTextMuted
                        font.family: UiStyle.fontSans
                        font.pixelSize: UiStyle.fontSizeXs
                        verticalAlignment: Text.AlignVCenter
                        elide: Text.ElideRight
                    }
                }
            }
        }
    }
}
