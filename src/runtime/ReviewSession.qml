import QtQuick

QtObject {
    property string subjectId: ""
    property string currentRouteId: ""
    property var notes: []
    property var statusOverrides: ({})
    property bool writeDisabled: true
}
