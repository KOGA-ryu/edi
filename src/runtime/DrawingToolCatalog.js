.pragma library

function toolModes() {
    return [
        { id: "select_move", label: "Select", meta: "edit" },
        { id: "anchor_points", label: "Point", meta: "snap" },
        { id: "line_polyline", label: "Line", meta: "draw" },
        { id: "circle_arc", label: "Circle", meta: "draw" },
        { id: "rectangle_polygon", label: "Rect", meta: "shape" },
        { id: "regular_polygon", label: "Polygon", meta: "shape" },
        { id: "image_reference_frame", label: "Image frame", meta: "ascii" },
        { id: "ascii_crop_frame", label: "ASCII crop", meta: "ascii" },
        { id: "ascii_cell_region", label: "ASCII cell region", meta: "ascii" },
        { id: "tone_probe", label: "Tone probe", meta: "ascii" },
        { id: "glyph_baseline", label: "Glyph baseline", meta: "ascii" },
        { id: "spline_curve", label: "Spline / curve", meta: "curve" },
        { id: "hatch_boundary", label: "Hatch / boundary", meta: "region" },
        { id: "svg_fit", label: "Asset fit", meta: "block" },
        { id: "offset_trim", label: "Offset / trim", meta: "mod" },
        { id: "mirror_array", label: "Mirror / array", meta: "mod" },
        { id: "measure_inspect", label: "Measure / inspect", meta: "data" },
        { id: "layer_review", label: "Layer review", meta: "std" },
        { id: "trace_markup", label: "Trace / markup", meta: "review" }
    ]
}

function toolSettingsById() {
    return {
        select_move: [
            { label: "Mode", value: "select and move existing objects" },
            { label: "Hit test", value: "object bounding boxes first" },
            { label: "Selection", value: "one object, one layer" },
            { label: "Mutation", value: "disabled in shell v0" }
        ],
        anchor_points: [
            { label: "Mode", value: "place and inspect named anchors" },
            { label: "Snap", value: "grid, radial axes, artboard center" },
            { label: "Required anchors", value: "root and tip" },
            { label: "Point display", value: "selected anchor highlighted" }
        ],
        svg_fit: [
            { label: "Mode", value: "fit SVG asset between two targets" },
            { label: "Transform", value: "translate, scale, rotate" },
            { label: "Mirror", value: "allowed by asset sidecar" },
            { label: "Validation", value: "anchor_root and anchor_tip required" }
        ],
        line_polyline: [
            { label: "Mode", value: "line and connected polyline drafting" },
            { label: "Input", value: "absolute, relative, or polar point pairs" },
            { label: "Snaps", value: "endpoint, midpoint, center, intersection" },
            { label: "Output role", value: "construction paths or closed regions" }
        ],
        circle_arc: [
            { label: "Mode", value: "circle or configured arc" },
            { label: "Input", value: "click center, click radius point" },
            { label: "Data", value: "store center, radius, start angle, end angle" },
            { label: "Output role", value: "circle guides or arc boundaries" }
        ],
        rectangle_polygon: [
            { label: "Mode", value: "rectangles, regular polygons, bounded cells" },
            { label: "Input", value: "corner-corner, center-size, side count, radius" },
            { label: "Pattern use", value: "tile units, frames, panels, medallion cells" },
            { label: "Validation", value: "closed polygon and exact side/angle metadata" }
        ],
        regular_polygon: [
            { label: "Mode", value: "regular polygon drafting" },
            { label: "Input", value: "click center, click radius point" },
            { label: "Controls", value: "side count and rotation are model-backed" },
            { label: "Output", value: "closed polygon with exact side metadata" }
        ],
        image_reference_frame: [
            { label: "Mode", value: "two-click frame for an image reference" },
            { label: "Use", value: "place source image bounds before ASCII conversion" },
            { label: "Output", value: "reference rectangle, not rendered source image" },
            { label: "Next hook", value: "bind frame to input image path" }
        ],
        ascii_crop_frame: [
            { label: "Mode", value: "two-click output crop frame" },
            { label: "Use", value: "define the exact region sent to the ASCII workbench" },
            { label: "Grid relation", value: "should align to character-cell columns and rows" },
            { label: "Output", value: "crop rectangle for future CLI params" }
        ],
        ascii_cell_region: [
            { label: "Mode", value: "two-click character-cell planning region" },
            { label: "Use", value: "reserve a block for dense glyph rendering or annotation" },
            { label: "Validation", value: "keep region inside crop frame when wired" },
            { label: "Output", value: "region rectangle with ASCII role" }
        ],
        tone_probe: [
            { label: "Mode", value: "single-click tone sample marker" },
            { label: "Use", value: "mark areas to compare brightness, contrast, and glyph density" },
            { label: "Future data", value: "sample luminance from image reference" },
            { label: "Output", value: "named probe point" }
        ],
        glyph_baseline: [
            { label: "Mode", value: "two-click baseline guide" },
            { label: "Use", value: "align text/glyph flow or directional ASCII strokes" },
            { label: "Future data", value: "angle and length for glyph placement" },
            { label: "Output", value: "baseline segment" }
        ],
        spline_curve: [
            { label: "Mode", value: "Bezier/spline curve drafting for ornament parts" },
            { label: "Input", value: "control points with root/tip anchors" },
            { label: "Math", value: "sampled curve, tangent, normal, bounds" },
            { label: "Boundary", value: "V0 places curves; bend-along-path stays future" }
        ],
        hatch_boundary: [
            { label: "Mode", value: "closed-region detection and fill/hatch preview" },
            { label: "Input", value: "selected closed paths or generated boundary" },
            { label: "Pattern use", value: "flat color fills, tile regions, negative space" },
            { label: "Validation", value: "reject open regions and ambiguous self-crossing paths" }
        ],
        offset_trim: [
            { label: "Mode", value: "derive parallel paths and cut back geometry" },
            { label: "Offset", value: "used for borders, straps, grout, lineweight bands" },
            { label: "Trim", value: "cut against selected boundary objects" },
            { label: "Validation", value: "no dangling generated segments" }
        ],
        mirror_array: [
            { label: "Mode", value: "mirror, rectangular array, polar array, path array" },
            { label: "Pattern use", value: "rosettes, repeats, borders, radial petals" },
            { label: "Origin", value: "anchor_center or selected base point" },
            { label: "Validation", value: "symmetry target must match recipe" }
        ],
        measure_inspect: [
            { label: "Mode", value: "measure distance, angle, radius, area, bounds" },
            { label: "Object data", value: "layer, kind, coordinates, counts" },
            { label: "Use", value: "recipe proof and visual rejection causes" },
            { label: "Export", value: "manifest rows and diagnostic badges" }
        ],
        layer_review: [
            { label: "Mode", value: "inspect and sort layers" },
            { label: "Order", value: "canvas, grid, construction, fit, motif, anchors, markup, metadata" },
            { label: "Visibility", value: "model layer flags" },
            { label: "Standards", value: "layer naming, color, lineweight, role checker" }
        ],
        trace_markup: [
            { label: "Mode", value: "review overlay without modifying source geometry" },
            { label: "Markup", value: "comments, reject boxes, move/copy/delete notes" },
            { label: "Trace", value: "separate layer, non-destructive" },
            { label: "Use", value: "human review comments for Builder Dex" }
        ]
    }
}

function precisionTools() {
    return [
        { id: "grid_snap", label: "Grid snap", meta: "on" },
        { id: "object_snap", label: "Object snap", meta: "end/mid/center/vertex" },
        { id: "object_snap_tracking", label: "Snap tracking", meta: "project" },
        { id: "polar_tracking", label: "Polar tracking", meta: "15 deg" },
        { id: "ortho_lock", label: "Ortho lock", meta: "off" },
        { id: "coordinate_input", label: "Coordinates", meta: "abs/rel/polar" },
        { id: "bounds_check", label: "Bounds check", meta: "on" }
    ]
}

function dataTools() {
    return [
        { id: "block_attributes", label: "Block attributes", meta: "sidecar" },
        { id: "object_counts", label: "Object counts", meta: "manifest" },
        { id: "standards_check", label: "Standards check", meta: "layers" },
        { id: "trace_review", label: "Trace review", meta: "comments" },
        { id: "command_macros", label: "Command macros", meta: "recipes" }
    ]
}

function imageTools() {
    return [
        { id: "image_to_ascii_workbench_v3", label: "Image to ASCII", meta: "headless CLI" }
    ]
}

function externalToolSettingsById() {
    return {
        image_to_ascii_workbench_v3: [
            { label: "Status", value: "space reserved; CLI hook pending" },
            { label: "Tool root", value: "/Users/kogaryu/gameguy-3d-lab/image_to_ascii_workbench_v3" },
            { label: "First hook", value: "call CLI with image path and output paths" },
            { label: "Output", value: "TXT, CP437, PNG preview" },
            { label: "Size / tone", value: "width, height, cell aspect, brightness, contrast, gamma" },
            { label: "Sampling", value: "center, average, median, super2x, super4x" },
            { label: "Look", value: "dither, Sobel edge mode, palette, measured font darkness" }
        ]
    }
}

function assetSources() {
    return [
        { id: "ornament_codex", label: "ornament blocks", meta: "SVG" },
        { id: "manual_plot_unions", label: "manual plot unions", meta: "JSON" },
        { id: "approved_flower_parts", label: "approved flower parts", meta: "review" },
        { id: "future_border_lane", label: "border block lane", meta: "hold" }
    ]
}

function patternFamilies() {
    return [
        { id: "lotus_floral_templates", label: "lotus / floral templates", meta: "active" },
        { id: "radial_medallions", label: "radial medallions", meta: "math" },
        { id: "circle_arc_fillers", label: "circle arc fillers", meta: "math" },
        { id: "rectilinear_meanders", label: "rectilinear meanders", meta: "math" },
        { id: "interlace_fields", label: "interlace fields", meta: "math" },
        { id: "tile_filler_geometry", label: "tile filler geometry", meta: "math" }
    ]
}

function toolPresets() {
    return [
        { id: "lotus_petal_fit", label: "Lotus petal fit", meta: "block" },
        { id: "radial_ring_8", label: "Polar array 8", meta: "array" },
        { id: "border_segment_hold", label: "Border segment", meta: "xref" },
        { id: "review_trace_note", label: "Trace review note", meta: "markup" }
    ]
}

function layerStack() {
    return [
        { id: "layer_00_canvas", label: "canvas / artboard", meta: "base", visible: true },
        { id: "layer_01_grid", label: "grid", meta: "16x16", visible: true },
        { id: "layer_08_metadata", label: "metadata / counts", meta: "data", visible: false },
        { id: "layer_09_script_geometry", label: "script geometry", meta: "tests", visible: true }
    ]
}

function sidebarSections() {
    return [
        { id: "draw", title: "Draw", hint: "tools", source: "drawingToolModes", action: "tool", selectedProperty: "selectedDrawingToolId", ids: ["select_move", "anchor_points", "line_polyline", "circle_arc", "rectangle_polygon", "regular_polygon", "spline_curve"] },
        { id: "modify", title: "Modify", hint: "ops", source: "drawingToolModes", action: "tool", selectedProperty: "selectedDrawingToolId", ids: ["offset_trim", "mirror_array", "hatch_boundary"] },
        { id: "review", title: "Review", hint: "inspect", source: "drawingToolModes", action: "tool", selectedProperty: "selectedDrawingToolId", ids: ["measure_inspect", "layer_review", "trace_markup", "svg_fit"] },
        { id: "ascii_draft", title: "ASCII Draft", hint: "tools", source: "drawingToolModes", action: "tool", selectedProperty: "selectedDrawingToolId", ids: ["image_reference_frame", "ascii_crop_frame", "ascii_cell_region", "tone_probe", "glyph_baseline"] },
        { id: "precision", title: "Precision", hint: "snap", source: "drawingPrecisionTools", action: "", selectedProperty: "", ids: [] },
        { id: "presets", title: "Presets", hint: "recipes", source: "drawingToolPresets", action: "preset", selectedProperty: "selectedDrawingPresetId", ids: [] },
        { id: "assets", title: "Assets", hint: "codex", source: "drawingAssetSources", action: "", selectedProperty: "", ids: [] },
        { id: "image_tools", title: "Image Tools", hint: "workbench", source: "drawingImageTools", action: "external_tool", selectedProperty: "selectedDrawingExternalToolId", ids: [] },
        { id: "patterns", title: "Patterns", hint: "math", source: "drawingPatternFamilies", action: "", selectedProperty: "", ids: [] },
        { id: "automation", title: "Data / Automation", hint: "scripts", source: "drawingDataTools", action: "", selectedProperty: "", ids: [] },
        { id: "layers", title: "Layers", hint: "stack", source: "drawingLayerStack", action: "layer", selectedProperty: "selectedDrawingLayerId", ids: [] }
    ]
}
