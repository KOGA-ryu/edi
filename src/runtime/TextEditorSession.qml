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
            pinned: false,
            role: "prompt",
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
    property bool textEditorSplitEnabled: false
    property bool textEditorStatsEnabled: false
    property string secondaryTextEditorDocumentId: ""
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
            pinned: !!(source && source.pinned),
            role: normalizedTextEditorDocumentRole(source && source.role),
            cursorPosition: Math.max(0, Number(source && source.cursorPosition) || 0),
            selectionStart: Math.max(0, Number(source && source.selectionStart) || 0),
            selectionEnd: Math.max(0, Number(source && source.selectionEnd) || 0)
        }
    }

    function normalizedTextEditorDocumentRole(role) {
        var text = String(role || "").toLowerCase().trim()
        if (text === "prompt" || text === "context" || text === "reference" || text === "scratch" || text === "output") {
            return text
        }
        return ""
    }

    function defaultTextEditorDocumentRole(index) {
        return index === 0 ? "prompt" : "context"
    }

    function nextTextEditorDocumentRole(role) {
        var roles = ["prompt", "context", "reference", "scratch", "output"]
        var current = normalizedTextEditorDocumentRole(role)
        var index = roles.indexOf(current)
        return roles[(index + 1) % roles.length]
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

    function textEditorOrderedDocuments(unusedRevision) {
        var pinned = []
        var unpinned = []
        for (var index = 0; index < textEditorDocuments.length; ++index) {
            var document = cloneTextEditorDocument(textEditorDocuments[index])
            if (document.pinned) {
                pinned.push(document)
            } else {
                unpinned.push(document)
            }
        }
        return pinned.concat(unpinned)
    }

    function isTextEditorDocumentPinned(id) {
        var index = textEditorDocumentIndex(id)
        if (index < 0) {
            return false
        }
        return !!textEditorDocuments[index].pinned
    }

    function activeTextEditorDocumentPinned() {
        return isTextEditorDocumentPinned(activeTextEditorDocumentId)
    }

    function activeTextEditorDocumentRole() {
        var index = textEditorDocumentIndex(activeTextEditorDocumentId)
        if (index < 0) {
            return ""
        }
        return normalizedTextEditorDocumentRole(textEditorDocuments[index].role) || defaultTextEditorDocumentRole(index)
    }

    function toggleActiveTextEditorDocumentPin() {
        var index = textEditorDocumentIndex(activeTextEditorDocumentId)
        if (index < 0) {
            return
        }
        var copy = textEditorDocuments.slice()
        var current = cloneTextEditorDocument(copy[index])
        current.pinned = !current.pinned
        copy[index] = current
        textEditorDocuments = copy
    }

    function cycleActiveTextEditorDocumentRole() {
        var index = textEditorDocumentIndex(activeTextEditorDocumentId)
        if (index < 0) {
            return
        }
        var copy = textEditorDocuments.slice()
        var current = cloneTextEditorDocument(copy[index])
        current.role = nextTextEditorDocumentRole(current.role)
        copy[index] = current
        textEditorDocuments = copy
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
        current.role = normalizedTextEditorDocumentRole(current.role) || defaultTextEditorDocumentRole(index)
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
            if (!document.role.length) {
                document.role = defaultTextEditorDocumentRole(normalized.length)
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
        ensureSecondaryTextEditorDocument()
        textEditorSaveOk = true
        textEditorSaveStatus = "loaded"
    }

    function loadTextEditorSessionState(state) {
        var sourceState = state || ({})
        if (!sourceState) {
            sourceState = ({})
        }

        if (typeof sourceState.wrap_enabled === "boolean") {
            textEditorWrapEnabled = sourceState.wrap_enabled
        } else if (typeof sourceState.wrapEnabled === "boolean") {
            textEditorWrapEnabled = sourceState.wrapEnabled
        }

        if (typeof sourceState.line_numbers_visible === "boolean") {
            textEditorLineNumbersVisible = sourceState.line_numbers_visible
        } else if (typeof sourceState.lineNumbersVisible === "boolean") {
            textEditorLineNumbersVisible = sourceState.lineNumbersVisible
        }

        if (typeof sourceState.split_enabled === "boolean") {
            textEditorSplitEnabled = sourceState.split_enabled
        } else if (typeof sourceState.splitEnabled === "boolean") {
            textEditorSplitEnabled = sourceState.splitEnabled
        }

        var requestedActiveId = String(sourceState.active_document_id || sourceState.activeDocumentId || "").trim()
        if (!requestedActiveId.length && !textEditorDocuments.length) {
            return
        }

        if (!requestedActiveId.length) {
            requestedActiveId = textEditorDocuments.length ? textEditorDocuments[0].id : ""
        }

        var activeIndex = textEditorDocumentIndex(requestedActiveId)
        if (activeIndex < 0 && textEditorDocuments.length) {
            activeIndex = 0
            requestedActiveId = textEditorDocuments[0].id
        }

        if (activeIndex >= 0) {
            activeTextEditorDocumentId = requestedActiveId
            loadTextEditorDocument(textEditorDocuments[activeIndex])
        } else if (textEditorDocuments.length > 0) {
            activeTextEditorDocumentId = textEditorDocuments[0].id
            loadTextEditorDocument(textEditorDocuments[0])
        }

        var requestedSecondaryId = String(sourceState.secondary_document_id || sourceState.secondaryDocumentId || "").trim()
        if (requestedSecondaryId.length && textEditorDocumentIndex(requestedSecondaryId) >= 0) {
            secondaryTextEditorDocumentId = requestedSecondaryId
        } else if (!textEditorSplitEnabled) {
            secondaryTextEditorDocumentId = ""
        }

        if (textEditorSplitEnabled
                && (textEditorDocumentIndex(secondaryTextEditorDocumentId) < 0
                    || secondaryTextEditorDocumentId === activeTextEditorDocumentId)) {
            ensureSecondaryTextEditorDocument()
        }
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
        ensureSecondaryTextEditorDocument()
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
            pinned: false,
            role: "context",
            cursorPosition: 0,
            selectionStart: 0,
            selectionEnd: 0
        }
        textEditorDocuments = textEditorDocuments.concat([document])
        textEditorRenameActive = false
        textEditorCloseStatus = ""
        loadTextEditorDocument(document)
        ensureSecondaryTextEditorDocument()
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
            pinned: false,
            role: source.role,
            cursorPosition: source.cursorPosition,
            selectionStart: source.selectionStart,
            selectionEnd: source.selectionEnd
        }
        textEditorDocuments = textEditorDocuments.concat([document])
        textEditorRenameActive = false
        textEditorCloseStatus = ""
        loadTextEditorDocument(document)
        ensureSecondaryTextEditorDocument()
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
        ensureSecondaryTextEditorDocument()
    }

    function toggleTextEditorWrap() {
        textEditorWrapEnabled = !textEditorWrapEnabled
    }

    function toggleTextEditorLineNumbers() {
        textEditorLineNumbersVisible = !textEditorLineNumbersVisible
    }

    function toggleTextEditorSplit() {
        textEditorSplitEnabled = !textEditorSplitEnabled
        ensureSecondaryTextEditorDocument()
    }

    function ensureSecondaryTextEditorDocument() {
        if (!textEditorDocuments.length) {
            secondaryTextEditorDocumentId = ""
            return
        }
        if (textEditorDocumentIndex(secondaryTextEditorDocumentId) >= 0
                && (secondaryTextEditorDocumentId !== activeTextEditorDocumentId
                    || textEditorDocuments.length === 1)) {
            return
        }
        for (var index = 0; index < textEditorDocuments.length; ++index) {
            if (textEditorDocuments[index].id !== activeTextEditorDocumentId) {
                secondaryTextEditorDocumentId = textEditorDocuments[index].id
                return
            }
        }
        secondaryTextEditorDocumentId = textEditorDocuments[0].id
    }

    function selectSecondaryTextEditorDocument(id) {
        var index = textEditorDocumentIndex(id)
        if (index >= 0) {
            secondaryTextEditorDocumentId = textEditorDocuments[index].id
        }
    }

    function secondaryTextEditorDocument(unusedRevision) {
        ensureSecondaryTextEditorDocument()
        var index = textEditorDocumentIndex(secondaryTextEditorDocumentId)
        if (index < 0) {
            return cloneTextEditorDocument({})
        }
        return cloneTextEditorDocument(textEditorDocuments[index])
    }

    function secondaryTextEditorText(unusedRevision) {
        return secondaryTextEditorDocument(unusedRevision).text
    }

    function updateSecondaryTextEditorState(text, cursorPosition, selectionStart, selectionEnd) {
        ensureSecondaryTextEditorDocument()
        if (secondaryTextEditorDocumentId === activeTextEditorDocumentId) {
            updateTextEditorState(text, cursorPosition, selectionStart, selectionEnd)
            return
        }

        var index = textEditorDocumentIndex(secondaryTextEditorDocumentId)
        if (index < 0) {
            return
        }
        var copy = textEditorDocuments.slice()
        var current = cloneTextEditorDocument(copy[index])
        current.text = String(text || "")
        current.cursorPosition = Math.max(0, Number(cursorPosition) || 0)
        current.selectionStart = Math.max(0, Number(selectionStart) || 0)
        current.selectionEnd = Math.max(0, Number(selectionEnd) || 0)
        copy[index] = current
        textEditorDocuments = copy
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

    function exportTextEditorBundle(metadata) {
        commitActiveTextEditorDocument()
        if (typeof textEditorStore === "undefined" || !textEditorStore
                || typeof textEditorStore.exportBundle !== "function") {
            textEditorSaveOk = false
            textEditorSaveStatus = "export unavailable"
            return ({ ok: false, message: textEditorSaveStatus, path: "" })
        }

        var result = textEditorStore.exportBundle(textEditorDocuments, activeTextEditorDocumentId, metadata || ({}))
        textEditorSaveOk = !!result.ok
        textEditorSaveStatus = String(result.message || (textEditorSaveOk ? "exported" : "export failed"))
        return result
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

    function textEditorWordCount(unusedRevision) {
        var content = String(textEditorText || "").trim()
        if (!content.length) {
            return 0
        }
        var matches = content.match(/[^\s]+/g)
        return matches ? matches.length : 0
    }

    function textEditorReadTime(unusedRevision) {
        var words = textEditorWordCount(unusedRevision)
        if (words <= 0) {
            return "0m"
        }
        return String(Math.max(1, Math.ceil(words / 250))) + "m"
    }

    function toggleTextEditorStats() {
        textEditorStatsEnabled = !textEditorStatsEnabled
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
