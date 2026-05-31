import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../../style"
import "../../components"

ColumnLayout {
    id: root
    property var controller: null
    property string draftStatus: "pending"
    spacing: UiStyle.space8

    UiSectionHeader {
        title: "Human Notes"
        Layout.fillWidth: true
    }

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
        Layout.preferredHeight: 86
        placeholderText: "Attach a note to this exact route. This first pass is in-memory only."
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: UiStyle.space8

        ComboBox {
            id: statusBox
            Layout.preferredWidth: 160
            model: ["pending", "accepted", "needs_rework", "rejected"]
            currentIndex: 0
            onCurrentTextChanged: root.draftStatus = currentText
        }

        UiButton {
            label: "Add Note"
            selected: true
            implicitWidth: 110
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
