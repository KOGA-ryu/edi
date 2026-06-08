#include "runtime/DrawingRuntimeCore.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaObject>
#include <QtMath>

#include <algorithm>
#include <cmath>
#include <limits>

namespace {

QVariantMap row(const QString &label, const QString &value)
{
    return {{"label", label}, {"value", value}};
}

QVariantMap item(const QString &id, const QString &label, const QString &meta)
{
    return {{"id", id}, {"label", label}, {"meta", meta}};
}

QVariantMap setting(const QString &label, const QString &value)
{
    return row(label, value);
}

QVariantList list(std::initializer_list<QVariantMap> rows)
{
    QVariantList result;
    for (const QVariantMap &item : rows) {
        result.push_back(item);
    }
    return result;
}

QVariantList toList(const QVariant &value)
{
    if (value.typeId() == QMetaType::QVariantList || value.canConvert<QVariantList>()) {
        return value.toList();
    }
    return {};
}

QVariantMap toMap(const QVariant &value)
{
    if (value.typeId() == QMetaType::QVariantMap || value.canConvert<QVariantMap>()) {
        return value.toMap();
    }
    return {};
}

QString text(const QVariantMap &map, const QString &key, const QString &fallback = {})
{
    const QString value = map.value(key).toString();
    return value.isEmpty() ? fallback : value;
}

double number(const QVariant &value, double fallback = 0.0)
{
    bool ok = false;
    const double result = value.toDouble(&ok);
    return ok && std::isfinite(result) ? result : fallback;
}

QString editNumberValue(const QVariant &value)
{
    const double result = std::round(number(value) * 1000.0) / 1000.0;
    QString text = QString::number(result, 'f', 3);
    while (text.contains('.') && text.endsWith('0')) {
        text.chop(1);
    }
    if (text.endsWith('.')) {
        text.chop(1);
    }
    return text.isEmpty() ? QStringLiteral("0") : text;
}

QVariant invoke(QObject *object, const char *method)
{
    QVariant result;
    if (object != nullptr) {
        QMetaObject::invokeMethod(object, method, Q_RETURN_ARG(QVariant, result));
    }
    return result;
}

QVariant invokeWithVariant(QObject *object, const char *method, const QVariant &arg)
{
    QVariant result;
    if (object != nullptr) {
        QMetaObject::invokeMethod(object, method, Q_RETURN_ARG(QVariant, result), Q_ARG(QVariant, arg));
    }
    return result;
}

QObject *objectProperty(QObject *object, const char *property)
{
    if (object == nullptr) {
        return nullptr;
    }
    return qvariant_cast<QObject *>(object->property(property));
}

QVariantMap selectedTool(QObject *controller)
{
    return toMap(invoke(controller, "selectedDrawingTool"));
}

QVariantMap selectedLayer(QObject *controller)
{
    return toMap(invoke(controller, "selectedDrawingLayer"));
}

QVariantMap selectedObject(QObject *controller)
{
    return toMap(invoke(controller, "selectedDrawingObject"));
}

QVariantMap selectedPreset(QObject *controller)
{
    return toMap(invoke(controller, "selectedDrawingPreset"));
}

QVariantMap modelDocument(QObject *controller)
{
    QObject *nativeController = objectProperty(controller, "drawingNativeController");
    if (nativeController == nullptr) {
        return {};
    }
    return toMap(invoke(nativeController, "modelDocument"));
}

QVariantMap anchorPoint(QObject *controller, const QString &id)
{
    return toMap(invokeWithVariant(controller, "drawingAnchorPoint", id));
}

QVariantList arrayField(const QVariantMap &object, const QString &key)
{
    return toList(object.value(key));
}

QString joinList(const QVariantList &items)
{
    QStringList parts;
    for (const QVariant &item : items) {
        parts.push_back(item.toString());
    }
    return parts.join(", ");
}

void appendPointInspectorRows(QVariantList &rows, const QVariantMap &object)
{
    rows.push_back(row("Point px", joinList(arrayField(object, "point_px"))));
}

void appendLineInspectorRows(QVariantList &rows, const QVariantMap &object)
{
    rows.push_back(row("From px", joinList(arrayField(object, "from_px"))));
    rows.push_back(row("To px", joinList(arrayField(object, "to_px"))));
}

void appendCircleInspectorRows(QVariantList &rows, const QVariantMap &object)
{
    rows.push_back(row("Center px", joinList(arrayField(object, "center_px"))));
    rows.push_back(row("Radius px", QString::number(number(object.value("radius_px")), 'f', 1)));
}

void appendInspectorRows(QVariantList &rows, const QVariantMap &object)
{
    const QString kind = object.value("kind").toString();
    if (kind == "point" || kind == "tone_probe") {
        appendPointInspectorRows(rows, object);
    } else if (kind == "line" || kind == "glyph_baseline") {
        appendLineInspectorRows(rows, object);
    } else if (kind == "circle") {
        appendCircleInspectorRows(rows, object);
    } else if (kind == "arc") {
        appendCircleInspectorRows(rows, object);
        rows.push_back(row("Angles", QString("%1 -> %2").arg(text(object, "start_angle_deg", "0"), text(object, "end_angle_deg", "0"))));
    } else if (kind == "polygon") {
        appendCircleInspectorRows(rows, object);
        rows.push_back(row("Sides", object.value("sides").toString()));
    } else if (kind == "rectangle" || kind == "image_reference_frame" || kind == "ascii_crop_frame" || kind == "ascii_cell_region") {
        rows.push_back(row("Rect px", joinList(arrayField(object, "rect_px"))));
    }
}

QVariantMap editRow(const QString &label, const QString &field, const QVariant &value, bool numeric = true)
{
    QVariantMap result{{"label", label}, {"field", field}, {"value", numeric ? editNumberValue(value) : value}};
    if (!numeric) {
        result.insert("numeric", false);
    }
    return result;
}

QString objectCountsText(const QVariantMap &counts)
{
    QStringList keys = counts.keys();
    std::sort(keys.begin(), keys.end());
    QStringList parts;
    for (const QString &key : keys) {
        parts.push_back(QString("%1:%2").arg(key, counts.value(key).toString()));
    }
    return parts.isEmpty() ? QStringLiteral("none") : parts.join(", ");
}

double snapshotValue(const QVariantMap &snapshot, const QString &name)
{
    return std::max(0.0, number(snapshot.value(name), 0.0));
}

double eventCount(double count)
{
    return count > 0.0 && std::isfinite(count) ? count : 1.0;
}

QVariantMap normalizedSnapshot(const QVariantMap &snapshot)
{
    return {
        {"revision", snapshotValue(snapshot, "revision")},
        {"selectedCount", snapshotValue(snapshot, "selectedCount")},
        {"visibleObjectCount", snapshotValue(snapshot, "visibleObjectCount")},
    };
}

QVariantList clonedEvents(const QVariant &events)
{
    QVariantList result;
    for (const QVariant &event : toList(events)) {
        result.push_back(toMap(event));
    }
    return result;
}

QVariantMap initialMetrics()
{
    return {
        {"active", false},
        {"mode", "idle"},
        {"startedAtMs", 0},
        {"pointerMoves", 0},
        {"controllerMutations", 0},
        {"renderRequests", 0},
        {"hitTests", 0},
        {"snapResolutions", 0},
        {"handlePlans", 0},
        {"revisionStart", 0},
        {"revisionEnd", 0},
        {"selectedCountStart", 0},
        {"selectedCountEnd", 0},
        {"visibleObjectCountStart", 0},
        {"visibleObjectCountEnd", 0},
        {"events", QVariantList{}},
    };
}

QVariantMap cloneMetrics(const QVariantMap &state)
{
    const QVariantMap source = state.isEmpty() ? initialMetrics() : state;
    QVariantList events;
    for (const QVariant &value : toList(source.value("events"))) {
        const QVariantMap event = toMap(value);
        events.push_back(QVariantMap{
            {"kind", event.value("kind").toString()},
            {"count", std::max(0.0, number(event.value("count"), 0.0))},
        });
    }
    return {
        {"active", source.value("active").toBool()},
        {"mode", text(source, "mode", "idle")},
        {"startedAtMs", number(source.value("startedAtMs"))},
        {"pointerMoves", snapshotValue(source, "pointerMoves")},
        {"controllerMutations", snapshotValue(source, "controllerMutations")},
        {"renderRequests", snapshotValue(source, "renderRequests")},
        {"hitTests", snapshotValue(source, "hitTests")},
        {"snapResolutions", snapshotValue(source, "snapResolutions")},
        {"handlePlans", snapshotValue(source, "handlePlans")},
        {"revisionStart", snapshotValue(source, "revisionStart")},
        {"revisionEnd", snapshotValue(source, "revisionEnd")},
        {"selectedCountStart", snapshotValue(source, "selectedCountStart")},
        {"selectedCountEnd", snapshotValue(source, "selectedCountEnd")},
        {"visibleObjectCountStart", snapshotValue(source, "visibleObjectCountStart")},
        {"visibleObjectCountEnd", snapshotValue(source, "visibleObjectCountEnd")},
        {"events", events},
    };
}

QVariantMap incrementMetric(const QVariantMap &state, const QString &field, double count, const QString &eventKind)
{
    QVariantMap next = cloneMetrics(state);
    if (!next.value("active").toBool()) {
        return next;
    }
    const double amount = eventCount(count);
    next.insert(field, std::max(0.0, number(next.value(field)) + amount));
    if (!eventKind.isEmpty()) {
        QVariantList events = toList(next.value("events"));
        events.push_back(QVariantMap{{"kind", eventKind}, {"count", amount}});
        next.insert("events", events);
    }
    return next;
}

QVariantMap finishMetrics(const QVariantMap &state, double timestampMs, const QVariantMap &snapshot)
{
    QVariantMap current = cloneMetrics(state);
    current.insert("revisionEnd", snapshotValue(snapshot, "revision"));
    current.insert("selectedCountEnd", snapshotValue(snapshot, "selectedCount"));
    current.insert("visibleObjectCountEnd", snapshotValue(snapshot, "visibleObjectCount"));
    const double finishedAtMs = std::isfinite(timestampMs) ? timestampMs : number(current.value("startedAtMs"));
    const QVariantMap record{
        {"mode", current.value("mode")},
        {"durationMs", std::max(0.0, finishedAtMs - number(current.value("startedAtMs")))},
        {"pointerMoves", current.value("pointerMoves")},
        {"controllerMutations", current.value("controllerMutations")},
        {"renderRequests", current.value("renderRequests")},
        {"hitTests", current.value("hitTests")},
        {"snapResolutions", current.value("snapResolutions")},
        {"handlePlans", current.value("handlePlans")},
        {"revisionStart", current.value("revisionStart")},
        {"revisionEnd", current.value("revisionEnd")},
        {"revisionDelta", number(current.value("revisionEnd")) - number(current.value("revisionStart"))},
        {"selectedCountStart", current.value("selectedCountStart")},
        {"selectedCountEnd", current.value("selectedCountEnd")},
        {"visibleObjectCountStart", current.value("visibleObjectCountStart")},
        {"visibleObjectCountEnd", current.value("visibleObjectCountEnd")},
        {"events", current.value("events")},
    };
    return {{"state", initialMetrics()}, {"record", record}};
}

void checkMetricMax(const QVariantMap &record, const QVariantMap &budget, const QString &budgetField, const QString &recordField, QStringList &failures)
{
    if (budget.contains(budgetField) && number(record.value(recordField)) > number(budget.value(budgetField))) {
        failures.push_back(QString("%1 expected <= %2, got %3").arg(recordField, budget.value(budgetField).toString(), record.value(recordField).toString()));
    }
}

void checkMetricEqual(const QVariantMap &record, const QVariantMap &budget, const QString &field, QStringList &failures)
{
    if (budget.contains(field) && number(record.value(field)) != number(budget.value(field))) {
        failures.push_back(QString("%1 expected %2, got %3").arg(field, budget.value(field).toString(), record.value(field).toString()));
    }
}

QVariantMap initialTelemetry()
{
    return {
        {"active", false},
        {"mode", "idle"},
        {"events", QVariantList{}},
        {"completedEvents", QVariantList{}},
    };
}

QVariantMap cloneTelemetry(const QVariantMap &state)
{
    const QVariantMap source = state.isEmpty() ? initialTelemetry() : state;
    return {
        {"active", source.value("active").toBool()},
        {"mode", text(source, "mode", "idle")},
        {"events", clonedEvents(source.value("events"))},
        {"completedEvents", clonedEvents(source.value("completedEvents"))},
    };
}

QVariantMap appendTelemetryEvent(const QVariantMap &state, const QVariantMap &event)
{
    QVariantMap next = cloneTelemetry(state);
    if (!next.value("active").toBool()) {
        return next;
    }
    QVariantList events = toList(next.value("events"));
    events.push_back(event);
    next.insert("events", events);
    return next;
}

QVariantMap finishTelemetry(const QVariantMap &state, const QString &type, double timestampMs, const QVariantMap &snapshot)
{
    QVariantMap current = cloneTelemetry(state);
    if (!current.value("active").toBool()) {
        return {{"state", current}, {"events", QVariantList{}}};
    }
    QVariantList events = toList(current.value("events"));
    events.push_back(QVariantMap{
        {"type", type},
        {"timestampMs", std::isfinite(timestampMs) ? timestampMs : 0.0},
        {"snapshot", normalizedSnapshot(snapshot)},
    });
    return {
        {"state", QVariantMap{{"active", false}, {"mode", "idle"}, {"events", QVariantList{}}, {"completedEvents", events}}},
        {"events", events},
    };
}

} // namespace

DrawingToolCatalog::DrawingToolCatalog(QObject *parent)
    : QObject(parent)
{
}

QVariantList DrawingToolCatalog::toolModes() const
{
    return list({
        item("select_move", "Select", "edit"),
        item("anchor_points", "Point", "snap"),
        item("line_polyline", "Line", "draw"),
        item("circle_arc", "Circle", "draw"),
        item("rectangle_polygon", "Rect", "shape"),
        item("regular_polygon", "Polygon", "shape"),
        item("image_reference_frame", "Image frame", "ascii"),
        item("ascii_crop_frame", "ASCII crop", "ascii"),
        item("ascii_cell_region", "ASCII cell region", "ascii"),
        item("tone_probe", "Tone probe", "ascii"),
        item("glyph_baseline", "Glyph baseline", "ascii"),
        item("spline_curve", "Spline / curve", "curve"),
        item("hatch_boundary", "Hatch / boundary", "region"),
        item("svg_fit", "Asset fit", "block"),
        item("offset_trim", "Offset / trim", "mod"),
        item("mirror_array", "Mirror / array", "mod"),
        item("measure_inspect", "Measure / inspect", "data"),
        item("layer_review", "Layer review", "std"),
        item("trace_markup", "Trace / markup", "review"),
    });
}

QVariantMap DrawingToolCatalog::toolSettingsById() const
{
    return {
        {"select_move", list({setting("Mode", "select and move existing objects"), setting("Hit test", "object bounding boxes first"), setting("Selection", "one object, one layer"), setting("Mutation", "disabled in shell v0")})},
        {"anchor_points", list({setting("Mode", "place and inspect named anchors"), setting("Snap", "grid, radial axes, artboard center"), setting("Required anchors", "root and tip"), setting("Point display", "selected anchor highlighted")})},
        {"svg_fit", list({setting("Mode", "fit SVG asset between two targets"), setting("Transform", "translate, scale, rotate"), setting("Mirror", "allowed by asset sidecar"), setting("Validation", "anchor_root and anchor_tip required")})},
        {"line_polyline", list({setting("Mode", "line and connected polyline drafting"), setting("Input", "absolute, relative, or polar point pairs"), setting("Snaps", "endpoint, midpoint, center, intersection"), setting("Output role", "construction paths or closed regions")})},
        {"circle_arc", list({setting("Mode", "circle or configured arc"), setting("Input", "click center, click radius point"), setting("Data", "store center, radius, start angle, end angle"), setting("Output role", "circle guides or arc boundaries")})},
        {"rectangle_polygon", list({setting("Mode", "rectangles, regular polygons, bounded cells"), setting("Input", "corner-corner, center-size, side count, radius"), setting("Pattern use", "tile units, frames, panels, medallion cells"), setting("Validation", "closed polygon and exact side/angle metadata")})},
        {"regular_polygon", list({setting("Mode", "regular polygon drafting"), setting("Input", "click center, click radius point"), setting("Controls", "side count and rotation are model-backed"), setting("Output", "closed polygon with exact side metadata")})},
        {"image_reference_frame", list({setting("Mode", "two-click frame for an image reference"), setting("Use", "place source image bounds before ASCII conversion"), setting("Output", "reference rectangle, not rendered source image"), setting("Next hook", "bind frame to input image path")})},
        {"ascii_crop_frame", list({setting("Mode", "two-click output crop frame"), setting("Use", "define the exact region sent to the ASCII workbench"), setting("Grid relation", "should align to character-cell columns and rows"), setting("Output", "crop rectangle for future CLI params")})},
        {"ascii_cell_region", list({setting("Mode", "two-click character-cell planning region"), setting("Use", "reserve a block for dense glyph rendering or annotation"), setting("Validation", "keep region inside crop frame when wired"), setting("Output", "region rectangle with ASCII role")})},
        {"tone_probe", list({setting("Mode", "single-click tone sample marker"), setting("Use", "mark areas to compare brightness, contrast, and glyph density"), setting("Future data", "sample luminance from image reference"), setting("Output", "named probe point")})},
        {"glyph_baseline", list({setting("Mode", "two-click baseline guide"), setting("Use", "align text/glyph flow or directional ASCII strokes"), setting("Future data", "angle and length for glyph placement"), setting("Output", "baseline segment")})},
        {"spline_curve", list({setting("Mode", "Bezier/spline curve drafting for ornament parts"), setting("Input", "control points with root/tip anchors"), setting("Math", "sampled curve, tangent, normal, bounds"), setting("Boundary", "V0 places curves; bend-along-path stays future")})},
        {"hatch_boundary", list({setting("Mode", "closed-region detection and fill/hatch preview"), setting("Input", "selected closed paths or generated boundary"), setting("Pattern use", "flat color fills, tile regions, negative space"), setting("Validation", "reject open regions and ambiguous self-crossing paths")})},
        {"offset_trim", list({setting("Mode", "derive parallel paths and cut back geometry"), setting("Offset", "used for borders, straps, grout, lineweight bands"), setting("Trim", "cut against selected boundary objects"), setting("Validation", "no dangling generated segments")})},
        {"mirror_array", list({setting("Mode", "mirror, rectangular array, polar array, path array"), setting("Pattern use", "rosettes, repeats, borders, radial petals"), setting("Origin", "anchor_center or selected base point"), setting("Validation", "symmetry target must match recipe")})},
        {"measure_inspect", list({setting("Mode", "measure distance, angle, radius, area, bounds"), setting("Object data", "layer, kind, coordinates, counts"), setting("Use", "recipe proof and visual rejection causes"), setting("Export", "manifest rows and diagnostic badges")})},
        {"layer_review", list({setting("Mode", "inspect and sort layers"), setting("Order", "canvas, grid, construction, fit, motif, anchors, markup, metadata"), setting("Visibility", "model layer flags"), setting("Standards", "layer naming, color, lineweight, role checker")})},
        {"trace_markup", list({setting("Mode", "review overlay without modifying source geometry"), setting("Markup", "comments, reject boxes, move/copy/delete notes"), setting("Trace", "separate layer, non-destructive"), setting("Use", "human review comments for Builder Dex")})},
    };
}

QVariantList DrawingToolCatalog::precisionTools() const
{
    return list({
        item("grid_snap", "Grid snap", "on"),
        item("object_snap", "Object snap", "end/mid/center/vertex"),
        item("object_snap_tracking", "Snap tracking", "project"),
        item("polar_tracking", "Polar tracking", "15 deg"),
        item("ortho_lock", "Ortho lock", "off"),
        item("coordinate_input", "Coordinates", "abs/rel/polar"),
        item("bounds_check", "Bounds check", "on"),
    });
}

QVariantList DrawingToolCatalog::dataTools() const
{
    return list({
        item("block_attributes", "Block attributes", "sidecar"),
        item("object_counts", "Object counts", "manifest"),
        item("standards_check", "Standards check", "layers"),
        item("trace_review", "Trace review", "comments"),
        item("command_macros", "Command macros", "recipes"),
    });
}

QVariantList DrawingToolCatalog::imageTools() const
{
    return list({item("image_to_ascii_workbench_v3", "Image to ASCII", "headless CLI")});
}

QVariantMap DrawingToolCatalog::externalToolSettingsById() const
{
    return {
        {"image_to_ascii_workbench_v3", list({
            setting("Status", "space reserved; CLI hook pending"),
            setting("Tool root", "/Users/kogaryu/gameguy-3d-lab/image_to_ascii_workbench_v3"),
            setting("First hook", "call CLI with image path and output paths"),
            setting("Output", "TXT, CP437, PNG preview"),
            setting("Size / tone", "width, height, cell aspect, brightness, contrast, gamma"),
            setting("Sampling", "center, average, median, super2x, super4x"),
            setting("Look", "dither, Sobel edge mode, palette, measured font darkness"),
        })},
    };
}

QVariantList DrawingToolCatalog::assetSources() const
{
    return list({
        item("ornament_codex", "ornament blocks", "SVG"),
        item("manual_plot_unions", "manual plot unions", "JSON"),
        item("approved_flower_parts", "approved flower parts", "review"),
        item("future_border_lane", "border block lane", "hold"),
    });
}

QVariantList DrawingToolCatalog::patternFamilies() const
{
    return list({
        item("lotus_floral_templates", "lotus / floral templates", "active"),
        item("radial_medallions", "radial medallions", "math"),
        item("circle_arc_fillers", "circle arc fillers", "math"),
        item("rectilinear_meanders", "rectilinear meanders", "math"),
        item("interlace_fields", "interlace fields", "math"),
        item("tile_filler_geometry", "tile filler geometry", "math"),
    });
}

QVariantList DrawingToolCatalog::toolPresets() const
{
    return list({
        item("lotus_petal_fit", "Lotus petal fit", "block"),
        item("radial_ring_8", "Polar array 8", "array"),
        item("border_segment_hold", "Border segment", "xref"),
        item("review_trace_note", "Trace review note", "markup"),
    });
}

QVariantList DrawingToolCatalog::layerStack() const
{
    QVariantList rows = list({
        item("layer_00_canvas", "canvas / artboard", "base"),
        item("layer_01_grid", "grid", "16x16"),
        item("layer_08_metadata", "metadata / counts", "data"),
        item("layer_09_script_geometry", "script geometry", "tests"),
    });
    rows[0].toMap()["visible"] = true;
    QVariantList result;
    for (int i = 0; i < rows.size(); ++i) {
        QVariantMap row = rows[i].toMap();
        row.insert("visible", i != 2);
        result.push_back(row);
    }
    return result;
}

QVariantList DrawingToolCatalog::sidebarSections() const
{
    return list({
        {{"id", "draw"}, {"title", "Draw"}, {"hint", "tools"}, {"source", "drawingToolModes"}, {"action", "tool"}, {"selectedProperty", "selectedDrawingToolId"}, {"ids", QStringList{"select_move", "anchor_points", "line_polyline", "circle_arc", "rectangle_polygon", "regular_polygon", "spline_curve"}}},
        {{"id", "modify"}, {"title", "Modify"}, {"hint", "ops"}, {"source", "drawingToolModes"}, {"action", "tool"}, {"selectedProperty", "selectedDrawingToolId"}, {"ids", QStringList{"offset_trim", "mirror_array", "hatch_boundary"}}},
        {{"id", "review"}, {"title", "Review"}, {"hint", "inspect"}, {"source", "drawingToolModes"}, {"action", "tool"}, {"selectedProperty", "selectedDrawingToolId"}, {"ids", QStringList{"measure_inspect", "layer_review", "trace_markup", "svg_fit"}}},
        {{"id", "ascii_draft"}, {"title", "ASCII Draft"}, {"hint", "tools"}, {"source", "drawingToolModes"}, {"action", "tool"}, {"selectedProperty", "selectedDrawingToolId"}, {"ids", QStringList{"image_reference_frame", "ascii_crop_frame", "ascii_cell_region", "tone_probe", "glyph_baseline"}}},
        {{"id", "precision"}, {"title", "Precision"}, {"hint", "snap"}, {"source", "drawingPrecisionTools"}, {"action", ""}, {"selectedProperty", ""}, {"ids", QStringList{}}},
        {{"id", "presets"}, {"title", "Presets"}, {"hint", "recipes"}, {"source", "drawingToolPresets"}, {"action", "preset"}, {"selectedProperty", "selectedDrawingPresetId"}, {"ids", QStringList{}}},
        {{"id", "assets"}, {"title", "Assets"}, {"hint", "codex"}, {"source", "drawingAssetSources"}, {"action", ""}, {"selectedProperty", ""}, {"ids", QStringList{}}},
        {{"id", "image_tools"}, {"title", "Image Tools"}, {"hint", "workbench"}, {"source", "drawingImageTools"}, {"action", "external_tool"}, {"selectedProperty", "selectedDrawingExternalToolId"}, {"ids", QStringList{}}},
        {{"id", "patterns"}, {"title", "Patterns"}, {"hint", "math"}, {"source", "drawingPatternFamilies"}, {"action", ""}, {"selectedProperty", ""}, {"ids", QStringList{}}},
        {{"id", "automation"}, {"title", "Data / Automation"}, {"hint", "scripts"}, {"source", "drawingDataTools"}, {"action", ""}, {"selectedProperty", ""}, {"ids", QStringList{}}},
        {{"id", "layers"}, {"title", "Layers"}, {"hint", "stack"}, {"source", "drawingLayerStack"}, {"action", "layer"}, {"selectedProperty", "selectedDrawingLayerId"}, {"ids", QStringList{}}},
    });
}

DrawingRuntimeRows::DrawingRuntimeRows(QObject *parent)
    : QObject(parent)
{
}

QVariantMap DrawingRuntimeRows::fitTransform(QObject *controller) const
{
    const QVariantMap root = anchorPoint(controller, "anchor_root");
    const QVariantMap tip = anchorPoint(controller, "anchor_tip");
    const double dx = number(tip.value("x")) - number(root.value("x"));
    const double dy = number(tip.value("y")) - number(root.value("y"));
    const double targetLength = std::sqrt(dx * dx + dy * dy);
    const double sourceDx = 0.0;
    const double sourceDy = -0.30;
    const double sourceLength = std::sqrt(sourceDx * sourceDx + sourceDy * sourceDy);
    const bool ok = targetLength > 0.001 && sourceLength > 0.001;
    const double rotationDeg = ok ? (std::atan2(dy, dx) - std::atan2(sourceDy, sourceDx)) * 180.0 / M_PI : 0.0;
    return {
        {"ok", ok},
        {"source_length", sourceLength},
        {"target_length", targetLength},
        {"scale", ok ? targetLength / sourceLength : 0.0},
        {"rotation_deg", rotationDeg},
        {"root_x", number(root.value("x"))},
        {"root_y", number(root.value("y"))},
        {"tip_x", number(tip.value("x"))},
        {"tip_y", number(tip.value("y"))},
        {"dx", dx},
        {"dy", dy},
    };
}

QString DrawingRuntimeRows::editNumber(const QVariant &value) const
{
    return editNumberValue(value);
}

QVariantList DrawingRuntimeRows::objectEditRows(QObject *controller) const
{
    const QVariantMap object = selectedObject(controller);
    const QString id = object.value("id").toString();
    if (!id.startsWith("script_")) {
        return {};
    }
    const QString kind = object.value("kind").toString();
    if (kind == "point" || kind == "tone_probe") {
        const QVariantList point = arrayField(object, "point_px");
        return list({editRow("X px", "x_px", point.value(0)), editRow("Y px", "y_px", point.value(1))});
    }
    if (kind == "line" || kind == "glyph_baseline") {
        const QVariantList from = arrayField(object, "from_px");
        const QVariantList to = arrayField(object, "to_px");
        return list({editRow("X1 px", "x1_px", from.value(0)), editRow("Y1 px", "y1_px", from.value(1)), editRow("X2 px", "x2_px", to.value(0)), editRow("Y2 px", "y2_px", to.value(1))});
    }
    if (kind == "circle") {
        const QVariantList center = arrayField(object, "center_px");
        return list({editRow("Center X px", "cx_px", center.value(0)), editRow("Center Y px", "cy_px", center.value(1)), editRow("Radius px", "radius_px", object.value("radius_px"))});
    }
    if (kind == "arc") {
        const QVariantList center = arrayField(object, "center_px");
        return list({editRow("Center X px", "cx_px", center.value(0)), editRow("Center Y px", "cy_px", center.value(1)), editRow("Radius px", "radius_px", object.value("radius_px")), editRow("Start deg", "start_angle_deg", object.value("start_angle_deg")), editRow("End deg", "end_angle_deg", object.value("end_angle_deg"))});
    }
    if (kind == "rectangle" || kind == "image_reference_frame" || kind == "ascii_crop_frame" || kind == "ascii_cell_region") {
        const QVariantList rect = arrayField(object, "rect_px");
        return list({editRow("X px", "x_px", rect.value(0)), editRow("Y px", "y_px", rect.value(1)), editRow("Width px", "width_px", rect.value(2)), editRow("Height px", "height_px", rect.value(3))});
    }
    if (kind == "polygon") {
        const QVariantList center = arrayField(object, "center_px");
        return list({editRow("Center X px", "cx_px", center.value(0)), editRow("Center Y px", "cy_px", center.value(1)), editRow("Radius px", "radius_px", object.value("radius_px")), editRow("Sides", "sides", object.value("sides")), editRow("Rotation deg", "rotation_deg", object.value("rotation_deg"))});
    }
    return {};
}

QVariantList DrawingRuntimeRows::inspectorRows(QObject *controller) const
{
    const QVariantMap tool = selectedTool(controller);
    const QVariantMap layer = selectedLayer(controller);
    const QVariantMap object = selectedObject(controller);
    const QString objectId = object.value("id").toString();
    const bool hasObject = !objectId.isEmpty();
    QVariantList rows = {
        row("Tool", text(tool, "label", controller->property("selectedDrawingToolId").toString())),
        row("Layer", text(layer, "label", controller->property("selectedDrawingLayerId").toString())),
        row("Object", hasObject ? text(object, "label", controller->property("selectedDrawingObjectId").toString()) : QStringLiteral("none")),
        row("Kind", hasObject ? text(object, "kind", "unknown") : QStringLiteral("none")),
        row("Detail", hasObject ? text(object, "detail", "model-backed drawing object") : QStringLiteral("No object selected")),
    };
    appendInspectorRows(rows, object);
    rows.push_back(row("Fit status", fitTransform(controller).value("ok").toBool() ? "root-tip transform valid" : "invalid root-tip vector"));
    return rows;
}

QVariantList DrawingRuntimeRows::toolSettingsRows(QObject *controller) const
{
    const QVariantMap settings = toMap(controller->property("drawingToolSettingsById"));
    QVariantList result = toList(settings.value(controller->property("selectedDrawingToolId").toString()));
    if (result.isEmpty()) {
        result = toList(settings.value("select_move"));
    }
    const QString selectedToolId = controller->property("selectedDrawingToolId").toString();
    if (selectedToolId == "circle_arc") {
        result.push_back(row("Current mode", controller->property("drawingCircleArcMode").toString()));
        result.push_back(row("Arc span", QString("%1 -> %2 deg").arg(editNumberValue(controller->property("drawingCircleArcStartAngleDeg")), editNumberValue(controller->property("drawingCircleArcEndAngleDeg")))));
    } else if (selectedToolId == "regular_polygon") {
        result.push_back(row("Sides", controller->property("drawingRegularPolygonSides").toString()));
        result.push_back(row("Rotation", editNumberValue(controller->property("drawingRegularPolygonRotationDeg")) + " deg"));
    }
    return result;
}

QVariantList DrawingRuntimeRows::toolParameterEditRows(QObject *controller) const
{
    const QString selectedToolId = controller->property("selectedDrawingToolId").toString();
    if (selectedToolId == "circle_arc") {
        return list({editRow("Mode", "circle_arc_mode", controller->property("drawingCircleArcMode"), false), editRow("Start deg", "circle_arc_start_angle_deg", controller->property("drawingCircleArcStartAngleDeg")), editRow("End deg", "circle_arc_end_angle_deg", controller->property("drawingCircleArcEndAngleDeg"))});
    }
    if (selectedToolId == "regular_polygon") {
        return list({editRow("Sides", "regular_polygon_sides", controller->property("drawingRegularPolygonSides")), editRow("Rotation deg", "regular_polygon_rotation_deg", controller->property("drawingRegularPolygonRotationDeg"))});
    }
    return {};
}

QVariantList DrawingRuntimeRows::modelValidationRows(QObject *controller) const
{
    const QVariantMap model = modelDocument(controller);
    if (!model.isEmpty()) {
        return toList(model.value("validation"));
    }
    QVariantList rows;
    const QVariantList generated = toList(controller->property("drawingGeneratedObjects"));
    rows.push_back(QVariantMap{{"id", "script_status"}, {"status", controller->property("drawingLastScriptStatus")}, {"detail", toList(controller->property("drawingLastScriptErrors")).isEmpty() ? "no replay errors" : "replay errors"}});
    rows.push_back(QVariantMap{{"id", "generated_count"}, {"status", generated.isEmpty() ? "empty" : "pass"}, {"detail", QString::number(generated.size())}});
    for (const QVariant &value : generated) {
        const QVariantMap object = toMap(value);
        bool finite = true;
        for (const QString &key : {"x1", "y1", "x2", "y2"}) {
            const double coordinate = number(object.value(key), std::numeric_limits<double>::quiet_NaN());
            if (!std::isfinite(coordinate) || coordinate < 0.0 || coordinate > 1.0) {
                finite = false;
            }
        }
        rows.push_back(QVariantMap{{"id", object.value("id").toString() + "_bounds"}, {"status", finite ? "pass" : "fail"}, {"detail", finite ? "inside normalized artboard" : "invalid coordinate"}});
    }
    return rows;
}

QVariantList DrawingRuntimeRows::externalToolRows(QObject *controller) const
{
    return toList(toMap(controller->property("drawingExternalToolSettingsById")).value(controller->property("selectedDrawingExternalToolId").toString()));
}

QVariantList DrawingRuntimeRows::sidebarRows(QObject *controller, const QVariantMap &section) const
{
    if (section.isEmpty()) {
        return {};
    }
    const QVariantList source = toList(controller->property(section.value("source").toString().toUtf8().constData()));
    const QVariantList ids = toList(section.value("ids"));
    if (ids.isEmpty()) {
        return source;
    }
    QVariantMap rowsById;
    for (const QVariant &value : source) {
        const QVariantMap row = toMap(value);
        rowsById.insert(row.value("id").toString(), row);
    }
    QVariantList rows;
    for (const QVariant &id : ids) {
        const QVariant row = rowsById.value(id.toString());
        if (row.isValid()) {
            rows.push_back(row);
        }
    }
    return rows;
}

bool DrawingRuntimeRows::sidebarRowSelected(QObject *controller, const QVariantMap &section, const QVariantMap &row) const
{
    if (section.isEmpty() || row.isEmpty() || section.value("selectedProperty").toString().isEmpty()) {
        return false;
    }
    return controller->property(section.value("selectedProperty").toString().toUtf8().constData()).toString() == row.value("id").toString();
}

bool DrawingRuntimeRows::sidebarRowClickable(const QVariantMap &section) const
{
    return !section.value("action").toString().isEmpty();
}

QVariantList DrawingRuntimeRows::toolPaletteRows(QObject *controller) const
{
    const QVariantMap preset = selectedPreset(controller);
    return {
        row("Tool", text(selectedTool(controller), "label", controller->property("selectedDrawingToolId").toString())),
        row("Preset", text(preset, "label", controller->property("selectedDrawingPresetId").toString())),
        row("Snap", "grid + object + polar"),
        row("Layer", text(selectedLayer(controller), "label", controller->property("selectedDrawingLayerId").toString())),
        row("Object", text(selectedObject(controller), "label", controller->property("selectedDrawingObjectId").toString())),
    };
}

QVariantList DrawingRuntimeRows::validationRows(QObject *controller) const
{
    const QVariantMap model = modelDocument(controller);
    if (!model.isEmpty()) {
        QVariantList rows = {row("State source", "C++ DrawingDocumentController"), row("Engine", text(model, "engine", "cpp_drawing_core_v1")), row("Status", text(model, "script_status", "unknown"))};
        for (const QVariant &value : toList(model.value("validation"))) {
            const QVariantMap validation = toMap(value);
            rows.push_back(row(text(validation, "id", "validation"), validation.value("status").toString() + " / " + validation.value("detail").toString()));
        }
        return rows;
    }
    return {
        row("State source", "runtime controller"),
        row("Canvas model", "drawingCanvasDocument()"),
        row("Fit transform", fitTransform(controller).value("ok").toBool() ? "valid" : "invalid"),
        row("Writes", controller->property("writeDisabled").toBool() ? "disabled" : "enabled"),
        row("Layer count", QString::number(toList(controller->property("drawingLayerStack")).size())),
        row("Object count", QString::number(toList(invokeWithVariant(controller, "drawingCanvasObjects", controller->property("revision"))).size())),
    };
}

QVariantList DrawingRuntimeRows::modelObjectRows(QObject *controller) const
{
    const QVariantList generated = toList(controller->property("drawingGeneratedObjects"));
    QVariantList rows;
    for (int i = generated.size() - 1; i >= 0; --i) {
        const QVariantMap object = toMap(generated[i]);
        rows.push_back(QVariantMap{{"id", object.value("id").toString()}, {"label", text(object, "id", "object")}, {"meta", text(object, "kind", "unknown")}, {"selected", object.value("id").toString() == controller->property("selectedDrawingObjectId").toString()}});
    }
    if (rows.isEmpty()) {
        rows.push_back(QVariantMap{{"id", ""}, {"label", "No model objects"}, {"meta", "draw something"}, {"selected", false}});
    }
    return rows;
}

QVariantList DrawingRuntimeRows::logRows(QObject *controller) const
{
    const QVariantMap model = modelDocument(controller);
    if (!model.isEmpty()) {
        const QVariantList commands = toList(model.value("command_log"));
        const QString lastCommand = commands.isEmpty() ? QStringLiteral("none") : text(toMap(commands.last()), "cmd", "unknown");
        return {
            row("Native document", "active"),
            row("Selected tool", text(selectedTool(controller), "label", controller->property("selectedDrawingToolId").toString())),
            row("Selected object", text(model, "selected_object_id", "none")),
            row("Script status", text(model, "script_status", "not_run")),
            row("Model objects", QString::number(toList(model.value("generated_objects")).size())),
            row("Commands", QString::number(commands.size())),
            row("Last command", lastCommand),
        };
    }
    const QVariantMap fit = fitTransform(controller);
    return {
        row("Native shell booted", "ok"),
        row("Selected tool", text(selectedTool(controller), "label", controller->property("selectedDrawingToolId").toString())),
        row("Selected layer", text(selectedLayer(controller), "label", controller->property("selectedDrawingLayerId").toString())),
        row("Selected object", text(selectedObject(controller), "label", controller->property("selectedDrawingObjectId").toString())),
        row("Fit scale", QString::number(number(fit.value("scale")), 'f', 3)),
        row("Fit rotation", QString::number(number(fit.value("rotation_deg")), 'f', 1) + " deg"),
    };
}

QVariantList DrawingRuntimeRows::exportRows(QObject *controller) const
{
    if (!modelDocument(controller).isEmpty()) {
        return {row("Model JSON", "native exportJson() ready"), row("SVG", "native exportSvg() ready"), row("Interactive writes", "in-memory document")};
    }
    return {row("Model JSON", "runtime fallback document"), row("SVG", "native controller required"), row("Interactive writes", controller->property("writeDisabled").toBool() ? "disabled" : "enabled")};
}

QVariantList DrawingRuntimeRows::manifestRows(QObject *controller) const
{
    const QVariantMap model = modelDocument(controller);
    if (!model.isEmpty()) {
        const QVariantMap snap = toMap(model.value("snap"));
        return {
            row("Export kind", model.value("export_kind").toString()),
            row("Engine", model.value("engine").toString()),
            row("Canvas", joinList(toList(model.value("canvas_px"))).replace(", ", " x ")),
            row("Counts", objectCountsText(toMap(model.value("object_counts")))),
            row("Commands", model.value("command_count").isValid() ? model.value("command_count").toString() : QString::number(toList(model.value("command_log")).size())),
            row("Snap", QString("%1 %2").arg(snap.value("grid_enabled").toBool() ? "grid" : "off", snap.value("grid_step_px").toString())),
        };
    }
    const QVariant counts = invokeWithVariant(controller, "drawingObjectCounts", controller->property("revision"));
    const QString countsText = QString::fromUtf8(QJsonDocument(QJsonObject::fromVariantMap(toMap(counts))).toJson(QJsonDocument::Compact));
    return {
        row("Recipe-first", "required"),
        row("Layer metadata", "model-backed"),
        row("Object counts", countsText),
        row("Validation rows", QString::number(modelValidationRows(controller).size())),
        row("Native controller", objectProperty(controller, "drawingNativeController") != nullptr ? "active" : "missing"),
    };
}

DrawingInteractionRuntime::DrawingInteractionRuntime(QObject *parent)
    : QObject(parent)
{
}

QVariantMap DrawingInteractionRuntime::initialMetricsState() const
{
    return initialMetrics();
}

QVariantMap DrawingInteractionRuntime::beginMetricsInteraction(const QVariantMap &, const QString &mode, double timestampMs, const QVariantMap &snapshot) const
{
    QVariantMap next = initialMetrics();
    next.insert("active", true);
    next.insert("mode", mode.isEmpty() ? QStringLiteral("idle") : mode);
    next.insert("startedAtMs", std::isfinite(timestampMs) ? timestampMs : 0.0);
    next.insert("revisionStart", snapshotValue(snapshot, "revision"));
    next.insert("revisionEnd", next.value("revisionStart"));
    next.insert("selectedCountStart", snapshotValue(snapshot, "selectedCount"));
    next.insert("selectedCountEnd", next.value("selectedCountStart"));
    next.insert("visibleObjectCountStart", snapshotValue(snapshot, "visibleObjectCount"));
    next.insert("visibleObjectCountEnd", next.value("visibleObjectCountStart"));
    return next;
}

QVariantMap DrawingInteractionRuntime::recordMetricsPointerMove(const QVariantMap &state, double count) const
{
    return incrementMetric(state, "pointerMoves", count, "pointer_move");
}

QVariantMap DrawingInteractionRuntime::recordMetricsControllerMutation(const QVariantMap &state, const QString &kind, double count) const
{
    return incrementMetric(state, "controllerMutations", count, kind.isEmpty() ? "controller_mutation" : kind);
}

QVariantMap DrawingInteractionRuntime::recordMetricsRenderRequest(const QVariantMap &state, double count) const
{
    return incrementMetric(state, "renderRequests", count, "render_request");
}

QVariantMap DrawingInteractionRuntime::recordMetricsHitTest(const QVariantMap &state, double count) const
{
    return incrementMetric(state, "hitTests", count, "hit_test");
}

QVariantMap DrawingInteractionRuntime::recordMetricsSnap(const QVariantMap &state, double count) const
{
    return incrementMetric(state, "snapResolutions", count, "snap_resolution");
}

QVariantMap DrawingInteractionRuntime::recordMetricsHandlePlan(const QVariantMap &state, double count) const
{
    return incrementMetric(state, "handlePlans", count, "handle_plan");
}

QVariantMap DrawingInteractionRuntime::finishMetricsInteraction(const QVariantMap &state, double timestampMs, const QVariantMap &snapshot) const
{
    return finishMetrics(state, timestampMs, snapshot);
}

QVariantMap DrawingInteractionRuntime::cancelMetricsInteraction(const QVariantMap &state, double timestampMs, const QVariantMap &snapshot) const
{
    QVariantMap finished = finishMetrics(state, timestampMs, snapshot);
    QVariantMap record = toMap(finished.value("record"));
    record.insert("canceled", true);
    finished.insert("record", record);
    return finished;
}

QVariantMap DrawingInteractionRuntime::assertWithinBudget(const QVariantMap &record, const QVariantMap &budget) const
{
    QStringList failures;
    if (budget.contains("mode") && record.value("mode").toString() != budget.value("mode").toString()) {
        failures.push_back(QString("mode expected %1, got %2").arg(budget.value("mode").toString(), record.value("mode").toString()));
    }
    checkMetricMax(record, budget, "maxDurationMs", "durationMs", failures);
    checkMetricMax(record, budget, "maxPointerMoves", "pointerMoves", failures);
    checkMetricMax(record, budget, "maxControllerMutations", "controllerMutations", failures);
    checkMetricMax(record, budget, "maxRenderRequests", "renderRequests", failures);
    checkMetricMax(record, budget, "maxHitTests", "hitTests", failures);
    checkMetricMax(record, budget, "maxSnapResolutions", "snapResolutions", failures);
    checkMetricMax(record, budget, "maxHandlePlans", "handlePlans", failures);
    checkMetricEqual(record, budget, "revisionDelta", failures);
    return {{"ok", failures.isEmpty()}, {"failures", failures}};
}

QVariantMap DrawingInteractionRuntime::initialTelemetryState() const
{
    return initialTelemetry();
}

QVariantMap DrawingInteractionRuntime::beginTelemetryInteraction(const QVariantMap &state, const QString &mode, double timestampMs, const QVariantMap &snapshot) const
{
    QVariantMap next = initialTelemetry();
    next.insert("active", true);
    next.insert("mode", mode.isEmpty() ? QStringLiteral("idle") : mode);
    next.insert("completedEvents", clonedEvents(state.value("completedEvents")));
    QVariantList events;
    events.push_back(QVariantMap{
        {"type", "begin"},
        {"mode", next.value("mode")},
        {"timestampMs", std::isfinite(timestampMs) ? timestampMs : 0.0},
        {"snapshot", normalizedSnapshot(snapshot)},
    });
    next.insert("events", events);
    return next;
}

QVariantMap DrawingInteractionRuntime::recordTelemetryPointerMove(const QVariantMap &state, double count) const
{
    return appendTelemetryEvent(state, {{"type", "pointerMove"}, {"count", eventCount(count)}});
}

QVariantMap DrawingInteractionRuntime::recordTelemetryControllerMutation(const QVariantMap &state, const QString &kind, double count) const
{
    return appendTelemetryEvent(state, {{"type", "controllerMutation"}, {"kind", kind.isEmpty() ? "controller_mutation" : kind}, {"count", eventCount(count)}});
}

QVariantMap DrawingInteractionRuntime::recordTelemetryRenderRequest(const QVariantMap &state, double count) const
{
    return appendTelemetryEvent(state, {{"type", "renderRequest"}, {"count", eventCount(count)}});
}

QVariantMap DrawingInteractionRuntime::recordTelemetryHitTest(const QVariantMap &state, double count) const
{
    return appendTelemetryEvent(state, {{"type", "hitTest"}, {"count", eventCount(count)}});
}

QVariantMap DrawingInteractionRuntime::recordTelemetrySnap(const QVariantMap &state, double count) const
{
    return appendTelemetryEvent(state, {{"type", "snap"}, {"count", eventCount(count)}});
}

QVariantMap DrawingInteractionRuntime::recordTelemetryHandlePlan(const QVariantMap &state, double count) const
{
    return appendTelemetryEvent(state, {{"type", "handlePlan"}, {"count", eventCount(count)}});
}

QVariantMap DrawingInteractionRuntime::finishTelemetryInteraction(const QVariantMap &state, double timestampMs, const QVariantMap &snapshot) const
{
    return finishTelemetry(state, "finish", timestampMs, snapshot);
}

QVariantMap DrawingInteractionRuntime::cancelTelemetryInteraction(const QVariantMap &state, double timestampMs, const QVariantMap &snapshot) const
{
    return finishTelemetry(state, "cancel", timestampMs, snapshot);
}
