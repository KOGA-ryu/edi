#include "drafting/DraftingSerialize.h"

#include "drafting/DraftingGeometry.h"
#include "drafting/DraftingMeasurement.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <utility>
#include <vector>

namespace edi::drafting {

using edi::formats::ByteBuffer;
using edi::formats::FormatResult;
using edi::formats::FormatResultCode;
using edi::formats::MsgPackValue;

namespace {

// --- inverse name parsers (shapeKindFromName moved to the public API when
// the inspector plan needed it too; the remaining parsers are decode-only) ---

bool isKnownShapeKindName(const std::string &name)
{
    return name == "point" || name == "line" || name == "rectangle" || name == "circle"
        || name == "arc" || name == "ellipse" || name == "polygon" || name == "polyline" || name == "guide"
        || name == "construction_line" || name == "dimension" || name == "text" || name == "spline";
}

GuideOrientation guideOrientationFromName(const std::string &name)
{
    return name == "vertical" ? GuideOrientation::Vertical : GuideOrientation::Horizontal;
}

DimensionKind dimensionKindFromName(const std::string &name)
{
    if (name == "width") return DimensionKind::Width;
    if (name == "height") return DimensionKind::Height;
    if (name == "radius") return DimensionKind::Radius;
    if (name == "diameter") return DimensionKind::Diameter;
    return DimensionKind::Distance;
}

MeasurementUnit measurementUnitFromName(const std::string &name)
{
    if (name == "canvas_unit") return MeasurementUnit::CanvasUnit;
    if (name == "millimeter") return MeasurementUnit::Millimeter;
    if (name == "centimeter") return MeasurementUnit::Centimeter;
    if (name == "meter") return MeasurementUnit::Meter;
    if (name == "inch") return MeasurementUnit::Inch;
    if (name == "foot") return MeasurementUnit::Foot;
    return MeasurementUnit::None;
}

// --- typed accessors over MsgPackValue (tolerant, fall back on type mismatch)

double asDouble(const MsgPackValue *v, double fallback)
{
    if (!v) return fallback;
    if (v->type == MsgPackValue::Type::Double) return v->doubleValue;
    if (v->type == MsgPackValue::Type::Int) return static_cast<double>(v->intValue);
    return fallback;
}

std::int64_t asInt(const MsgPackValue *v, std::int64_t fallback)
{
    if (!v) return fallback;
    if (v->type == MsgPackValue::Type::Int) return v->intValue;
    if (v->type == MsgPackValue::Type::Double) return static_cast<std::int64_t>(v->doubleValue);
    return fallback;
}

bool asBool(const MsgPackValue *v, bool fallback)
{
    if (v && v->type == MsgPackValue::Type::Bool) return v->boolValue;
    return fallback;
}

std::string asString(const MsgPackValue *v, const std::string &fallback)
{
    if (v && v->type == MsgPackValue::Type::String) return v->stringValue;
    return fallback;
}

const MsgPackValue *child(const MsgPackValue &map, const std::string &key)
{
    return map.type == MsgPackValue::Type::Map ? map.find(key) : nullptr;
}

// --- Point2D <-> 2-element array --------------------------------------------

MsgPackValue pointValue(const Point2D &p)
{
    return MsgPackValue::array({MsgPackValue::number(p.x), MsgPackValue::number(p.y)});
}

Point2D readPoint(const MsgPackValue *v)
{
    Point2D p;
    if (v && v->type == MsgPackValue::Type::Array && v->arrayValue.size() == 2) {
        p.x = asDouble(&v->arrayValue[0], 0.0);
        p.y = asDouble(&v->arrayValue[1], 0.0);
    }
    return p;
}

MsgPackValue verticesValue(const std::vector<Point2D> &vertices)
{
    std::vector<MsgPackValue> items;
    items.reserve(vertices.size());
    for (const auto &p : vertices) {
        items.push_back(pointValue(p));
    }
    return MsgPackValue::array(std::move(items));
}

std::vector<Point2D> readVertices(const MsgPackValue *v)
{
    std::vector<Point2D> vertices;
    if (v && v->type == MsgPackValue::Type::Array) {
        for (const auto &item : v->arrayValue) {
            vertices.push_back(readPoint(&item));
        }
    }
    return vertices;
}

// --- geometry per variant alternative ---------------------------------------

MsgPackValue geometryValue(const DraftingGeometry &geometry)
{
    std::vector<std::pair<std::string, MsgPackValue>> fields;
    std::visit([&](const auto &g) {
        using G = std::decay_t<decltype(g)>;
        if constexpr (std::is_same_v<G, PointGeometry>) {
            fields.emplace_back("point", pointValue(g.point));
        } else if constexpr (std::is_same_v<G, LineGeometry>) {
            fields.emplace_back("a", pointValue(g.a));
            fields.emplace_back("b", pointValue(g.b));
        } else if constexpr (std::is_same_v<G, RectangleGeometry>) {
            fields.emplace_back("origin", pointValue(g.origin));
            fields.emplace_back("width", MsgPackValue::number(g.width));
            fields.emplace_back("height", MsgPackValue::number(g.height));
            fields.emplace_back("rotation_deg", MsgPackValue::number(g.rotationDeg));
            fields.emplace_back("corner_radius", MsgPackValue::number(g.cornerRadius));
            fields.emplace_back("inset", MsgPackValue::number(g.inset));
        } else if constexpr (std::is_same_v<G, CircleGeometry>) {
            fields.emplace_back("center", pointValue(g.center));
            fields.emplace_back("radius", MsgPackValue::number(g.radius));
        } else if constexpr (std::is_same_v<G, ArcGeometry>) {
            fields.emplace_back("center", pointValue(g.center));
            fields.emplace_back("radius", MsgPackValue::number(g.radius));
            fields.emplace_back("start_angle_deg", MsgPackValue::number(g.startAngleDeg));
            fields.emplace_back("end_angle_deg", MsgPackValue::number(g.endAngleDeg));
        } else if constexpr (std::is_same_v<G, EllipseGeometry>) {
            fields.emplace_back("center", pointValue(g.center));
            fields.emplace_back("rx", MsgPackValue::number(g.rx));
            fields.emplace_back("ry", MsgPackValue::number(g.ry));
        } else if constexpr (std::is_same_v<G, PolygonGeometry>) {
            fields.emplace_back("vertices", verticesValue(g.vertices));
        } else if constexpr (std::is_same_v<G, PolylineGeometry>) {
            fields.emplace_back("vertices", verticesValue(g.vertices));
        } else if constexpr (std::is_same_v<G, GuideGeometry>) {
            fields.emplace_back("orientation", MsgPackValue::text(guideOrientationName(g.orientation)));
            fields.emplace_back("position", MsgPackValue::number(g.position));
        } else if constexpr (std::is_same_v<G, ConstructionLineGeometry>) {
            fields.emplace_back("a", pointValue(g.a));
            fields.emplace_back("b", pointValue(g.b));
        } else if constexpr (std::is_same_v<G, DimensionGeometry>) {
            fields.emplace_back("kind", MsgPackValue::text(dimensionKindName(g.kind)));
            fields.emplace_back("a", pointValue(g.a));
            fields.emplace_back("b", pointValue(g.b));
            fields.emplace_back("offset", MsgPackValue::number(g.offset));
        } else if constexpr (std::is_same_v<G, TextAnnotationGeometry>) {
            fields.emplace_back("position", pointValue(g.position));
            fields.emplace_back("content", MsgPackValue::text(g.content));
            fields.emplace_back("height", MsgPackValue::number(g.height));
        } else if constexpr (std::is_same_v<G, SplineGeometry>) {
            fields.emplace_back("control_points", verticesValue(g.controlPoints));
        } else {
            static_assert(always_false_v<G>, "geometryValue: unhandled geometry kind — add an arm");
        }
    }, geometry);
    return MsgPackValue::map(std::move(fields));
}

DraftingGeometry readGeometry(DraftingShapeKind kind, const MsgPackValue &g)
{
    switch (kind) {
    case DraftingShapeKind::Point: {
        PointGeometry geo;
        geo.point = readPoint(child(g, "point"));
        return geo;
    }
    case DraftingShapeKind::Line: {
        LineGeometry geo;
        geo.a = readPoint(child(g, "a"));
        geo.b = readPoint(child(g, "b"));
        return geo;
    }
    case DraftingShapeKind::Rectangle: {
        RectangleGeometry geo;
        geo.origin = readPoint(child(g, "origin"));
        geo.width = asDouble(child(g, "width"), 0.0);
        geo.height = asDouble(child(g, "height"), 0.0);
        geo.rotationDeg = asDouble(child(g, "rotation_deg"), 0.0);
        geo.cornerRadius = asDouble(child(g, "corner_radius"), 0.0);
        geo.inset = asDouble(child(g, "inset"), 0.0);
        return geo;
    }
    case DraftingShapeKind::Circle: {
        CircleGeometry geo;
        geo.center = readPoint(child(g, "center"));
        geo.radius = asDouble(child(g, "radius"), 0.0);
        return geo;
    }
    case DraftingShapeKind::Arc: {
        ArcGeometry geo;
        geo.center = readPoint(child(g, "center"));
        geo.radius = asDouble(child(g, "radius"), 0.0);
        geo.startAngleDeg = asDouble(child(g, "start_angle_deg"), 0.0);
        geo.endAngleDeg = asDouble(child(g, "end_angle_deg"), 0.0);
        return geo;
    }
    case DraftingShapeKind::Ellipse: {
        EllipseGeometry geo;
        geo.center = readPoint(child(g, "center"));
        geo.rx = asDouble(child(g, "rx"), 0.0);
        geo.ry = asDouble(child(g, "ry"), 0.0);
        return geo;
    }
    case DraftingShapeKind::Polygon: {
        PolygonGeometry geo;
        geo.vertices = readVertices(child(g, "vertices"));
        return geo;
    }
    case DraftingShapeKind::Polyline: {
        PolylineGeometry geo;
        geo.vertices = readVertices(child(g, "vertices"));
        return geo;
    }
    case DraftingShapeKind::Spline: {
        SplineGeometry geo;
        geo.controlPoints = readVertices(child(g, "control_points"));
        return geo;
    }
    case DraftingShapeKind::Guide: {
        GuideGeometry geo;
        geo.orientation = guideOrientationFromName(asString(child(g, "orientation"), "horizontal"));
        geo.position = asDouble(child(g, "position"), 0.0);
        return geo;
    }
    case DraftingShapeKind::ConstructionLine: {
        ConstructionLineGeometry geo;
        geo.a = readPoint(child(g, "a"));
        geo.b = readPoint(child(g, "b"));
        return geo;
    }
    case DraftingShapeKind::Dimension: {
        DimensionGeometry geo;
        geo.kind = dimensionKindFromName(asString(child(g, "kind"), "distance"));
        geo.a = readPoint(child(g, "a"));
        geo.b = readPoint(child(g, "b"));
        geo.offset = asDouble(child(g, "offset"), 0.04);
        return geo;
    }
    case DraftingShapeKind::TextAnnotation: {
        TextAnnotationGeometry geo;
        geo.position = readPoint(child(g, "position"));
        geo.content = asString(child(g, "content"), "");
        geo.height = asDouble(child(g, "height"), 0.04);
        return geo;
    }
    }
    return PointGeometry{};
}

// --- styles / transform / metadata ------------------------------------------

MsgPackValue strokeValue(const StrokeStyle &s)
{
    return MsgPackValue::map({
        {"width", MsgPackValue::number(s.width)},
        {"opacity", MsgPackValue::number(s.opacity)},
        {"color", MsgPackValue::text(s.color)},
        {"line_style", MsgPackValue::text(s.lineStyle)},
    });
}

StrokeStyle readStroke(const MsgPackValue *v, int documentVersion)
{
    StrokeStyle s;
    if (v) {
        s.width = asDouble(child(*v, "width"), s.width);
        // Clamp at the read seam: a file is external input, and every
        // consumer downstream (projection keys, the inspector spin, SVG's
        // emit gate) otherwise sees the raw value. NaN/inf mean "no usable
        // opinion" -> fully opaque.
        const double rawOpacity = asDouble(child(*v, "opacity"), s.opacity);
        s.opacity = std::isfinite(rawOpacity) ? std::clamp(rawOpacity, 0.0, 1.0) : 1.0;
        s.color = asString(child(*v, "color"), s.color);
        s.lineStyle = asString(child(*v, "line_style"), s.lineStyle);
        // VERSION-1 shim only: before per-object styling shipped, every
        // stored stroke carried the never-writable defaults ("#000000",
        // width 1) — they mean "no opinion", so they map to the inherit
        // sentinels instead of turning old drawings black. From version 2
        // on, a black 1px stroke is a choice a user can make, and the
        // review caught the unversioned shim eating it on reload.
        if (documentVersion < 2 && s.color == "#000000" && s.width == 1.0) {
            s.color.clear();
            s.width = 0.0;
        }
    }
    return s;
}

MsgPackValue fillValue(const FillStyle &f)
{
    return MsgPackValue::map({
        {"opacity", MsgPackValue::number(f.opacity)},
        {"color", MsgPackValue::text(f.color)},
    });
}

FillStyle readFill(const MsgPackValue *v)
{
    FillStyle f;
    if (v) {
        // Same read-seam clamp as readStroke: untrusted alpha never escapes.
        const double rawOpacity = asDouble(child(*v, "opacity"), f.opacity);
        f.opacity = std::isfinite(rawOpacity) ? std::clamp(rawOpacity, 0.0, 1.0) : 1.0;
        f.color = asString(child(*v, "color"), f.color);
    }
    return f;
}

MsgPackValue transformValue(const Transform2D &t)
{
    return MsgPackValue::map({
        {"translate_x", MsgPackValue::number(t.translateX)},
        {"translate_y", MsgPackValue::number(t.translateY)},
        {"scale_x", MsgPackValue::number(t.scaleX)},
        {"scale_y", MsgPackValue::number(t.scaleY)},
        {"rotation_deg", MsgPackValue::number(t.rotationDeg)},
    });
}

Transform2D readTransform(const MsgPackValue *v)
{
    Transform2D t;
    if (v) {
        t.translateX = asDouble(child(*v, "translate_x"), t.translateX);
        t.translateY = asDouble(child(*v, "translate_y"), t.translateY);
        t.scaleX = asDouble(child(*v, "scale_x"), t.scaleX);
        t.scaleY = asDouble(child(*v, "scale_y"), t.scaleY);
        t.rotationDeg = asDouble(child(*v, "rotation_deg"), t.rotationDeg);
    }
    return t;
}

MsgPackValue metadataValue(const ObjectMetadata &m)
{
    MsgPackValue measurement = MsgPackValue::map({
        {"unit", MsgPackValue::text(measurementUnitName(m.measurement.unit))},
        {"canvas_units_per_real_unit", MsgPackValue::number(m.measurement.canvasUnitsPerRealUnit)},
        {"label", MsgPackValue::text(m.measurement.label)},
    });
    MsgPackValue guideVisual = MsgPackValue::map({
        {"label", MsgPackValue::text(m.guideVisual.label)},
        {"color", MsgPackValue::text(m.guideVisual.color)},
        {"dash_style", MsgPackValue::text(m.guideVisual.dashStyle)},
        {"show_label", MsgPackValue::boolean(m.guideVisual.showLabel)},
    });
    MsgPackValue dimensionVisual = MsgPackValue::map({
        {"show_label", MsgPackValue::boolean(m.dimensionVisual.showLabel)},
    });
    MsgPackValue lineVisual = MsgPackValue::map({
        {"end_arrow", MsgPackValue::boolean(m.lineVisual.endArrow)},
        {"start_arrow", MsgPackValue::boolean(m.lineVisual.startArrow)},
    });
    std::vector<MsgPackValue> tagItems;
    tagItems.reserve(m.tags.size());
    for (const std::string &tag : m.tags) {
        tagItems.push_back(MsgPackValue::text(tag));
    }
    return MsgPackValue::map({
        {"schema_version", MsgPackValue::integer(static_cast<std::int64_t>(m.schemaVersion))},
        {"author", MsgPackValue::text(m.author)},
        {"source", MsgPackValue::text(m.source)},
        {"created_at", MsgPackValue::text(m.createdAt)},
        {"tool_provenance", MsgPackValue::text(m.toolProvenance)},
        {"measurement_note", MsgPackValue::text(m.measurementNote)},
        {"measurement", std::move(measurement)},
        {"guide_visual", std::move(guideVisual)},
        {"dimension_visual", std::move(dimensionVisual)},
        {"line_visual", std::move(lineVisual)},
        {"role", MsgPackValue::text(objectRoleName(m.role))},
        {"material", MsgPackValue::text(m.material)},
        {"export_group", MsgPackValue::text(m.exportGroup)},
        {"tags", MsgPackValue::array(std::move(tagItems))},
    });
}

ObjectMetadata readMetadata(const MsgPackValue *v)
{
    ObjectMetadata m;
    if (!v) {
        return m;
    }
    m.schemaVersion = static_cast<std::uint32_t>(asInt(child(*v, "schema_version"), m.schemaVersion));
    m.author = asString(child(*v, "author"), m.author);
    m.source = asString(child(*v, "source"), m.source);
    m.createdAt = asString(child(*v, "created_at"), m.createdAt);
    m.toolProvenance = asString(child(*v, "tool_provenance"), m.toolProvenance);
    m.measurementNote = asString(child(*v, "measurement_note"), m.measurementNote);
    if (const MsgPackValue *meas = child(*v, "measurement")) {
        m.measurement.unit = measurementUnitFromName(asString(child(*meas, "unit"), "none"));
        m.measurement.canvasUnitsPerRealUnit = asDouble(child(*meas, "canvas_units_per_real_unit"), 0.0);
        m.measurement.label = asString(child(*meas, "label"), {});
    }
    if (const MsgPackValue *gv = child(*v, "guide_visual")) {
        m.guideVisual.label = asString(child(*gv, "label"), m.guideVisual.label);
        m.guideVisual.color = asString(child(*gv, "color"), m.guideVisual.color);
        m.guideVisual.dashStyle = asString(child(*gv, "dash_style"), m.guideVisual.dashStyle);
        m.guideVisual.showLabel = asBool(child(*gv, "show_label"), m.guideVisual.showLabel);
    }
    if (const MsgPackValue *dv = child(*v, "dimension_visual")) {
        m.dimensionVisual.showLabel = asBool(child(*dv, "show_label"), m.dimensionVisual.showLabel);
    }
    if (const MsgPackValue *lv = child(*v, "line_visual")) {
        m.lineVisual.endArrow = asBool(child(*lv, "end_arrow"), m.lineVisual.endArrow);
        m.lineVisual.startArrow = asBool(child(*lv, "start_arrow"), m.lineVisual.startArrow);
    }
    m.role = objectRoleFromName(asString(child(*v, "role"), "none"));
    m.material = asString(child(*v, "material"), m.material);
    m.exportGroup = asString(child(*v, "export_group"), m.exportGroup);
    if (const MsgPackValue *tags = child(*v, "tags"); tags && tags->type == MsgPackValue::Type::Array) {
        m.tags.clear();
        for (const MsgPackValue &item : tags->arrayValue) {
            if (item.type == MsgPackValue::Type::String) {
                m.tags.push_back(item.stringValue);
            }
        }
    }
    return m;
}

// --- layer / object ---------------------------------------------------------

MsgPackValue layerValue(const DraftingLayer &layer)
{
    MsgPackValue plot = MsgPackValue::map({
        {"pen_id", MsgPackValue::text(layer.plot.penId)},
        {"stroke_color", MsgPackValue::text(layer.plot.strokeColor)},
        {"stroke_width", MsgPackValue::number(layer.plot.strokeWidth)},
        {"plot_enabled", MsgPackValue::boolean(layer.plot.plotEnabled)},
    });
    return MsgPackValue::map({
        {"id", MsgPackValue::text(layer.id)},
        {"name", MsgPackValue::text(layer.name)},
        {"order", MsgPackValue::integer(layer.order)},
        {"visible", MsgPackValue::boolean(layer.visible)},
        {"locked", MsgPackValue::boolean(layer.locked)},
        {"plot", std::move(plot)},
    });
}

DraftingLayer readLayer(const MsgPackValue &v)
{
    DraftingLayer layer;
    layer.id = asString(child(v, "id"), layer.id);
    layer.name = asString(child(v, "name"), layer.name);
    layer.order = static_cast<int>(asInt(child(v, "order"), layer.order));
    layer.visible = asBool(child(v, "visible"), layer.visible);
    layer.locked = asBool(child(v, "locked"), layer.locked);
    if (const MsgPackValue *plot = child(v, "plot")) {
        layer.plot.penId = asString(child(*plot, "pen_id"), layer.plot.penId);
        layer.plot.strokeColor = asString(child(*plot, "stroke_color"), layer.plot.strokeColor);
        layer.plot.strokeWidth = asDouble(child(*plot, "stroke_width"), layer.plot.strokeWidth);
        layer.plot.plotEnabled = asBool(child(*plot, "plot_enabled"), layer.plot.plotEnabled);
    }
    return layer;
}

MsgPackValue objectValue(const DraftingObject &object)
{
    // Bounds are intentionally not stored; they are recomputed on load.
    return MsgPackValue::map({
        {"id", MsgPackValue::text(object.id)},
        {"kind", MsgPackValue::text(shapeKindName(object.kind))},
        {"layer_id", MsgPackValue::text(object.layerId)},
        {"visible", MsgPackValue::boolean(object.visible)},
        {"locked", MsgPackValue::boolean(object.locked)},
        {"style_id", MsgPackValue::text(object.styleId)},
        {"geometry", geometryValue(object.geometry)},
        {"stroke", strokeValue(object.stroke)},
        {"fill", fillValue(object.fill)},
        {"transform", transformValue(object.transform)},
        {"metadata", metadataValue(object.metadata)},
    });
}

} // namespace

MsgPackValue draftingDocumentToValue(const DraftingDocument &document)
{
    std::vector<MsgPackValue> layers;
    layers.reserve(document.layers.size());
    for (const auto &layer : document.layers) {
        layers.push_back(layerValue(layer));
    }

    std::vector<MsgPackValue> objects;
    objects.reserve(document.objects.size());
    for (const auto &object : document.objects) {
        objects.push_back(objectValue(object));
    }

    std::vector<MsgPackValue> selected;
    selected.reserve(document.selectedObjectIds.size());
    for (const auto &id : document.selectedObjectIds) {
        selected.push_back(MsgPackValue::text(id));
    }

    MsgPackValue activeObject = document.activeObjectId
        ? MsgPackValue::text(*document.activeObjectId)
        : MsgPackValue::nil();

    MsgPackValue documentMap = MsgPackValue::map({
        {"id", MsgPackValue::text(document.id)},
        {"title", MsgPackValue::text(document.title)},
        {"active_layer_id", MsgPackValue::text(document.activeLayerId)},
        {"revision", MsgPackValue::integer(static_cast<std::int64_t>(document.revision))},
        {"layers", MsgPackValue::array(std::move(layers))},
        {"objects", MsgPackValue::array(std::move(objects))},
        {"selected_object_ids", MsgPackValue::array(std::move(selected))},
        {"active_object_id", std::move(activeObject)},
    });

    return MsgPackValue::map({
        {"schema", MsgPackValue::text(kDraftingDocumentSchema)},
        {"version", MsgPackValue::integer(kDraftingDocumentVersion)},
        {"document", std::move(documentMap)},
    });
}

FormatResult<DraftingDocument> draftingDocumentFromValue(const MsgPackValue &value)
{
    if (value.type != MsgPackValue::Type::Map) {
        return FormatResult<DraftingDocument>::failure({}, FormatResultCode::SyntaxError,
                                                       "drawing root is not a map");
    }
    const std::string schema = asString(value.find("schema"), {});
    if (schema != kDraftingDocumentSchema) {
        return FormatResult<DraftingDocument>::failure({}, FormatResultCode::UnsupportedSchema,
                                                       "unsupported drawing schema");
    }
    const MsgPackValue *versionValue = value.find("version");
    if (!versionValue) {
        return FormatResult<DraftingDocument>::failure({}, FormatResultCode::MissingSchema,
                                                       "drawing version is missing");
    }
    const int version = static_cast<int>(asInt(versionValue, -1));
    if (version < kDraftingDocumentMinReadVersion || version > kDraftingDocumentVersion) {
        return FormatResult<DraftingDocument>::failure({}, FormatResultCode::UnsupportedVersion,
                                                       "unsupported drawing version");
    }
    const MsgPackValue *documentValue = value.find("document");
    if (!documentValue || documentValue->type != MsgPackValue::Type::Map) {
        return FormatResult<DraftingDocument>::failure({}, FormatResultCode::SyntaxError,
                                                       "drawing document section is missing");
    }

    DraftingDocument document;
    document.id = asString(child(*documentValue, "id"), {});
    document.title = asString(child(*documentValue, "title"), {});
    document.activeLayerId = asString(child(*documentValue, "active_layer_id"), document.activeLayerId);
    document.revision = static_cast<std::uint64_t>(asInt(child(*documentValue, "revision"), 0));

    if (const MsgPackValue *layers = child(*documentValue, "layers");
        layers && layers->type == MsgPackValue::Type::Array) {
        for (const auto &layer : layers->arrayValue) {
            document.layers.push_back(readLayer(layer));
        }
    }

    if (const MsgPackValue *objects = child(*documentValue, "objects");
        objects && objects->type == MsgPackValue::Type::Array) {
        for (const auto &objectValueRef : objects->arrayValue) {
            if (objectValueRef.type != MsgPackValue::Type::Map) {
                return FormatResult<DraftingDocument>::failure({}, FormatResultCode::SyntaxError,
                                                               "drawing object is not a map");
            }
            const std::string kindName = asString(objectValueRef.find("kind"), {});
            if (!isKnownShapeKindName(kindName)) {
                return FormatResult<DraftingDocument>::failure({}, FormatResultCode::SyntaxError,
                                                               "drawing object has unknown kind");
            }
            const MsgPackValue *geometry = objectValueRef.find("geometry");
            if (!geometry || geometry->type != MsgPackValue::Type::Map) {
                return FormatResult<DraftingDocument>::failure({}, FormatResultCode::SyntaxError,
                                                               "drawing object geometry is missing");
            }
            DraftingObject object;
            object.id = asString(objectValueRef.find("id"), {});
            object.kind = shapeKindFromName(kindName);
            object.geometry = readGeometry(object.kind, *geometry);
            object.layerId = asString(objectValueRef.find("layer_id"), object.layerId);
            object.visible = asBool(objectValueRef.find("visible"), object.visible);
            object.locked = asBool(objectValueRef.find("locked"), object.locked);
            object.styleId = asString(objectValueRef.find("style_id"), object.styleId);
            object.stroke = readStroke(objectValueRef.find("stroke"), version);
            object.fill = readFill(objectValueRef.find("fill"));
            object.transform = readTransform(objectValueRef.find("transform"));
            object.metadata = readMetadata(objectValueRef.find("metadata"));
            object.bounds = computeBounds(object.geometry); // recomputed, never stored
            document.objects.push_back(std::move(object));
        }
    }

    if (const MsgPackValue *selected = child(*documentValue, "selected_object_ids");
        selected && selected->type == MsgPackValue::Type::Array) {
        for (const auto &id : selected->arrayValue) {
            if (id.type == MsgPackValue::Type::String) {
                document.selectedObjectIds.push_back(id.stringValue);
            }
        }
    }

    if (const MsgPackValue *activeObject = child(*documentValue, "active_object_id");
        activeObject && activeObject->type == MsgPackValue::Type::String) {
        document.activeObjectId = activeObject->stringValue;
    }

    return FormatResult<DraftingDocument>::success(std::move(document));
}

ByteBuffer encodeDraftingDocument(const DraftingDocument &document)
{
    ByteBuffer bytes;
    bytes.push_back(edi::formats::ediMessagePackMagic0);
    bytes.push_back(edi::formats::ediMessagePackMagic1);
    bytes.push_back(edi::formats::ediMessagePackMagic2);
    bytes.push_back(edi::formats::ediMessagePackMagic3);
    bytes.push_back(static_cast<std::uint8_t>(edi::formats::ediMessagePackSupportedVersion));
    ByteBuffer payload = edi::formats::encodeMessagePack(draftingDocumentToValue(document));
    bytes.insert(bytes.end(), payload.begin(), payload.end());
    return bytes;
}

FormatResult<DraftingDocument> decodeDraftingDocument(const ByteBuffer &bytes, const std::string &source)
{
    if (bytes.size() < 5) {
        return FormatResult<DraftingDocument>::failure(source, FormatResultCode::EmptyBuffer,
                                                       "drawing file is too small");
    }
    if (bytes[0] != edi::formats::ediMessagePackMagic0
        || bytes[1] != edi::formats::ediMessagePackMagic1
        || bytes[2] != edi::formats::ediMessagePackMagic2
        || bytes[3] != edi::formats::ediMessagePackMagic3) {
        return FormatResult<DraftingDocument>::failure(source, FormatResultCode::UnsupportedSchema,
                                                       "drawing file magic marker is unsupported");
    }
    if (bytes[4] != edi::formats::ediMessagePackSupportedVersion) {
        return FormatResult<DraftingDocument>::failure(source, FormatResultCode::UnsupportedVersion,
                                                       "drawing file envelope version is unsupported");
    }
    ByteBuffer payload(bytes.begin() + 5, bytes.end());
    auto decoded = edi::formats::decodeMessagePack(payload, source);
    if (!decoded.ok || !decoded.value) {
        return FormatResult<DraftingDocument>::failure(source, decoded.code,
                                                       decoded.message.empty() ? "drawing payload is malformed" : decoded.message);
    }
    return draftingDocumentFromValue(*decoded.value);
}

} // namespace edi::drafting
