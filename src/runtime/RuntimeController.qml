import QtQuick
import "../style"

QtObject {
    id: runtimeController

    property var targetRoot: null
    property string activityMode: "blank"
    property bool hasReviewSubject: false
    property string selectedSubjectId: ""
    property string selectedSubjectLabel: ""
    property string selectedRouteId: ""
    property string selectedLocalTab: "overview"
    property string selectedShelfTab: ""
    property string selectedSettingsPage: "theme"
    property bool writeDisabled: true
    property string projectProfilePath: ""
    property var projectProfileDocument: ({})
    property string projectId: "draftsman_blank"
    property string projectTitle: "Draftsman"
    property string projectType: "blank_shell"
    property string projectRootPath: ""
    property string projectSummary: "Reusable blank Draftsman shell."
    property string settingsNavLabel: "Theme and layout"
    property string mainWorkspaceFeature: "blank_canvas"
    property string rightInspectorSource: "none"
    property var customActions: []
    property string customActionStatus: ""
    property string customActionOutputPath: ""
    property var leftProjectRows: []
    property var projectPanelDefaults: ({})
    property MapSession mapSession: MapSession { id: mapSession }
    property alias mapCsvPath: mapSession.mapCsvPath
    property alias cellMetadataPath: mapSession.cellMetadataPath
    property alias mapCsvText: mapSession.mapCsvText
    property alias cellMetadataText: mapSession.cellMetadataText
    property alias mapGrid: mapSession.mapGrid
    property alias cellMetadataByKey: mapSession.cellMetadataByKey
    property alias selectedMapRow: mapSession.selectedMapRow
    property alias selectedMapCol: mapSession.selectedMapCol
    property alias mapTokenPalette: mapSession.mapTokenPalette
    property DrawingSession drawingSession: DrawingSession {
        id: drawingSession
        writeDisabled: runtimeController.writeDisabled
        onChanged: runtimeController.handleDrawingSessionChanged()
    }
    property alias selectedDrawingToolId: drawingSession.selectedDrawingToolId
    property alias selectedDrawingVariantId: drawingSession.selectedDrawingVariantId
    property alias selectedDrawingExternalToolId: drawingSession.selectedDrawingExternalToolId
    property alias selectedDrawingLayerId: drawingSession.selectedDrawingLayerId
    property alias selectedDrawingObjectId: drawingSession.selectedDrawingObjectId
    property alias selectedDrawingObjectIds: drawingSession.selectedDrawingObjectIds
    property alias selectedDrawingPresetId: drawingSession.selectedDrawingPresetId
    property alias drawingToolPaletteOpen: drawingSession.drawingToolPaletteOpen
    property alias drawingToolPaletteX: drawingSession.drawingToolPaletteX
    property alias drawingToolPaletteY: drawingSession.drawingToolPaletteY
    property alias drawingToolModes: drawingSession.drawingToolModes
    property alias drawingToolSettingsById: drawingSession.drawingToolSettingsById
    property alias drawingPrecisionTools: drawingSession.drawingPrecisionTools
    property alias drawingDataTools: drawingSession.drawingDataTools
    property alias drawingImageTools: drawingSession.drawingImageTools
    property alias drawingExternalToolSettingsById: drawingSession.drawingExternalToolSettingsById
    property alias drawingAssetSources: drawingSession.drawingAssetSources
    property alias drawingPatternFamilies: drawingSession.drawingPatternFamilies
    property alias drawingToolPresets: drawingSession.drawingToolPresets
    property alias drawingLayerStack: drawingSession.drawingLayerStack
    property alias drawingSidebarSections: drawingSession.drawingSidebarSections
    property alias drawingNativeController: drawingSession.drawingNativeController
    property alias drawingExternalModelDocument: drawingSession.drawingExternalModelDocument
    property alias drawingGeneratedObjects: drawingSession.drawingGeneratedObjects
    property alias drawingPendingPoint: drawingSession.drawingPendingPoint
    property alias drawingPendingShapeActive: drawingSession.drawingPendingShapeActive
    property alias drawingSnapGridEnabled: drawingSession.drawingSnapGridEnabled
    property alias drawingSnapGridStepPx: drawingSession.drawingSnapGridStepPx
    property alias drawingObjectSnapEnabled: drawingSession.drawingObjectSnapEnabled
    property alias drawingObjectSnapTolerancePx: drawingSession.drawingObjectSnapTolerancePx
    property alias drawingObjectSnapEndpointEnabled: drawingSession.drawingObjectSnapEndpointEnabled
    property alias drawingObjectSnapMidpointEnabled: drawingSession.drawingObjectSnapMidpointEnabled
    property alias drawingObjectSnapCenterEnabled: drawingSession.drawingObjectSnapCenterEnabled
    property alias drawingObjectSnapVertexEnabled: drawingSession.drawingObjectSnapVertexEnabled
    property alias drawingCircleArcMode: drawingSession.drawingCircleArcMode
    property alias drawingLineVariant: drawingSession.drawingLineVariant
    property alias drawingStrokeColor: drawingSession.drawingStrokeColor
    property alias drawingFillColor: drawingSession.drawingFillColor
    property alias drawingLineThickness: drawingSession.drawingLineThickness
    property alias drawingLineStyle: drawingSession.drawingLineStyle
    property alias drawingStrokeOpacity: drawingSession.drawingStrokeOpacity
    property alias drawingCircleArcStartAngleDeg: drawingSession.drawingCircleArcStartAngleDeg
    property alias drawingCircleArcEndAngleDeg: drawingSession.drawingCircleArcEndAngleDeg
    property alias drawingRegularPolygonSides: drawingSession.drawingRegularPolygonSides
    property alias drawingRegularPolygonRotationDeg: drawingSession.drawingRegularPolygonRotationDeg
    property alias drawingGridVisible: drawingSession.drawingGridVisible
    property alias drawingGridMode: drawingSession.drawingGridMode
    property alias drawingGridDivisions: drawingSession.drawingGridDivisions
    property alias drawingGridMajorEvery: drawingSession.drawingGridMajorEvery
    property alias drawingAsciiCellGridVisible: drawingSession.drawingAsciiCellGridVisible
    property alias drawingAsciiColumns: drawingSession.drawingAsciiColumns
    property alias drawingAsciiRows: drawingSession.drawingAsciiRows
    property alias drawingAsciiMajorEvery: drawingSession.drawingAsciiMajorEvery
    property alias drawingCenterAxesVisible: drawingSession.drawingCenterAxesVisible
    property alias drawingDiagonalGuidesVisible: drawingSession.drawingDiagonalGuidesVisible
    property alias drawingRadialGuidesVisible: drawingSession.drawingRadialGuidesVisible
    property alias drawingRadialGuideCount: drawingSession.drawingRadialGuideCount
    property alias drawingArtboardBorderVisible: drawingSession.drawingArtboardBorderVisible
    property alias drawingCanvasSizePx: drawingSession.drawingCanvasSizePx
    property alias drawingCanvasZoom: drawingSession.drawingCanvasZoom
    property alias drawingCanvasZoomMin: drawingSession.drawingCanvasZoomMin
    property alias drawingCanvasZoomMax: drawingSession.drawingCanvasZoomMax
    property alias drawingCanvasPanXPx: drawingSession.drawingCanvasPanXPx
    property alias drawingCanvasPanYPx: drawingSession.drawingCanvasPanYPx
    property alias drawingCanUndoCommand: drawingSession.drawingCanUndoCommand
    property alias drawingCanRedoCommand: drawingSession.drawingCanRedoCommand
    property alias drawingLastScriptId: drawingSession.drawingLastScriptId
    property alias drawingLastScriptStatus: drawingSession.drawingLastScriptStatus
    property alias drawingLastScriptErrors: drawingSession.drawingLastScriptErrors
    property TextEditorSession textEditorSession: TextEditorSession { id: textEditorSession }
    property alias textEditorDocuments: textEditorSession.textEditorDocuments
    property alias activeTextEditorDocumentId: textEditorSession.activeTextEditorDocumentId
    property alias textEditorDocumentCounter: textEditorSession.textEditorDocumentCounter
    property alias textEditorDocumentName: textEditorSession.textEditorDocumentName
    property alias textEditorLanguage: textEditorSession.textEditorLanguage
    property alias textEditorInitialText: textEditorSession.textEditorInitialText
    property alias textEditorText: textEditorSession.textEditorText
    property alias textEditorCursorPosition: textEditorSession.textEditorCursorPosition
    property alias textEditorSelectionStart: textEditorSession.textEditorSelectionStart
    property alias textEditorSelectionEnd: textEditorSession.textEditorSelectionEnd
    property alias textEditorPreviewLimit: textEditorSession.textEditorPreviewLimit
    property alias textEditorModified: textEditorSession.textEditorModified
    property alias textEditorRenameActive: textEditorSession.textEditorRenameActive
    property alias textEditorCloseStatus: textEditorSession.textEditorCloseStatus
    property alias textEditorWrapEnabled: textEditorSession.textEditorWrapEnabled
    property alias textEditorLineNumbersVisible: textEditorSession.textEditorLineNumbersVisible
    property alias textEditorSplitEnabled: textEditorSession.textEditorSplitEnabled
    property alias textEditorStatsEnabled: textEditorSession.textEditorStatsEnabled
    property alias secondaryTextEditorDocumentId: textEditorSession.secondaryTextEditorDocumentId
    property alias textEditorRequestedCommand: textEditorSession.textEditorRequestedCommand
    property alias textEditorCommandRevision: textEditorSession.textEditorCommandRevision
    property alias textEditorStoragePath: textEditorSession.textEditorStoragePath
    property alias textEditorSaveOk: textEditorSession.textEditorSaveOk
    property alias textEditorSaveStatus: textEditorSession.textEditorSaveStatus
    property bool leftPanelCollapsed: false
    property bool rightPanelCollapsed: true
    property bool bottomPanelCollapsed: true
    property int leftPanelWidth: UiStyle.leftPanelWidth
    property int rightPanelWidth: UiStyle.rightPanelWidth
    property int bottomPanelHeight: UiStyle.bottomPanelHeight
    property int leftPanelMinWidth: 180
    property int leftPanelMaxWidth: 520
    property int rightPanelMinWidth: 160
    property int rightPanelMaxWidth: 2400
    property int bottomPanelMinHeight: 96
    property int bottomPanelMaxHeight: 360
    property int leftPanelAutoHideWidth: 640
    property int rightPanelAutoHideWidth: 0
    property int bottomPanelAutoHideHeight: 520
    property var rightInspectorSections: ({
        facts: true,
        selection: true,
        code_refs: true,
        notes: true,
        receipts: true,
        actions: true
    })
    property bool shellLayoutDirty: false
    property bool shellLayoutSaveOk: true
    property string shellLayoutPath: ""
    property bool drawingDocumentIoOk: true
    property string drawingDocumentIoStatus: "not saved"
    property string drawingDocumentPath: ""
    property bool drawingDocumentDirty: false
    property string drawingDocumentCleanSnapshot: ""
    property int revision: 0

    property var backStack: []
    property var forwardStack: []
    property var statusOverrides: ({})
    property var notes: []
    property var reviewSubjectDocument: ({})
    property var uiThemeDocument: ({})
    property string uiThemePath: ""

    property var activityModes: [
        { id: "blank", label: "Blank", icon: "B", tooltip: "Blank workspace" },
        { id: "settings", label: "Settings", icon: "S", tooltip: "Settings surface" }
    ]

    readonly property var localTabs: [
        { id: "overview", label: "Overview", tooltip: "Summary of the selected review route, including purpose, status counts, and nearby route cards." },
        { id: "objects", label: "Objects", tooltip: "Review the child routes, UI regions, panels, and objects attached to the selected route." },
        { id: "code", label: "Code", tooltip: "Show the source references and implementation files tied to the selected route." },
        { id: "prompts", label: "Prompts", tooltip: "Show the human and agent prompts that define what should be reviewed or changed." },
        { id: "notes", label: "Notes", tooltip: "Show review notes, decisions, and comments attached to the selected route." }
    ]
    property var shelfTabs: []

    property var routes: []
    property string rootRouteId: ""

    Component.onCompleted: {
        projectProfilePath = typeof initialProjectProfilePath === "undefined" ? "" : String(initialProjectProfilePath)
        projectProfileDocument = typeof initialProjectProfile === "undefined" ? ({}) : initialProjectProfile
        loadProjectProfile(projectProfileDocument)

        uiThemeDocument = typeof initialUiTheme === "undefined" ? ({}) : initialUiTheme
        uiThemePath = typeof initialUiThemePath === "undefined" ? "" : String(initialUiThemePath)
        UiStyle.applyTheme(uiThemeDocument)
        shellLayoutPath = typeof initialShellLayoutPath === "undefined" ? "" : String(initialShellLayoutPath)
        loadShellLayout(typeof initialShellLayout === "undefined" ? ({}) : initialShellLayout)
        applyProjectPanelDefaults()

        var document = typeof initialReviewSubject === "undefined" ? ({}) : initialReviewSubject
        reviewSubjectDocument = document
        loadReviewSubject(reviewSubjectDocument)

        mapSession.mapCsvPath = typeof initialMapCsvPath === "undefined" ? "" : String(initialMapCsvPath)
        mapSession.mapCsvText = typeof initialMapCsvText === "undefined" ? "" : String(initialMapCsvText)
        mapSession.cellMetadataPath = typeof initialCellMetadataPath === "undefined" ? "" : String(initialCellMetadataPath)
        mapSession.cellMetadataText = typeof initialCellMetadataText === "undefined" ? "" : String(initialCellMetadataText)
        loadMapData(mapSession.mapCsvText, mapSession.cellMetadataText)

        textEditorSession.textEditorStoragePath = typeof initialTextEditorManifestPath === "undefined" ? "" : String(initialTextEditorManifestPath)
        textEditorSession.loadTextEditorDocuments(typeof initialTextEditorDocuments === "undefined" ? [] : initialTextEditorDocuments)
        textEditorSession.loadTextEditorSessionState(typeof initialTextEditorState === "undefined" ? ({}) : initialTextEditorState)

        drawingNativeController = typeof nativeDrawingController === "undefined" ? null : nativeDrawingController
        drawingSession.loadDrawingToolRegistry(
            typeof initialDrawingToolRegistry === "undefined" ? ({}) : initialDrawingToolRegistry,
            typeof initialDrawingToolRegistryPath === "undefined" ? "" : String(initialDrawingToolRegistryPath))
        drawingExternalModelDocument = drawingNativeController ? drawingNativeController.modelDocument() : (typeof initialDrawingModel === "undefined" ? ({}) : initialDrawingModel)
        drawingSession.loadInitialDrawingModel(drawingExternalModelDocument)
        markDrawingDocumentClean("not saved")
    }

    function clamp(value, low, high) {
        return Math.max(low, Math.min(high, Math.round(Number(value))))
    }

    function policyInt(source, key, fallback, low, high) {
        var value = source && Number.isFinite(Number(source[key])) ? Number(source[key]) : fallback
        return clamp(value, low, high)
    }

    function markShellLayoutDirty() {
        shellLayoutDirty = true
        revision += 1
    }

    function loadShellLayout(document) {
        var policy = document && document.policy ? document.policy : ({})
        var leftPolicy = policy.left || ({})
        var rightPolicy = policy.right || ({})
        var bottomPolicy = policy.bottom || ({})
        var rightPanel = document && document.right_panel ? document.right_panel : ({})
        var panels = document && document.panels ? document.panels : ({})
        var left = panels.left || ({})
        var right = panels.right || ({})
        var bottom = panels.bottom || ({})

        leftPanelMinWidth = policyInt(leftPolicy, "min_width", 180, 120, 900)
        leftPanelMaxWidth = policyInt(leftPolicy, "max_width", 520, leftPanelMinWidth, 1200)
        rightPanelMinWidth = policyInt(rightPolicy, "min_width", 160, 120, 900)
        rightPanelMaxWidth = policyInt(rightPolicy, "max_width", 2400, rightPanelMinWidth, 2400)
        bottomPanelMinHeight = policyInt(bottomPolicy, "min_height", 96, 60, 700)
        bottomPanelMaxHeight = policyInt(bottomPolicy, "max_height", 360, bottomPanelMinHeight, 1000)
        leftPanelAutoHideWidth = policyInt(leftPolicy, "auto_hide_below_width", 640, 0, 2400)
        rightPanelAutoHideWidth = policyInt(rightPolicy, "auto_hide_below_width", 0, 0, 2400)
        bottomPanelAutoHideHeight = policyInt(bottomPolicy, "auto_hide_below_height", 520, 0, 1800)
        leftPanelCollapsed = !!left.collapsed
        rightPanelCollapsed = typeof right.collapsed === "boolean" ? right.collapsed : true
        bottomPanelCollapsed = typeof bottom.collapsed === "boolean" ? bottom.collapsed : true
        rightInspectorSections = normalizedInspectorSections(rightPanel.sections)
        leftPanelWidth = clamp(left.width || UiStyle.leftPanelWidth, leftPanelMinWidth, leftPanelMaxWidth)
        rightPanelWidth = clamp(right.width || UiStyle.rightPanelWidth, rightPanelMinWidth, rightPanelMaxWidth)
        bottomPanelHeight = clamp(bottom.height || UiStyle.bottomPanelHeight, bottomPanelMinHeight, bottomPanelMaxHeight)
        shellLayoutDirty = false
        shellLayoutSaveOk = true
    }

    function shellLayoutDocument() {
        return {
            window: {
                width: Math.round(windowWidth()),
                height: Math.round(windowHeight())
            },
            policy: {
                left: {
                    min_width: leftPanelMinWidth,
                    max_width: leftPanelMaxWidth,
                    auto_hide_below_width: leftPanelAutoHideWidth
                },
                right: {
                    min_width: rightPanelMinWidth,
                    max_width: rightPanelMaxWidth,
                    auto_hide_below_width: rightPanelAutoHideWidth
                },
                bottom: {
                    min_height: bottomPanelMinHeight,
                    max_height: bottomPanelMaxHeight,
                    auto_hide_below_height: bottomPanelAutoHideHeight
                }
            },
            right_panel: {
                sections: rightInspectorSections
            },
            panels: {
                left: {
                    collapsed: leftPanelCollapsed,
                    width: leftPanelWidth
                },
                right: {
                    collapsed: rightPanelCollapsed,
                    width: rightPanelWidth
                },
                bottom: {
                    collapsed: bottomPanelCollapsed,
                    height: bottomPanelHeight
                }
            }
        }
    }

    function normalizedInspectorSections(source) {
        var defaults = defaultInspectorSections()
        if (!source) {
            return defaults
        }
        var result = Object.assign({}, defaults)
        var keys = Object.keys(defaults)
        for (var index = 0; index < keys.length; ++index) {
            var key = keys[index]
            if (typeof source[key] === "boolean") {
                result[key] = source[key]
            }
        }
        return result
    }

    function defaultInspectorSections() {
        var inspector = projectProfileDocument && projectProfileDocument.right_inspector ? projectProfileDocument.right_inspector : ({})
        var source = inspector.sections || ({})
        var result = {
            facts: true,
            selection: true,
            code_refs: true,
            notes: true,
            receipts: true,
            actions: true
        }
        var keys = Object.keys(result)
        for (var index = 0; index < keys.length; ++index) {
            var key = keys[index]
            if (typeof source[key] === "boolean") {
                result[key] = source[key]
            }
        }
        return result
    }

    function defaultActivityModes() {
        return [
            { id: "blank", label: "Blank", icon: "B", tooltip: "Blank workspace", exclusiveGroup: "" },
            { id: "review", label: "Review", icon: "R", tooltip: "Review gate workspace", exclusiveGroup: "" },
            { id: "settings", label: "Settings", icon: "S", tooltip: "Settings surface", exclusiveGroup: "system" },
            { id: "proof", label: "Proof", icon: "P", tooltip: "Proof and receipts", exclusiveGroup: "" }
        ]
    }

    function normalizeActivityModes(source) {
        var modes = []
        var sourceModes = asArray(source)
        for (var index = 0; index < sourceModes.length; ++index) {
            var mode = sourceModes[index]
            if (mode && mode.enabled !== false && String(mode.id || "").length > 0) {
                var id = String(mode.id)
                modes.push({
                    id: id,
                    label: String(mode.label || mode.id),
                    icon: String(mode.icon || String(mode.label || mode.id).charAt(0).toUpperCase()),
                    tooltip: String(mode.tooltip || mode.label || mode.id),
                    exclusiveGroup: String(mode.exclusive_group || mode.exclusiveGroup || defaultActivityExclusiveGroup(id))
                })
            }
        }
        return modes.length > 0 ? modes : defaultActivityModes()
    }

    function defaultActivityExclusiveGroup(modeId) {
        var id = String(modeId || "")
        if (id === "map_generator"
                || id === "map_editor"
                || id === "drawing_tool"
                || id === "drawing_drafting"
                || id === "blender_scripts"
                || id === "tool_workspace") {
            return "tool_type"
        }
        if (id === "settings") {
            return "system"
        }
        return ""
    }

    function activityModeRecord(modeId) {
        var id = String(modeId || "")
        for (var index = 0; index < activityModes.length; ++index) {
            if (String(activityModes[index].id || "") === id) {
                return activityModes[index]
            }
        }
        return null
    }

    function activityExclusiveGroup(modeId) {
        var mode = activityModeRecord(modeId)
        return mode ? String(mode.exclusiveGroup || "") : ""
    }

    function activityInExclusiveGroup(modeId, groupId) {
        return activityExclusiveGroup(modeId) === String(groupId || "")
    }

    function activeToolTypeMode() {
        return activityInExclusiveGroup(activityMode, "tool_type") ? activityMode : ""
    }

    function toolTypeModes(unusedRevision) {
        var result = []
        for (var index = 0; index < activityModes.length; ++index) {
            if (String(activityModes[index].exclusiveGroup || "") === "tool_type") {
                result.push(activityModes[index])
            }
        }
        return result
    }

    function setToolTypeMode(modeId) {
        if (!activityInExclusiveGroup(modeId, "tool_type")) {
            return
        }
        setActivityMode(modeId)
    }

    function normalizeProjectRows(source) {
        var rows = []
        var sourceRows = asArray(source)
        for (var index = 0; index < sourceRows.length; ++index) {
            var row = sourceRows[index]
            if (row && String(row.label || "").length > 0) {
                rows.push({
                    label: String(row.label),
                    meta: String(row.meta || "")
                })
            }
        }
        return rows.length > 0 ? rows : [
            { label: "Project slot", meta: "blank" },
            { label: "Scratch", meta: "workflow" },
            { label: "Final", meta: "workflow" }
        ]
    }

    function normalizeShelfTabs(source) {
        var result = []
        if (Array.isArray(source) && source.length === 0) {
            return result
        }
        var tabs = asArray(source)
        for (var index = 0; index < tabs.length; ++index) {
            var tab = String(tabs[index] || "").trim()
            if (tab.length > 0) {
                result.push(tab)
            }
        }
        return result.length > 0 ? result : ["Output", "Proof", "Receipts", "Log"]
    }

    function activityModeAvailable(modeId) {
        for (var index = 0; index < activityModes.length; ++index) {
            if (activityModes[index].id === modeId) {
                return true
            }
        }
        return false
    }

    function hasActivityMode(modeId) {
        return activityModeAvailable(modeId)
    }

    function loadProjectProfile(document) {
        var profile = document && document.profile ? document.profile : ({})
        var leftPanel = document && document.left_panel ? document.left_panel : ({})
        var mainWorkspace = document && document.main_workspace ? document.main_workspace : ({})
        var rightInspector = document && document.right_inspector ? document.right_inspector : ({})
        var bottomPanel = document && document.bottom_panel ? document.bottom_panel : ({})
        var panelDefaults = document && document.panel_defaults ? document.panel_defaults : ({})
        var writePolicy = document && document.write_policy ? document.write_policy : ({})

        projectId = String(profile.profile_id || "draftsman_blank")
        projectTitle = String(profile.label || "Draftsman")
        projectType = String(profile.type || "blank_shell")
        projectRootPath = String(profile.root_path || "")
        projectSummary = String(profile.summary || "Reusable blank Draftsman shell.")
        settingsNavLabel = String(leftPanel.settings_label || "Theme and layout")
        mainWorkspaceFeature = String(mainWorkspace.feature || "blank_canvas")
        rightInspectorSource = String(rightInspector.source || "none")
        projectPanelDefaults = panelDefaults
        writeDisabled = typeof writePolicy.writes_enabled === "boolean" ? !writePolicy.writes_enabled : true
        activityModes = normalizeActivityModes(document && document.activity_modes)
        leftProjectRows = normalizeProjectRows(leftPanel.project_rows)
        shelfTabs = normalizeShelfTabs(bottomPanel.tabs)
        customActions = normalizeCustomActions(document && document.custom_actions)
        selectedShelfTab = shelfTabs.length > 0 && shelfTabs.indexOf(selectedShelfTab) >= 0 ? selectedShelfTab : (shelfTabs[0] || "")
        rightInspectorSections = normalizedInspectorSections(rightInspector.sections)

        var defaultActivity = String(profile.default_activity || activityMode)
        activityMode = activityModeAvailable(defaultActivity) ? defaultActivity : activityModes[0].id
    }

    function normalizeCustomActions(source) {
        var actions = asArray(source)
        var result = []
        var seen = ({})
        for (var index = 0; index < actions.length; ++index) {
            var action = actions[index] || ({})
            var id = String(action.id || "").trim()
            var label = String(action.label || "").trim()
            var handler = String(action.handler || "").trim()
            if (!id.length || !label.length || !handler.length || seen[id]) {
                continue
            }
            seen[id] = true
            result.push({
                id: id,
                label: label,
                menu: String(action.menu || "Tools").trim() || "Tools",
                activity: String(action.activity || "").trim(),
                handler: handler,
                enabled: action.enabled !== false,
                args: action.args || ({})
            })
        }
        return result
    }

    function customActionVisible(action) {
        if (!action || action.enabled === false) {
            return false
        }
        if (action.activity && action.activity !== activityMode) {
            return false
        }
        return true
    }

    function menuCustomActions(menuName, unusedRevision) {
        var menu = String(menuName || "")
        var result = []
        for (var index = 0; index < customActions.length; ++index) {
            var action = customActions[index]
            if (String(action.menu || "") === menu && customActionVisible(action)) {
                result.push(action)
            }
        }
        return result
    }

    function customActionById(actionId) {
        var id = String(actionId || "")
        for (var index = 0; index < customActions.length; ++index) {
            if (customActions[index].id === id) {
                return customActions[index]
            }
        }
        return null
    }

    function runCustomAction(actionId) {
        var action = customActionById(actionId)
        if (!customActionVisible(action)) {
            customActionStatus = "action unavailable"
            customActionOutputPath = ""
            revision += 1
            return false
        }

        var handler = String(action.handler || "")
        var args = action.args || ({})
        if (handler === "text_editor_command") {
            requestTextEditorCommand(String(args.command || ""))
            customActionStatus = "ran " + action.label
            customActionOutputPath = ""
            revision += 1
            return true
        }
        if (handler === "switch_activity") {
            setActivityMode(String(args.activity || ""))
            customActionStatus = "ran " + action.label
            customActionOutputPath = ""
            revision += 1
            return true
        }
        if (handler === "export_text_bundle") {
            return exportTextEditorBundle(args)
        }

        customActionStatus = "unsupported action"
        customActionOutputPath = ""
        revision += 1
        return false
    }

    function applyProjectPanelDefaults() {
        if (!projectPanelDefaults) {
            return
        }
        if (typeof projectPanelDefaults.left_collapsed === "boolean") {
            leftPanelCollapsed = projectPanelDefaults.left_collapsed
        }
        if (typeof projectPanelDefaults.right_collapsed === "boolean") {
            rightPanelCollapsed = projectPanelDefaults.right_collapsed
        }
        if (typeof projectPanelDefaults.bottom_collapsed === "boolean") {
            bottomPanelCollapsed = projectPanelDefaults.bottom_collapsed
        }
    }

    function saveShellLayout() {
        if (typeof shellLayoutStore === "undefined" || !shellLayoutStore) {
            shellLayoutSaveOk = false
            return false
        }
        shellLayoutSaveOk = shellLayoutStore.save(shellLayoutDocument())
        if (shellLayoutSaveOk) {
            shellLayoutDirty = false
        }
        revision += 1
        return shellLayoutSaveOk
    }

    function resetShellLayout() {
        leftPanelCollapsed = false
        rightPanelCollapsed = true
        bottomPanelCollapsed = true
        resetPanelSizes()
        markShellLayoutDirty()
    }

    function resetPanelSizes() {
        leftPanelWidth = clamp(UiStyle.leftPanelWidth, leftPanelMinWidth, leftPanelMaxWidth)
        rightPanelWidth = clamp(UiStyle.rightPanelWidth, rightPanelMinWidth, rightPanelMaxWidth)
        bottomPanelHeight = clamp(UiStyle.bottomPanelHeight, bottomPanelMinHeight, bottomPanelMaxHeight)
    }

    function windowWidth() {
        return targetRoot ? Number(targetRoot.width) : UiStyle.windowWidth
    }

    function windowHeight() {
        return targetRoot ? Number(targetRoot.height) : UiStyle.windowHeight
    }

    function panelManualCollapsed(panelId) {
        if (panelId === "left") {
            return leftPanelCollapsed
        }
        if (panelId === "right") {
            return rightPanelCollapsed
        }
        if (panelId === "bottom") {
            return bottomPanelCollapsed
        }
        return false
    }

    function panelAutoHidden(panelId) {
        if (panelManualCollapsed(panelId)) {
            return false
        }
        if (panelId === "left") {
            return windowWidth() < leftPanelAutoHideWidth
        }
        if (panelId === "right") {
            return windowWidth() < rightPanelAutoHideWidth
        }
        if (panelId === "bottom") {
            return windowHeight() < bottomPanelAutoHideHeight
        }
        return false
    }

    function panelVisible(panelId) {
        return !panelManualCollapsed(panelId) && !panelAutoHidden(panelId)
    }

    function panelState(panelId) {
        if (panelManualCollapsed(panelId)) {
            return "collapsed"
        }
        if (panelAutoHidden(panelId)) {
            return "auto_hidden"
        }
        return "visible"
    }

    function panelStateLabel(panelId) {
        var state = panelState(panelId)
        if (state === "collapsed") {
            return "manual collapsed"
        }
        if (state === "auto_hidden") {
            return "auto-hidden"
        }
        return "visible"
    }

    function panelStateDetail(panelId) {
        if (panelId === "left") {
            return panelAutoHidden(panelId) ? "auto-hidden below " + String(leftPanelAutoHideWidth) + "px width" : String(leftPanelWidth) + " px"
        }
        if (panelId === "right") {
            return panelAutoHidden(panelId) ? "auto-hidden below " + String(rightPanelAutoHideWidth) + "px width" : String(rightPanelWidth) + " px"
        }
        if (panelId === "bottom") {
            return panelAutoHidden(panelId) ? "auto-hidden below " + String(bottomPanelAutoHideHeight) + "px height" : String(bottomPanelHeight) + " px"
        }
        return ""
    }

    function applyLayoutPreset(presetId) {
        if (presetId === "full") {
            leftPanelCollapsed = false
            rightPanelCollapsed = false
            bottomPanelCollapsed = false
            resetPanelSizes()
        } else if (presetId === "focus") {
            leftPanelCollapsed = true
            rightPanelCollapsed = true
            bottomPanelCollapsed = true
        } else if (presetId === "review") {
            leftPanelCollapsed = false
            rightPanelCollapsed = true
            bottomPanelCollapsed = true
            resetPanelSizes()
        } else if (presetId === "tiny") {
            leftPanelCollapsed = true
            rightPanelCollapsed = true
            bottomPanelCollapsed = true
            resetPanelSizes()
        }
        markShellLayoutDirty()
    }

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

    function loadMapData(csvText, metadataText) {
        mapSession.loadMapData(csvText, metadataText)
        revision += 1
    }

    function updateTextEditorState(text, cursorPosition, selectionStart, selectionEnd) {
        textEditorSession.updateTextEditorState(text, cursorPosition, selectionStart, selectionEnd)
        revision += 1
    }

    function persistTextEditorSessionState() {
        if (typeof textEditorStore === "undefined" || !textEditorStore || typeof textEditorStore.saveState !== "function") {
            return false
        }
        var editorState = {
            active_document_id: activeTextEditorDocumentId,
            split_enabled: textEditorSplitEnabled,
            secondary_document_id: secondaryTextEditorDocumentId,
            wrap_enabled: textEditorWrapEnabled,
            line_numbers_visible: textEditorLineNumbersVisible
        }
        return textEditorStore.saveState(editorState)
    }

    function textEditorDocumentIndex(id) {
        return textEditorSession.textEditorDocumentIndex(id)
    }

    function activeTextEditorDocument(unusedRevision) {
        return textEditorSession.activeTextEditorDocument(unusedRevision)
    }

    function textEditorDocumentModified(document) {
        return textEditorSession.textEditorDocumentModified(document)
    }

    function textEditorDocumentState(document) {
        return textEditorSession.textEditorDocumentState(document)
    }

    function textEditorOrderedDocuments(unusedRevision) {
        return textEditorSession.textEditorOrderedDocuments(unusedRevision)
    }

    function textEditorActiveDocumentPinned() {
        return textEditorSession.activeTextEditorDocumentPinned()
    }

    function textEditorActiveDocumentRole() {
        return textEditorSession.activeTextEditorDocumentRole()
    }

    function toggleTextEditorActiveDocumentPin() {
        textEditorSession.toggleActiveTextEditorDocumentPin()
        revision += 1
    }

    function cycleTextEditorActiveDocumentRole() {
        textEditorSession.cycleActiveTextEditorDocumentRole()
        revision += 1
    }

    function isTextEditorDocumentPinned(id) {
        return textEditorSession.isTextEditorDocumentPinned(id)
    }

    function commitActiveTextEditorDocument() {
        textEditorSession.commitActiveTextEditorDocument()
        revision += 1
    }

    function loadTextEditorDocument(document) {
        textEditorSession.loadTextEditorDocument(document)
        revision += 1
    }

    function loadTextEditorDocuments(documents) {
        textEditorSession.loadTextEditorDocuments(documents)
        revision += 1
    }

    function selectTextEditorDocument(id) {
        textEditorSession.selectTextEditorDocument(id)
        persistTextEditorSessionState()
        revision += 1
    }

    function nextTextEditorDocumentName(prefix) {
        return textEditorSession.nextTextEditorDocumentName(prefix)
    }

    function newTextEditorDocument() {
        textEditorSession.newTextEditorDocument()
        persistTextEditorSessionState()
        revision += 1
    }

    function duplicateTextEditorDocument() {
        textEditorSession.duplicateTextEditorDocument()
        persistTextEditorSessionState()
        revision += 1
    }

    function renameActiveTextEditorDocument() {
        textEditorSession.renameActiveTextEditorDocument()
        revision += 1
    }

    function applyTextEditorRename(name) {
        textEditorSession.applyTextEditorRename(name)
        revision += 1
    }

    function cancelTextEditorRename() {
        textEditorSession.cancelTextEditorRename()
        revision += 1
    }

    function closeActiveTextEditorDocument() {
        textEditorSession.closeActiveTextEditorDocument()
        persistTextEditorSessionState()
        revision += 1
    }

    function toggleTextEditorWrap() {
        textEditorSession.toggleTextEditorWrap()
        persistTextEditorSessionState()
        revision += 1
    }

    function toggleTextEditorLineNumbers() {
        textEditorSession.toggleTextEditorLineNumbers()
        persistTextEditorSessionState()
        revision += 1
    }

    function toggleTextEditorSplit() {
        textEditorSession.toggleTextEditorSplit()
        persistTextEditorSessionState()
        revision += 1
    }

    function toggleTextEditorStats() {
        textEditorSession.toggleTextEditorStats()
        revision += 1
    }

    function selectSecondaryTextEditorDocument(id) {
        textEditorSession.selectSecondaryTextEditorDocument(id)
        persistTextEditorSessionState()
        revision += 1
    }

    function secondaryTextEditorDocument(unusedRevision) {
        return textEditorSession.secondaryTextEditorDocument(unusedRevision)
    }

    function secondaryTextEditorText(unusedRevision) {
        return textEditorSession.secondaryTextEditorText(unusedRevision)
    }

    function updateSecondaryTextEditorState(text, cursorPosition, selectionStart, selectionEnd) {
        textEditorSession.updateSecondaryTextEditorState(text, cursorPosition, selectionStart, selectionEnd)
        revision += 1
    }

    function requestTextEditorCommand(command) {
        textEditorSession.requestTextEditorCommand(command)
        revision += 1
    }

    function textEditorPathForId(id) {
        return textEditorSession.textEditorPathForId(id)
    }

    function saveTextEditorDocument() {
        var result = textEditorSession.saveTextEditorDocument()
        revision += 1
        return result
    }

    function saveAllTextEditorDocuments() {
        var result = textEditorSession.saveAllTextEditorDocuments()
        revision += 1
        return result
    }

    function saveTextEditorDocuments(saveAll) {
        var result = textEditorSession.saveTextEditorDocuments(saveAll)
        revision += 1
        return result
    }

    function exportTextEditorBundle(args) {
        var source = args || ({})
        var metadata = Object.assign({}, source)
        metadata.profile_id = projectId
        metadata.profile_label = projectTitle
        if (!String(metadata.packet_type || "").length) {
            metadata.packet_type = "text_editor_bundle"
        }

        var result = textEditorSession.exportTextEditorBundle(metadata)
        customActionStatus = String(result.message || (result.ok ? "exported bundle" : "export failed"))
        customActionOutputPath = String(result.path || "")
        revision += 1
        return !!result.ok
    }

    function currentDrawingModelDocument() {
        if (drawingNativeController && typeof drawingNativeController.modelDocument === "function") {
            return drawingNativeController.modelDocument()
        }
        return drawingSession.drawingCanvasExportDocument(revision)
    }

    function drawingDocumentSnapshot() {
        return JSON.stringify(currentDrawingModelDocument())
    }

    function refreshDrawingDocumentDirty() {
        if (drawingDocumentCleanSnapshot.length === 0) {
            drawingDocumentDirty = false
            return
        }
        drawingDocumentDirty = drawingDocumentSnapshot() !== drawingDocumentCleanSnapshot
    }

    function markDrawingDocumentClean(status) {
        drawingDocumentCleanSnapshot = drawingDocumentSnapshot()
        drawingDocumentDirty = false
        drawingDocumentIoStatus = String(status || "saved")
    }

    function handleDrawingSessionChanged() {
        revision += 1
        refreshDrawingDocumentDirty()
    }

    function drawingDocumentFileName() {
        if (!drawingDocumentPath.length) {
            return "untitled"
        }
        var parts = drawingDocumentPath.split("/")
        return parts.length > 0 ? parts[parts.length - 1] : drawingDocumentPath
    }

    function drawingDocumentStatusText() {
        var state = drawingDocumentDirty ? "unsaved" : (drawingDocumentPath.length ? "saved" : "not saved")
        return drawingDocumentFileName() + " / " + state
    }

    function saveDrawingDocument(url) {
        if (typeof drawingDocumentStore === "undefined" || !drawingDocumentStore || typeof drawingDocumentStore.save !== "function") {
            drawingDocumentIoOk = false
            drawingDocumentIoStatus = "drawing storage unavailable"
            revision += 1
            return false
        }
        var result = drawingDocumentStore.save(url, currentDrawingModelDocument())
        drawingDocumentIoOk = !!result.ok
        drawingDocumentIoStatus = String(result.message || (drawingDocumentIoOk ? "saved drawing" : "save failed"))
        if (drawingDocumentIoOk) {
            drawingDocumentPath = String(result.path || "")
            markDrawingDocumentClean("saved drawing")
        }
        revision += 1
        return drawingDocumentIoOk
    }

    function saveCurrentDrawingDocument() {
        if (!drawingDocumentPath.length) {
            drawingDocumentIoOk = false
            drawingDocumentIoStatus = "drawing path missing"
            revision += 1
            return false
        }
        return saveDrawingDocument(drawingDocumentPath)
    }

    function openDrawingDocument(url) {
        if (typeof drawingDocumentStore === "undefined" || !drawingDocumentStore || typeof drawingDocumentStore.open !== "function") {
            drawingDocumentIoOk = false
            drawingDocumentIoStatus = "drawing storage unavailable"
            revision += 1
            return false
        }
        var result = drawingDocumentStore.open(url)
        drawingDocumentIoOk = !!result.ok
        drawingDocumentIoStatus = String(result.message || (drawingDocumentIoOk ? "opened drawing" : "open failed"))
        if (!drawingDocumentIoOk) {
            revision += 1
            return false
        }
        if (!drawingNativeController || typeof drawingNativeController.loadModel !== "function" || !drawingNativeController.loadModel(result.model || ({}))) {
            drawingDocumentIoOk = false
            drawingDocumentIoStatus = "drawing model rejected"
            revision += 1
            return false
        }
        drawingDocumentPath = String(result.path || "")
        drawingSession.syncNativeDrawingModel()
        markDrawingDocumentClean("opened drawing")
        revision += 1
        return true
    }

    function textEditorLineCount(unusedRevision) {
        return textEditorSession.textEditorLineCount(unusedRevision)
    }

    function textEditorCharCount(unusedRevision) {
        return textEditorSession.textEditorCharCount(unusedRevision)
    }

    function textEditorWordCount(unusedRevision) {
        return textEditorSession.textEditorWordCount(unusedRevision)
    }

    function textEditorReadTime(unusedRevision) {
        return textEditorSession.textEditorReadTime(unusedRevision)
    }

    function textEditorSelectionLength(unusedRevision) {
        return textEditorSession.textEditorSelectionLength(unusedRevision)
    }

    function textEditorCursorLine(unusedRevision) {
        return textEditorSession.textEditorCursorLine(unusedRevision)
    }

    function textEditorCursorColumn(unusedRevision) {
        return textEditorSession.textEditorCursorColumn(unusedRevision)
    }

    function textEditorModifiedState(unusedRevision) {
        return textEditorSession.textEditorModifiedState(unusedRevision)
    }

    function drawingFindById(items, id, fallback) {
        return drawingSession.drawingFindById(items, id, fallback)
    }

    function selectedDrawingTool() {
        return drawingSession.selectedDrawingTool()
    }

    function selectedDrawingExternalTool() {
        return drawingSession.selectedDrawingExternalTool()
    }

    function selectedDrawingLayer() {
        return drawingSession.selectedDrawingLayer()
    }

    function selectedDrawingObject() {
        return drawingSession.selectedDrawingObject()
    }

    function drawingAnchorPoint(anchorId) {
        return drawingSession.drawingAnchorPoint(anchorId)
    }

    function setDrawingAnchorPosition(anchorId, x, y) {
        drawingSession.setDrawingAnchorPosition(anchorId, x, y)
    }

    function selectNearestDrawingAnchor(x, y, tolerance) {
        return drawingSession.selectNearestDrawingAnchor(x, y, tolerance)
    }

    function selectDrawingTool(toolId) {
        drawingSession.selectDrawingTool(toolId)
    }

    function selectDrawingExternalTool(toolId) {
        drawingSession.selectDrawingExternalTool(toolId)
    }

    function selectDrawingPreset(presetId) {
        drawingSession.selectDrawingPreset(presetId)
    }

    function selectedDrawingPreset() {
        return drawingSession.selectedDrawingPreset()
    }

    function toggleDrawingToolPalette() {
        drawingSession.toggleDrawingToolPalette()
    }

    function closeDrawingToolPalette() {
        drawingSession.closeDrawingToolPalette()
    }

    function moveDrawingToolPalette(x, y) {
        drawingSession.moveDrawingToolPalette(x, y)
    }

    function setDrawingGridVisible(visible) {
        drawingSession.setDrawingGridVisible(visible)
    }

    function setDrawingLineVariant(variant) {
        drawingSession.setDrawingLineVariant(variant)
    }

    function setSelectedDrawingVariantId(variantId) {
        drawingSession.setSelectedDrawingVariantId(variantId)
    }

    function setDrawingCircleArcMode(mode) {
        drawingSession.setDrawingCircleArcMode(mode)
    }

    function setDrawingStrokeColor(rawColor) {
        drawingSession.setDrawingStrokeColor(rawColor)
    }

    function setDrawingFillColor(rawColor) {
        drawingSession.setDrawingFillColor(rawColor)
    }

    function setDrawingLineThickness(value) {
        drawingSession.setDrawingLineThickness(value)
    }

    function setDrawingLineStyle(style) {
        drawingSession.setDrawingLineStyle(style)
    }

    function setDrawingStrokeOpacity(value) {
        drawingSession.setDrawingStrokeOpacity(value)
    }

    function setDrawingGridMode(mode) {
        drawingSession.setDrawingGridMode(mode)
    }

    function setDrawingGridDivisions(divisions) {
        drawingSession.setDrawingGridDivisions(divisions)
    }

    function setDrawingGridMajorEvery(value) {
        drawingSession.setDrawingGridMajorEvery(value)
    }

    function setDrawingAsciiCellGridVisible(visible) {
        drawingSession.setDrawingAsciiCellGridVisible(visible)
    }

    function setDrawingAsciiColumns(columns) {
        drawingSession.setDrawingAsciiColumns(columns)
    }

    function setDrawingAsciiRows(rows) {
        drawingSession.setDrawingAsciiRows(rows)
    }

    function setDrawingAsciiMajorEvery(value) {
        drawingSession.setDrawingAsciiMajorEvery(value)
    }

    function setDrawingCenterAxesVisible(visible) {
        drawingSession.setDrawingCenterAxesVisible(visible)
    }

    function setDrawingDiagonalGuidesVisible(visible) {
        drawingSession.setDrawingDiagonalGuidesVisible(visible)
    }

    function setDrawingRadialGuidesVisible(visible) {
        drawingSession.setDrawingRadialGuidesVisible(visible)
    }

    function setDrawingRadialGuideCount(count) {
        drawingSession.setDrawingRadialGuideCount(count)
    }

    function setDrawingArtboardBorderVisible(visible) {
        drawingSession.setDrawingArtboardBorderVisible(visible)
    }

    function setDrawingSnapGrid(enabled) {
        drawingSession.setDrawingSnapGrid(enabled)
    }

    function setDrawingSnapGridStepPx(stepPx) {
        drawingSession.setDrawingSnapGridStepPx(stepPx)
    }

    function setDrawingObjectSnapEnabled(enabled) {
        drawingSession.setDrawingObjectSnapEnabled(enabled)
    }

    function setDrawingObjectSnapTolerancePx(value) {
        drawingSession.setDrawingObjectSnapTolerancePx(value)
    }

    function setDrawingObjectSnapEndpointEnabled(enabled) {
        drawingSession.setDrawingObjectSnapEndpointEnabled(enabled)
    }

    function setDrawingObjectSnapMidpointEnabled(enabled) {
        drawingSession.setDrawingObjectSnapMidpointEnabled(enabled)
    }

    function setDrawingObjectSnapCenterEnabled(enabled) {
        drawingSession.setDrawingObjectSnapCenterEnabled(enabled)
    }

    function setDrawingObjectSnapVertexEnabled(enabled) {
        drawingSession.setDrawingObjectSnapVertexEnabled(enabled)
    }

    function setDrawingToolParameter(parameter, value) {
        drawingSession.setDrawingToolParameter(parameter, value)
    }

    function setDrawingRegularPolygonSides(value) {
        drawingSession.setDrawingRegularPolygonSides(value)
    }

    function setDrawingRegularPolygonRotationDeg(value) {
        drawingSession.setDrawingRegularPolygonRotationDeg(value)
    }

    function updateDrawingToolParameterField(field, rawValue) {
        drawingSession.updateDrawingToolParameterField(field, rawValue)
    }

    function setDrawingCanvasZoom(zoom) {
        drawingSession.setDrawingCanvasZoom(zoom)
    }

    function drawingCanvasBaseViewSize(viewWidth, viewHeight) {
        return drawingSession.drawingCanvasBaseViewSize(viewWidth, viewHeight)
    }

    function zoomDrawingCanvasAt(factor, focusX, focusY, viewWidth, viewHeight) {
        drawingSession.zoomDrawingCanvasAt(factor, focusX, focusY, viewWidth, viewHeight)
    }

    function panDrawingCanvasBy(dx, dy) {
        drawingSession.panDrawingCanvasBy(dx, dy)
    }

    function zoomDrawingCanvasIn() {
        drawingSession.zoomDrawingCanvasIn()
    }

    function zoomDrawingCanvasOut() {
        drawingSession.zoomDrawingCanvasOut()
    }

    function resetDrawingCanvasZoom() {
        drawingSession.resetDrawingCanvasZoom()
    }

    function fitDrawingCanvasToView() {
        drawingSession.fitDrawingCanvasToView()
    }

    function selectDrawingLayer(layerId) {
        drawingSession.selectDrawingLayer(layerId)
    }

    function selectDrawingObject(objectId) {
        drawingSession.selectDrawingObject(objectId)
    }

    function clearDrawingObjectSelection() {
        drawingSession.clearDrawingObjectSelection()
    }

    function deleteSelectedDrawingObject() {
        drawingSession.deleteSelectedDrawingObject()
    }

    function duplicateSelectedDrawingObject() {
        drawingSession.duplicateSelectedDrawingObject()
    }

    function copySelectedDrawingObject() {
        drawingSession.copySelectedDrawingObject()
    }

    function pasteCopiedDrawingObject() {
        drawingSession.pasteCopiedDrawingObject()
    }

    function moveDrawingObjectBy(objectId, dx, dy) {
        drawingSession.moveDrawingObjectBy(objectId, dx, dy)
    }

    function moveDrawingObjectsBy(objectIds, dx, dy) {
        drawingSession.moveDrawingObjectsBy(objectIds, dx, dy)
    }

    function beginDrawingObjectMove() {
        drawingSession.beginDrawingObjectMove()
    }

    function endDrawingObjectMove() {
        drawingSession.endDrawingObjectMove()
    }

    function moveSelectedDrawingObjectBy(dx, dy) {
        drawingSession.moveSelectedDrawingObjectBy(dx, dy)
    }

    function nudgeSelectedDrawingObjectByPx(dxPx, dyPx) {
        drawingSession.nudgeSelectedDrawingObjectByPx(dxPx, dyPx)
    }

    function updateSelectedDrawingObjectField(field, rawValue) {
        drawingSession.updateSelectedDrawingObjectField(field, rawValue)
    }

    function selectDrawingObjectAtNormalized(x, y) {
        return drawingSession.selectDrawingObjectAtNormalized(x, y)
    }

    function hitDrawingObjectAtNormalized(x, y) {
        return drawingSession.hitDrawingObjectAtNormalized(x, y)
    }

    function selectDrawingObjects(objectIds) {
        drawingSession.selectDrawingObjects(objectIds)
    }

    function toggleDrawingObjectSelection(objectId) {
        drawingSession.toggleDrawingObjectSelection(objectId)
    }

    function syncNativeDrawingModel() {
        drawingSession.syncNativeDrawingModel()
    }

    function handleDrawingCanvasClick(x, y, snapStepPx) {
        drawingSession.handleDrawingCanvasClick(x, y, snapStepPx)
    }

    function cancelDrawingPendingShape() {
        drawingSession.cancelDrawingPendingShape()
    }

    function undoDrawingCommand() {
        drawingSession.undoDrawingCommand()
    }

    function redoDrawingCommand() {
        drawingSession.redoDrawingCommand()
    }

    function resetNativeDrawingDocument() {
        drawingSession.resetNativeDrawingDocument()
    }

    function loadInitialDrawingModel(document) {
        drawingSession.loadInitialDrawingModel(document)
    }

    function drawingCanvasObjects(unusedRevision) {
        return drawingSession.drawingCanvasObjects(unusedRevision)
    }

    function drawingCanvasDocument(unusedRevision) {
        return drawingSession.drawingCanvasDocument(unusedRevision)
    }

    function drawingCanvasExportDocument(unusedRevision) {
        return drawingSession.drawingCanvasExportDocument(unusedRevision)
    }

    function drawingCanvasExportJson(unusedRevision) {
        return drawingSession.drawingCanvasExportJson(unusedRevision)
    }

    function drawingObjectCounts(unusedRevision) {
        return drawingSession.drawingObjectCounts(unusedRevision)
    }

    function drawingModelValidationRows(unusedRevision) {
        return drawingSession.drawingModelValidationRows(unusedRevision)
    }

    function drawingFitTransform(unusedRevision) {
        return drawingSession.drawingFitTransform(unusedRevision)
    }

    function drawingEditNumber(value) {
        return drawingSession.drawingEditNumber(value)
    }

    function drawingObjectEditRows(unusedRevision) {
        return drawingSession.drawingObjectEditRows(unusedRevision)
    }

    function drawingInspectorRows(unusedRevision) {
        return drawingSession.drawingInspectorRows(unusedRevision)
    }

    function drawingToolSettingsRows(unusedRevision) {
        return drawingSession.drawingToolSettingsRows(unusedRevision)
    }

    function drawingToolParameterEditRows(unusedRevision) {
        return drawingSession.drawingToolParameterEditRows(unusedRevision)
    }

    function hasSelectedDrawingExternalTool(unusedRevision) {
        return drawingSession.hasSelectedDrawingExternalTool(unusedRevision)
    }

    function drawingExternalToolRows(unusedRevision) {
        return drawingSession.drawingExternalToolRows(unusedRevision)
    }

    function drawingSidebarRows(section, unusedRevision) {
        return drawingSession.drawingSidebarRows(section, unusedRevision)
    }

    function drawingSidebarRowSelected(section, row, unusedRevision) {
        return drawingSession.drawingSidebarRowSelected(section, row, unusedRevision)
    }

    function drawingSidebarRowClickable(section, unusedRevision) {
        return drawingSession.drawingSidebarRowClickable(section, unusedRevision)
    }

    function drawingSidebarRowClicked(section, row) {
        drawingSession.drawingSidebarRowClicked(section, row)
    }

    function drawingToolPaletteRows(unusedRevision) {
        return drawingSession.drawingToolPaletteRows(unusedRevision)
    }

    function drawingValidationRows(unusedRevision) {
        return drawingSession.drawingValidationRows(unusedRevision)
    }

    function drawingModelObjectRows(unusedRevision) {
        return drawingSession.drawingModelObjectRows(unusedRevision)
    }

    function drawingLogRows(unusedRevision) {
        return drawingSession.drawingLogRows(unusedRevision)
    }

    function drawingExportRows(unusedRevision) {
        return drawingSession.drawingExportRows(unusedRevision)
    }

    function drawingManifestRows(unusedRevision) {
        return drawingSession.drawingManifestRows(unusedRevision)
    }

    function displayDataPath(path) {
        var text = String(path || "")
        var marker = "/qt_qml_region_split/"
        var index = text.indexOf(marker)
        if (index >= 0) {
            return text.slice(index + marker.length)
        }
        return text
    }

    function mapRowCount() {
        return mapSession.mapRowCount()
    }

    function mapColCount() {
        return mapSession.mapColCount()
    }

    function mapTokenAt(row, col) {
        return mapSession.mapTokenAt(row, col)
    }

    function selectedMapToken() {
        return mapSession.selectedMapToken()
    }

    function selectedMapMetadata() {
        return mapSession.selectedMapMetadata()
    }

    function selectMapCell(row, col) {
        mapSession.selectMapCell(row, col)
        revision += 1
    }

    function mapTokenCounts(unusedRevision) {
        return mapSession.mapTokenCounts()
    }

    function mapPaletteRows(unusedRevision) {
        return mapSession.mapPaletteRows()
    }

    function selectedCellIntent() {
        return mapSession.selectedCellIntent()
    }

    function selectedCellTags() {
        return mapSession.selectedCellTags()
    }

    function selectedCellStatus() {
        return mapSession.selectedCellStatus()
    }

    function selectedCellCodeRefs() {
        var refs = mapSession.selectedCellCodeRefs()
        if (!Array.isArray(refs) || refs.length === 0) {
            return [displayDataPath(mapSession.mapCsvPath)]
        }
        return refs
    }

    function mapValidationRows(unusedRevision) {
        var rows = [
            { label: "Rows", value: String(mapRowCount()) },
            { label: "Columns", value: String(mapColCount()) },
            { label: "Metadata rows", value: String(Object.keys(cellMetadataByKey).length) },
            { label: "Writes", value: writeDisabled ? "disabled" : "enabled" }
        ]
        if (mapRowCount() === 0 || mapColCount() === 0) {
            rows.push({ label: "Grid", value: "missing map data" })
        } else {
            rows.push({ label: "Grid", value: "loaded" })
        }
        return rows
    }

    function mapLogRows(unusedRevision) {
        return [
            { label: "Storage", value: "read only" },
            { label: "CSV", value: displayDataPath(mapCsvPath) },
            { label: "Metadata", value: displayDataPath(cellMetadataPath) },
            { label: "Selected", value: String(selectedMapRow) + "," + String(selectedMapCol) }
        ]
    }

    function mapCellInspectorDocument(unusedRevision) {
        var token = selectedMapToken()
        var tags = selectedCellTags()
        var tagText = tags.length > 0 ? tags.join(", ") : "none"
        var codeRefs = selectedCellCodeRefs()
        return {
            id: "csv_map_cell_inspector",
            targetId: "cell_" + String(selectedMapRow) + "_" + String(selectedMapCol),
            targetLabel: "cell " + String(selectedMapRow) + "," + String(selectedMapCol),
            targetType: "map_cell",
            status: selectedCellStatus(),
            sections: [
                {
                    id: "facts",
                    label: "Facts",
                    type: "rows",
                    visible: inspectorSectionVisible("facts"),
                    rows: [
                        { label: "Coordinate", value: String(selectedMapRow) + "," + String(selectedMapCol) },
                        { label: "Token", value: token },
                        { label: "Status", value: selectedCellStatus() },
                        { label: "Tags", value: tagText }
                    ]
                },
                {
                    id: "selection",
                    label: "Selection",
                    type: "text",
                    visible: inspectorSectionVisible("selection"),
                    items: [
                        { label: "Intent", value: selectedCellIntent() },
                        { label: "Map", value: projectTitle },
                        { label: "Validation", value: mapRowCount() > 0 ? "grid loaded; persistence disabled" : "missing grid data" }
                    ]
                },
                {
                    id: "code_refs",
                    label: "Code Refs",
                    type: "code_refs",
                    visible: inspectorSectionVisible("code_refs"),
                    items: codeRefs
                },
                {
                    id: "receipts",
                    label: "Receipts",
                    type: "rows",
                    visible: inspectorSectionVisible("receipts"),
                    rows: [
                        { label: "Writes", value: writeDisabled ? "disabled" : "enabled" },
                        { label: "CSV", value: displayDataPath(mapCsvPath) },
                        { label: "Metadata", value: displayDataPath(cellMetadataPath) }
                    ]
                }
            ]
        }
    }

    function normalizeRoute(route) {
        var routeId = String(route.route_id || route.id || "")
        return {
            id: routeId,
            parent: String(route.parent || ""),
            label: String(route.label || routeId),
            type: String(route.type || "route"),
            status: String(route.status || "pending"),
            summary: String(route.summary || ""),
            purpose: String(route.purpose || route.summary || ""),
            objects: asArray(route.objects),
            children: asArray(route.children),
            codeRefs: asArray(route.code_refs || route.codeRefs),
            prompts: asArray(route.prompts)
        }
    }

    function fallbackRootRoute() {
        return {
            id: "missing_review_subject",
            parent: "",
            label: "Missing Review Subject",
            type: "root",
            status: "pending",
            summary: "Review subject data could not be loaded.",
            purpose: "Check the active project profile data_sources.review_subject path.",
            objects: [],
            children: [],
            codeRefs: [],
            prompts: ["Is the review subject JSON present and valid?"]
        }
    }

    function blankRoute() {
        return {
            id: "",
            parent: "",
            label: "",
            type: "",
            status: "",
            summary: "",
            purpose: "",
            objects: [],
            children: [],
            codeRefs: [],
            prompts: []
        }
    }

    function clearReviewSubject() {
        hasReviewSubject = false
        selectedSubjectId = ""
        selectedSubjectLabel = ""
        rootRouteId = ""
        selectedRouteId = ""
        routes = []
        backStack = []
        forwardStack = []
        statusOverrides = ({})
        notes = []
        revision += 1
    }

    function loadReviewSubject(document) {
        var reviewModeEnabled = hasActivityMode("review")
        var subject = document && document.subject ? document.subject : ({})
        var sourceRoutes = asArray(document && document.routes ? document.routes : [])

        if (!reviewModeEnabled && !subject.subject_id && sourceRoutes.length === 0) {
            clearReviewSubject()
            return
        }

        selectedSubjectId = String(subject.subject_id || "")
        selectedSubjectLabel = String(subject.label || selectedSubjectId)
        rootRouteId = String(subject.root_route_id || "")

        var nextRoutes = []
        for (var index = 0; index < sourceRoutes.length; ++index) {
            var normalized = normalizeRoute(sourceRoutes[index])
            if (normalized.id.length > 0) {
                nextRoutes.push(normalized)
            }
        }

        routes = nextRoutes.length > 0 ? nextRoutes : [fallbackRootRoute()]
        hasReviewSubject = nextRoutes.length > 0
        if (!findRouteById(rootRouteId)) {
            rootRouteId = routes[0].id
        }
        selectedRouteId = rootRouteId
        backStack = []
        forwardStack = []
        revision += 1
    }

    function findRouteById(routeId) {
        for (var index = 0; index < routes.length; ++index) {
            if (routes[index].id === routeId) {
                return routes[index]
            }
        }
        return null
    }

    function routeById(routeId) {
        if (!hasReviewSubject && routes.length === 0) {
            return blankRoute()
        }
        return findRouteById(routeId) || routes[0] || fallbackRootRoute()
    }

    function currentRoute() {
        return routeById(selectedRouteId)
    }

    function routeStatus(routeId) {
        var route = routeById(routeId)
        return statusOverrides[routeId] || route.status || "pending"
    }

    function noteCount(routeId) {
        var count = 0
        for (var index = 0; index < notes.length; ++index) {
            if (notes[index].routeId === routeId) {
                count += 1
            }
        }
        return count
    }

    function routeNotes(routeId, unusedRevision) {
        var result = []
        for (var index = 0; index < notes.length; ++index) {
            if (notes[index].routeId === routeId) {
                result.push(notes[index])
            }
        }
        return result
    }

    function allNotes(unusedRevision) {
        return notes
    }

    function inspectorSectionVisible(sectionId) {
        var sections = normalizedInspectorSections(rightInspectorSections)
        return sections[String(sectionId)] !== false
    }

    function setInspectorSectionVisible(sectionId, visible) {
        var next = normalizedInspectorSections(rightInspectorSections)
        next[String(sectionId)] = !!visible
        rightInspectorSections = next
        markShellLayoutDirty()
    }

    function rightInspectorDocument(unusedRevision) {
        if (!hasReviewSubject) {
            return {
                id: "empty_inspector",
                targetId: "",
                targetLabel: "",
                targetType: "",
                status: "",
                sections: []
            }
        }
        var route = currentRoute()
        var notesForRoute = routeNotes(route.id, revision)
        return {
            id: "route_inspector",
            targetId: route.id,
            targetLabel: route.label,
            targetType: route.type,
            status: routeStatus(route.id),
            sections: [
                {
                    id: "facts",
                    label: "Facts",
                    type: "rows",
                    visible: inspectorSectionVisible("facts"),
                    rows: [
                        { label: "Route", value: route.label },
                        { label: "Type", value: route.type },
                        { label: "Status", value: routeStatus(route.id) },
                        { label: "Children", value: String((route.children || []).length) },
                        { label: "Notes", value: String(noteCount(route.id)) }
                    ]
                },
                {
                    id: "selection",
                    label: "Selection",
                    type: "text",
                    visible: inspectorSectionVisible("selection"),
                    items: [
                        { label: "Summary", value: route.summary || "" },
                        { label: "Purpose", value: route.purpose || "" },
                        { label: "Path", value: breadcrumbText(route.id) }
                    ]
                },
                {
                    id: "code_refs",
                    label: "Code Refs",
                    type: "code_refs",
                    visible: inspectorSectionVisible("code_refs"),
                    items: route.codeRefs || []
                },
                {
                    id: "notes",
                    label: "Notes",
                    type: "notes",
                    visible: inspectorSectionVisible("notes") && notesForRoute.length > 0,
                    items: notesForRoute.slice(-2)
                },
                {
                    id: "receipts",
                    label: "Receipts",
                    type: "rows",
                    visible: inspectorSectionVisible("receipts"),
                    rows: [
                        { label: "Writes", value: writeDisabled ? "disabled" : "enabled" },
                        { label: "Storage", value: "in memory" },
                        { label: "Subject", value: selectedSubjectId }
                    ]
                },
                {
                    id: "actions",
                    label: "Actions",
                    type: "actions",
                    visible: inspectorSectionVisible("actions"),
                    actions: [
                        { id: "status_pending", label: "Pending", kind: "status", value: "pending" },
                        { id: "status_accepted", label: "Accept", kind: "status", value: "accepted" },
                        { id: "status_needs_rework", label: "Rework", kind: "status", value: "needs_rework" },
                        { id: "status_rejected", label: "Reject", kind: "status", value: "rejected" }
                    ]
                }
            ]
        }
    }

    function childRoutes(routeId, unusedRevision) {
        if (!hasReviewSubject) {
            return []
        }
        var route = routeById(routeId)
        var result = []
        for (var index = 0; index < route.children.length; ++index) {
            result.push(routeById(route.children[index]))
        }
        return result
    }

    function siblingRoutes(routeId) {
        if (!hasReviewSubject) {
            return []
        }
        var route = routeById(routeId)
        if (!route.parent) {
            return childRoutes(rootRouteId, revision)
        }
        return childRoutes(route.parent, revision)
    }

    function breadcrumb(routeId) {
        if (!hasReviewSubject) {
            return []
        }
        var result = []
        var guard = 0
        var route = routeById(routeId)
        while (route && guard < 20) {
            result.unshift(route)
            if (!route.parent) {
                break
            }
            route = routeById(route.parent)
            guard += 1
        }
        return result
    }

    function breadcrumbText(routeId) {
        var crumbs = breadcrumb(routeId)
        var labels = []
        for (var index = 0; index < crumbs.length; ++index) {
            labels.push(crumbs[index].label)
        }
        return labels.join(" / ")
    }

    function setActivityMode(modeId) {
        if (!activityModeAvailable(modeId)) {
            return
        }
        activityMode = modeId
        if (modeId === "settings" && !selectedSettingsPage.length) {
            selectedSettingsPage = "theme"
        }
        revision += 1
    }

    function setSettingsPage(pageId) {
        selectedSettingsPage = pageId
        activityMode = "settings"
        revision += 1
    }

    function toggleLeftPanel() {
        leftPanelCollapsed = !leftPanelCollapsed
        markShellLayoutDirty()
    }

    function toggleRightPanel() {
        rightPanelCollapsed = !rightPanelCollapsed
        markShellLayoutDirty()
    }

    function toggleBottomPanel() {
        bottomPanelCollapsed = !bottomPanelCollapsed
        markShellLayoutDirty()
    }

    function setLeftPanelCollapsed(collapsed) {
        leftPanelCollapsed = !!collapsed
        markShellLayoutDirty()
    }

    function setRightPanelCollapsed(collapsed) {
        rightPanelCollapsed = !!collapsed
        markShellLayoutDirty()
    }

    function setBottomPanelCollapsed(collapsed) {
        bottomPanelCollapsed = !!collapsed
        markShellLayoutDirty()
    }

    function setLeftPanelWidth(width) {
        leftPanelWidth = clamp(width, leftPanelMinWidth, leftPanelMaxWidth)
        markShellLayoutDirty()
    }

    function setRightPanelWidth(width) {
        rightPanelWidth = clamp(width, rightPanelMinWidth, rightPanelMaxWidth)
        markShellLayoutDirty()
    }

    function setBottomPanelHeight(height) {
        bottomPanelHeight = clamp(height, bottomPanelMinHeight, bottomPanelMaxHeight)
        markShellLayoutDirty()
    }

    function applyTheme(theme) {
        uiThemeDocument = theme
        UiStyle.applyTheme(theme)
        revision += 1
    }

    function selectRoute(routeId) {
        if (!hasReviewSubject) {
            return
        }
        if (routeId === selectedRouteId || !findRouteById(routeId)) {
            return
        }
        var nextBack = backStack.slice()
        nextBack.push(selectedRouteId)
        backStack = nextBack
        forwardStack = []
        selectedRouteId = routeId
        revision += 1
    }

    function goHome() {
        selectRoute(rootRouteId)
    }

    function goBack() {
        if (backStack.length === 0) {
            return
        }
        var nextBack = backStack.slice()
        var prior = nextBack.pop()
        var nextForward = forwardStack.slice()
        nextForward.push(selectedRouteId)
        backStack = nextBack
        forwardStack = nextForward
        selectedRouteId = prior
        revision += 1
    }

    function goForward() {
        if (forwardStack.length === 0) {
            return
        }
        var nextForward = forwardStack.slice()
        var next = nextForward.pop()
        var nextBack = backStack.slice()
        nextBack.push(selectedRouteId)
        backStack = nextBack
        forwardStack = nextForward
        selectedRouteId = next
        revision += 1
    }

    function setLocalTab(tabName) {
        var requested = String(tabName || "").toLowerCase()
        for (var index = 0; index < localTabs.length; ++index) {
            if (String(localTabs[index].id).toLowerCase() === requested
                    || String(localTabs[index].label).toLowerCase() === requested) {
                selectedLocalTab = localTabs[index].id
                revision += 1
                return
            }
        }
        selectedLocalTab = tabName
        revision += 1
    }

    function setShelfTab(tabName) {
        selectedShelfTab = tabName
        revision += 1
    }

    function setStatus(routeId, status) {
        var next = Object.assign({}, statusOverrides)
        next[routeId] = status
        statusOverrides = next
        revision += 1
    }

    function runInspectorAction(actionId, targetId) {
        var text = String(actionId || "")
        if (text.indexOf("status_") !== 0) {
            return
        }
        setStatus(targetId || selectedRouteId, text.slice(7))
    }

    function addNote(routeId, status, body) {
        if (!hasReviewSubject) {
            return
        }
        var text = String(body || "").trim()
        if (!text.length) {
            return
        }
        var next = notes.slice()
        next.push({
            id: "note_" + String(next.length + 1).padStart(3, "0"),
            routeId: routeId,
            routeLabel: routeById(routeId).label,
            status: status,
            body: text,
            createdAt: new Date().toLocaleString()
        })
        notes = next
        setStatus(routeId, status)
        revision += 1
    }

    function statusCounts(unusedRevision) {
        var counts = { pending: 0, accepted: 0, needs_rework: 0, rejected: 0 }
        for (var index = 0; index < routes.length; ++index) {
            var status = routeStatus(routes[index].id)
            if (counts[status] === undefined) {
                counts[status] = 0
            }
            counts[status] += 1
        }
        return counts
    }
}
