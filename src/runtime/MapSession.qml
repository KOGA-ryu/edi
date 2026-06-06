import QtQuick

QtObject {
    id: mapSession

    property string mapCsvPath: ""
    property string cellMetadataPath: ""
    property string mapCsvText: ""
    property string cellMetadataText: ""
    property var mapGrid: []
    property var cellMetadataByKey: ({})
    property int selectedMapRow: 0
    property int selectedMapCol: 0
    property var mapTokenPalette: ["wall", "floor", "door", "start", "secret", "encounter"]

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

    function cellKey(row, col) {
        return String(row) + ":" + String(col)
    }

    function parseCsvRow(line) {
        var parts = String(line || "").split(",")
        var row = []
        for (var index = 0; index < parts.length; ++index) {
            row.push(String(parts[index] || "").trim())
        }
        return row
    }

    function parseCsvGrid(text) {
        var rows = []
        var lines = String(text || "").split(/\r?\n/)
        for (var lineIndex = 0; lineIndex < lines.length; ++lineIndex) {
            var line = String(lines[lineIndex] || "").trim()
            if (line.length > 0) {
                rows.push(parseCsvRow(line))
            }
        }
        return rows
    }

    function parseCellMetadata(text) {
        var result = ({})
        var lines = String(text || "").split(/\r?\n/)
        for (var lineIndex = 0; lineIndex < lines.length; ++lineIndex) {
            var line = String(lines[lineIndex] || "").trim()
            if (!line.length) {
                continue
            }
            try {
                var item = JSON.parse(line)
                var row = Number(item.row)
                var col = Number(item.col)
                if (Number.isFinite(row) && Number.isFinite(col)) {
                    result[cellKey(row, col)] = item
                }
            } catch (error) {
            }
        }
        return result
    }

    function loadMapData(csvText, metadataText) {
        mapCsvText = String(csvText || "")
        cellMetadataText = String(metadataText || "")
        mapGrid = parseCsvGrid(mapCsvText)
        cellMetadataByKey = parseCellMetadata(cellMetadataText)

        if (mapGrid.length > 0 && mapGrid[0].length > 0) {
            var start = firstMapCellWithToken("start")
            selectedMapRow = start.row
            selectedMapCol = start.col
        } else {
            selectedMapRow = 0
            selectedMapCol = 0
        }
    }

    function firstMapCellWithToken(token) {
        for (var row = 0; row < mapGrid.length; ++row) {
            for (var col = 0; col < mapGrid[row].length; ++col) {
                if (mapTokenAt(row, col) === token) {
                    return { row: row, col: col }
                }
            }
        }
        return { row: 0, col: 0 }
    }

    function mapRowCount() {
        return mapGrid.length
    }

    function mapColCount() {
        var maxColumns = 0
        for (var row = 0; row < mapGrid.length; ++row) {
            maxColumns = Math.max(maxColumns, mapGrid[row].length)
        }
        return maxColumns
    }

    function mapTokenAt(row, col) {
        if (row < 0 || row >= mapGrid.length) {
            return ""
        }
        var gridRow = mapGrid[row] || []
        if (col < 0 || col >= gridRow.length) {
            return ""
        }
        return String(gridRow[col] || "")
    }

    function selectedMapToken() {
        return mapTokenAt(selectedMapRow, selectedMapCol)
    }

    function selectedMapMetadata() {
        return cellMetadataByKey[cellKey(selectedMapRow, selectedMapCol)] || ({})
    }

    function selectMapCell(row, col) {
        var requestedRow = Number(row)
        var requestedCol = Number(col)
        selectedMapRow = Math.max(0, Math.min(Number.isFinite(requestedRow) ? requestedRow : 0, Math.max(0, mapRowCount() - 1)))
        selectedMapCol = Math.max(0, Math.min(Number.isFinite(requestedCol) ? requestedCol : 0, Math.max(0, mapColCount() - 1)))
    }

    function mapTokenCounts() {
        var counts = ({})
        for (var row = 0; row < mapGrid.length; ++row) {
            for (var col = 0; col < mapGrid[row].length; ++col) {
                var token = mapTokenAt(row, col)
                counts[token] = Number(counts[token] || 0) + 1
            }
        }
        return counts
    }

    function mapPaletteRows() {
        var counts = mapTokenCounts()
        var rows = []
        for (var index = 0; index < mapTokenPalette.length; ++index) {
            var token = mapTokenPalette[index]
            rows.push({ label: token, meta: String(counts[token] || 0) })
        }
        return rows
    }

    function selectedCellIntent() {
        var metadata = selectedMapMetadata()
        return String(metadata.intent || defaultIntentForToken(selectedMapToken()))
    }

    function selectedCellTags() {
        var tags = selectedMapMetadata().tags || []
        if (!Array.isArray(tags)) {
            return []
        }
        return tags
    }

    function selectedCellStatus() {
        return String(selectedMapMetadata().status || "unreviewed")
    }

    function selectedCellCodeRefs() {
        return selectedMapMetadata().code_refs || selectedMapMetadata().codeRefs || []
    }

    function defaultIntentForToken(token) {
        if (token === "wall") return "boundary or blocker"
        if (token === "floor") return "walkable path"
        if (token === "door") return "transition or gate"
        if (token === "start") return "entry point"
        if (token === "secret") return "hidden optional branch"
        if (token === "encounter") return "combat, puzzle, or hazard beat"
        return "unclassified cell"
    }
}
