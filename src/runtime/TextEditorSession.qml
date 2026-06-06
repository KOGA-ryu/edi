import QtQuick

QtObject {
    id: textEditorSession

    property var textEditorDocuments: [
        {
            id: "scratch",
            name: "scratch.txt",
            language: "text",
            path: "docs/scratch.txt",
            initialText: "",
            text: "",
            cursorPosition: 0,
            selectionStart: 0,
            selectionEnd: 0
        }
    ]
    property string activeTextEditorDocumentId: "scratch"
    property int textEditorDocumentCounter: 1
    property string textEditorDocumentName: "scratch.txt"
    property string textEditorLanguage: "text"
    property string textEditorInitialText: ""
    property string textEditorText: ""
    property int textEditorCursorPosition: 0
    property int textEditorSelectionStart: 0
    property int textEditorSelectionEnd: 0
    property int textEditorPreviewLimit: 12000
    property bool textEditorModified: false
    property bool textEditorRenameActive: false
    property string textEditorCloseStatus: ""
    property bool textEditorWrapEnabled: true
    property bool textEditorLineNumbersVisible: true
    property string textEditorRequestedCommand: ""
    property int textEditorCommandRevision: 0
    property string textEditorStoragePath: ""
    property bool textEditorSaveOk: true
    property string textEditorSaveStatus: "not saved"

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

    function cloneTextEditorDocument(source) {
        return {
            id: String(source && source.id ? source.id : ""),
            name: String(source && source.name ? source.name : "untitled.txt"),
            language: String(source && source.language ? source.language : "text"),
            path: String(source && source.path ? source.path : ""),
            initialText: String(source && source.initialText ? source.initialText : ""),
            text: String(source && source.text ? source.text : ""),
            cursorPosition: Math.max(0, Number(source && source.cursorPosition) || 0),
            selectionStart: Math.max(0, Number(source && source.selectionStart) || 0),
            selectionEnd: Math.max(0, Number(source && source.selectionEnd) || 0)
        }
    }

    function updateTextEditorState(text, cursorPosition, selectionStart, selectionEnd) {
        textEditorText = String(text || "")
        textEditorCursorPosition = Math.max(0, Number(cursorPosition) || 0)
        textEditorSelectionStart = Math.max(0, Number(selectionStart) || 0)
        textEditorSelectionEnd = Math.max(0, Number(selectionEnd) || 0)
        textEditorModified = textEditorText !== textEditorInitialText
        textEditorCloseStatus = ""
        commitActiveTextEditorDocument()
    }

    function textEditorDocumentIndex(id) {
        var key = String(id || "")
        for (var index = 0; index < textEditorDocuments.length; ++index) {
            if (String(textEditorDocuments[index].id || "") === key) {
                return index
            }
        }
        return -1
    }

    function activeTextEditorDocument(unusedRevision) {
        var index = textEditorDocumentIndex(activeTextEditorDocumentId)
        if (index < 0 && textEditorDocuments.length > 0) {
            return cloneTextEditorDocument(textEditorDocuments[0])
        }
        if (index < 0) {
            return cloneTextEditorDocument({})
        }
        return cloneTextEditorDocument(textEditorDocuments[index])
    }

    function textEditorDocumentModified(document) {
        var item = document || ({})
        return String(item.text || "") !== String(item.initialText || "")
    }

    function textEditorDocumentState(document) {
        return textEditorDocumentModified(document) ? "modified" : "clean"
    }

    function commitActiveTextEditorDocument() {
        var index = textEditorDocumentIndex(activeTextEditorDocumentId)
        if (index < 0) {
            return
        }
        var copy = textEditorDocuments.slice()
        var current = cloneTextEditorDocument(copy[index])
        current.name = textEditorDocumentName
        current.language = textEditorLanguage
        current.path = current.path || textEditorPathForId(current.id)
        current.initialText = textEditorInitialText
        current.text = textEditorText
        current.cursorPosition = textEditorCursorPosition
        current.selectionStart = textEditorSelectionStart
        current.selectionEnd = textEditorSelectionEnd
        copy[index] = current
        textEditorDocuments = copy
        textEditorModified = textEditorDocumentModified(current)
    }

    function loadTextEditorDocument(document) {
        var item = cloneTextEditorDocument(document)
        activeTextEditorDocumentId = item.id
        textEditorDocumentName = item.name
        textEditorLanguage = item.language
        if (!item.path.length) {
            item.path = textEditorPathForId(item.id)
        }
        textEditorInitialText = item.initialText
        textEditorText = item.text
        textEditorCursorPosition = item.cursorPosition
        textEditorSelectionStart = item.selectionStart
        textEditorSelectionEnd = item.selectionEnd
        textEditorModified = textEditorDocumentModified(item)
    }

    function loadTextEditorDocuments(documents) {
        var sourceDocuments = asArray(documents)
        if (!sourceDocuments.length) {
            return
        }
        var normalized = []
        var highestCounter = textEditorDocumentCounter
        for (var index = 0; index < sourceDocuments.length; ++index) {
            var document = cloneTextEditorDocument(sourceDocuments[index])
            if (!document.id.length) {
                continue
            }
            if (!document.path.length) {
                document.path = textEditorPathForId(document.id)
            }
            normalized.push(document)
            var match = document.id.match(/(\d+)$/)
            if (match) {
                highestCounter = Math.max(highestCounter, Number(match[1]))
            }
        }
        if (!normalized.length) {
            return
        }
        textEditorDocuments = normalized
        textEditorDocumentCounter = Math.max(highestCounter, normalized.length)
        loadTextEditorDocument(normalized[0])
        textEditorSaveOk = true
        textEditorSaveStatus = "loaded"
    }

    function selectTextEditorDocument(id) {
        commitActiveTextEditorDocument()
        var index = textEditorDocumentIndex(id)
        if (index < 0) {
            return
        }
        textEditorRenameActive = false
        textEditorCloseStatus = ""
        loadTextEditorDocument(textEditorDocuments[index])
    }

    function nextTextEditorDocumentName(prefix) {
        textEditorDocumentCounter += 1
        return String(prefix || "untitled") + "-" + String(textEditorDocumentCounter) + ".txt"
    }

    function newTextEditorDocument() {
        commitActiveTextEditorDocument()
        var id = "draft-" + String(textEditorDocumentCounter + 1)
        var document = {
            id: id,
            name: nextTextEditorDocumentName("draft"),
            language: "text",
            path: textEditorPathForId(id),
            initialText: "",
            text: "",
            cursorPosition: 0,
            selectionStart: 0,
            selectionEnd: 0
        }
        textEditorDocuments = textEditorDocuments.concat([document])
        textEditorRenameActive = false
        textEditorCloseStatus = ""
        loadTextEditorDocument(document)
    }

    function duplicateTextEditorDocument() {
        commitActiveTextEditorDocument()
        var source = activeTextEditorDocument()
        var id = "draft-" + String(textEditorDocumentCounter + 1)
        var document = {
            id: id,
            name: nextTextEditorDocumentName("copy"),
            language: source.language,
            path: textEditorPathForId(id),
            initialText: source.text,
            text: source.text,
            cursorPosition: source.cursorPosition,
            selectionStart: source.selectionStart,
            selectionEnd: source.selectionEnd
        }
        textEditorDocuments = textEditorDocuments.concat([document])
        textEditorRenameActive = false
        textEditorCloseStatus = ""
        loadTextEditorDocument(document)
    }

    function renameActiveTextEditorDocument() {
        textEditorRenameActive = true
        textEditorCloseStatus = ""
    }

    function applyTextEditorRename(name) {
        commitActiveTextEditorDocument()
        var normalizedName = String(name || "").trim()
        if (!normalizedName.length) {
            normalizedName = textEditorDocumentName
        }
        textEditorDocumentName = normalizedName
        textEditorRenameActive = false
        textEditorCloseStatus = ""
        commitActiveTextEditorDocument()
    }

    function cancelTextEditorRename() {
        textEditorRenameActive = false
    }

    function closeActiveTextEditorDocument() {
        if (textEditorDocuments.length <= 1) {
            textEditorCloseStatus = "last document"
            return
        }
        var index = textEditorDocumentIndex(activeTextEditorDocumentId)
        if (index < 0) {
            return
        }
        if (textEditorModified) {
            textEditorCloseStatus = "close blocked: modified"
            return
        }
        var copy = textEditorDocuments.slice()
        copy.splice(index, 1)
        textEditorDocuments = copy
        var nextIndex = Math.max(0, Math.min(index, textEditorDocuments.length - 1))
        textEditorRenameActive = false
        textEditorCloseStatus = ""
        loadTextEditorDocument(textEditorDocuments[nextIndex])
    }

    function toggleTextEditorWrap() {
        textEditorWrapEnabled = !textEditorWrapEnabled
    }

    function toggleTextEditorLineNumbers() {
        textEditorLineNumbersVisible = !textEditorLineNumbersVisible
    }

    function requestTextEditorCommand(command) {
        textEditorRequestedCommand = String(command || "")
        textEditorCommandRevision += 1
    }

    function textEditorPathForId(id) {
        return "docs/" + String(id || "untitled").replace(/[^A-Za-z0-9_-]/g, "_") + ".txt"
    }

    function saveTextEditorDocument() {
        return saveTextEditorDocuments(false)
    }

    function saveAllTextEditorDocuments() {
        return saveTextEditorDocuments(true)
    }

    function saveTextEditorDocuments(saveAll) {
        commitActiveTextEditorDocument()
        if (typeof textEditorStore === "undefined" || !textEditorStore) {
            textEditorSaveOk = false
            textEditorSaveStatus = "storage unavailable"
            return false
        }
        var result = textEditorStore.save(textEditorDocuments, activeTextEditorDocumentId, !!saveAll)
        textEditorSaveOk = !!result.ok
        textEditorSaveStatus = String(result.message || (textEditorSaveOk ? "saved" : "save failed"))
        if (textEditorSaveOk) {
            var activeId = activeTextEditorDocumentId
            textEditorDocuments = asArray(result.documents)
            var activeIndex = textEditorDocumentIndex(activeId)
            if (activeIndex >= 0) {
                loadTextEditorDocument(textEditorDocuments[activeIndex])
            } else if (textEditorDocuments.length > 0) {
                loadTextEditorDocument(textEditorDocuments[0])
            }
        }
        return textEditorSaveOk
    }

    function textEditorLineCount(unusedRevision) {
        if (!textEditorText.length) {
            return 1
        }
        return textEditorText.split(/\n/).length
    }

    function textEditorCharCount(unusedRevision) {
        return textEditorText.length
    }

    function textEditorSelectionLength(unusedRevision) {
        return Math.abs(textEditorSelectionEnd - textEditorSelectionStart)
    }

    function textEditorCursorLine(unusedRevision) {
        var position = Math.max(0, Math.min(textEditorCursorPosition, textEditorText.length))
        if (position <= 0) {
            return 1
        }
        return textEditorText.slice(0, position).split(/\n/).length
    }

    function textEditorCursorColumn(unusedRevision) {
        var position = Math.max(0, Math.min(textEditorCursorPosition, textEditorText.length))
        var previousBreak = textEditorText.lastIndexOf("\n", position - 1)
        return position - previousBreak
    }

    function textEditorModifiedState(unusedRevision) {
        return textEditorModified ? "modified" : "clean"
    }
}
