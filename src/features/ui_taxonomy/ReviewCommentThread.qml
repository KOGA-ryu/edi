import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../../style"
import "../../components"

ColumnLayout {
    id: root
    property var controller: null
    property string draftStatus: "pending"
    spacing: UiStyle.space4

    Repeater {
        model: root.controller ? root.controller.routeNotes(root.controller.selectedRouteId, root.controller.revision) : []
        delegate: UiNoteCard {
            Layout.fillWidth: true
            status: modelData.status
            body: modelData.body
            createdAt: modelData.createdAt
        }
    }

    UiTextArea {
        id: noteText
        Layout.fillWidth: true
        Layout.preferredHeight: 62
        fillColor: UiStyle.colorSurface
        placeholderText: "Attach a note to this exact route. This first pass is in-memory only."
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: UiStyle.space4

        ComboBox {
            id: statusBox
            Layout.preferredWidth: 132
            Layout.preferredHeight: 24
            model: ["pending", "accepted", "needs_rework", "rejected"]
            currentIndex: 0
            onCurrentTextChanged: root.draftStatus = currentText
            font.family: UiStyle.fontSans
            font.pixelSize: UiStyle.fontSizeXs
            background: Rectangle {
                color: UiStyle.colorControl
                radius: UiStyle.radiusSm
                border.width: UiStyle.borderNone
            }
            contentItem: Text {
                leftPadding: UiStyle.space8
                rightPadding: UiStyle.space20
                text: statusBox.displayText
                color: UiStyle.colorTextMuted
                font.family: UiStyle.fontSans
                font.pixelSize: UiStyle.fontSizeXs
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
            }
            indicator: Text {
                anchors.right: parent.right
                anchors.rightMargin: UiStyle.space6
                anchors.verticalCenter: parent.verticalCenter
                text: "v"
                color: UiStyle.colorTextFaint
                font.family: UiStyle.fontSans
                font.pixelSize: UiStyle.fontSizeXs
            }
        }

        UiButton {
            label: "Add Note"
            selected: true
            implicitWidth: 84
            implicitHeight: 24
            onClicked: {
                if (root.controller) {
                    root.controller.addNote(root.controller.selectedRouteId, root.draftStatus, noteText.text)
                    noteText.text = ""
                }
            }
        }

        Text {
            Layout.fillWidth: true
            text: root.controller && root.controller.writeDisabled ? "writes disabled" : "writes enabled"
            color: UiStyle.colorTextFaint
            font.family: UiStyle.fontSans
            font.pixelSize: UiStyle.fontSizeXs
            horizontalAlignment: Text.AlignRight
        }
    }
}
