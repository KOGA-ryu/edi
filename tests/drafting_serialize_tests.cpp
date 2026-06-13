#include "drafting/DraftingSerialize.h"
#include "drafting/DraftingDocument.h"
#include "drafting/DraftingGeometry.h"

#include <cassert>
#include <cmath>
#include <limits>
#include <string>
#include <vector>

using namespace edi::drafting;
using edi::formats::MsgPackValue;

namespace {

DraftingObject makeObject(const std::string &id, DraftingShapeKind kind, DraftingGeometry geometry,
                          const std::string &layerId)
{
    DraftingObject object = makeDraftingObject(id, kind, std::move(geometry));
    object.layerId = layerId;
    object.bounds = computeBounds(object.geometry);
    return object;
}

bool pointsEqual(const Point2D &a, const Point2D &b)
{
    return a.x == b.x && a.y == b.y;
}

bool boundsEqual(const Bounds2D &a, const Bounds2D &b)
{
    return a.x == b.x && a.y == b.y && a.width == b.width && a.height == b.height;
}

// A document exercising every object kind, two layers, and selection.
DraftingDocument buildSampleDocument()
{
    DraftingDocument document = makeDraftingDocument("doc-1", "Sample Drawing");
    document.layers.clear();

    DraftingLayer base = makeDraftingLayer("default", "Default", 0);
    base.plot.penId = "pen_black";
    base.plot.strokeColor = "#112233";
    base.plot.strokeWidth = 1.5;
    base.plot.plotEnabled = true;
    DraftingLayer guides = makeDraftingLayer("guides", "Guides", 1);
    guides.visible = false;
    guides.locked = true;
    guides.plot.penId = "pen_blue";
    guides.plot.strokeColor = "#83aeca";
    guides.plot.strokeWidth = 0.5;
    guides.plot.plotEnabled = false;
    document.layers = {base, guides};
    document.activeLayerId = "guides";

    PointGeometry pt; pt.point = {0.1, 0.2};
    LineGeometry line; line.a = {0.0, 0.0}; line.b = {0.5, 0.6};
    RectangleGeometry rect; rect.origin = {0.1, 0.1}; rect.width = 0.3; rect.height = 0.2; rect.rotationDeg = 12.5;
    rect.cornerRadius = 0.04; rect.inset = 0.015; // N4 rounded+frame variant params
    CircleGeometry circle; circle.center = {0.4, 0.4}; circle.radius = 0.15;
    ArcGeometry arc; arc.center = {0.3, 0.3}; arc.radius = 0.12; arc.startAngleDeg = 15.0; arc.endAngleDeg = 120.0;
    EllipseGeometry ellipse; ellipse.center = {0.5, 0.5}; ellipse.rx = 0.18; ellipse.ry = 0.1; // rx != ry so bounds pin both
    PolygonGeometry polygon; polygon.vertices = {{0.0, 0.0}, {0.2, 0.0}, {0.2, 0.2}, {0.0, 0.2}};
    PolylineGeometry polyline; polyline.vertices = {{0.1, 0.1}, {0.3, 0.2}, {0.5, 0.1}};
    GuideGeometry guide; guide.orientation = GuideOrientation::Vertical; guide.position = 0.42;
    ConstructionLineGeometry cline; cline.a = {0.1, 0.9}; cline.b = {0.9, 0.1};
    DimensionGeometry dim; dim.kind = DimensionKind::Radius; dim.a = {0.2, 0.2}; dim.b = {0.6, 0.2}; dim.offset = 0.07;

    document.objects = {
        makeObject("point-1", DraftingShapeKind::Point, pt, "default"),
        makeObject("line-1", DraftingShapeKind::Line, line, "default"),
        makeObject("rect-1", DraftingShapeKind::Rectangle, rect, "default"),
        makeObject("circle-1", DraftingShapeKind::Circle, circle, "default"),
        makeObject("arc-1", DraftingShapeKind::Arc, arc, "default"),
        makeObject("polygon-1", DraftingShapeKind::Polygon, polygon, "default"),
        makeObject("polyline-1", DraftingShapeKind::Polyline, polyline, "default"),
        makeObject("guide-1", DraftingShapeKind::Guide, guide, "guides"),
        makeObject("cline-1", DraftingShapeKind::ConstructionLine, cline, "default"),
        makeObject("dim-1", DraftingShapeKind::Dimension, dim, "default"),
        makeObject("ellipse-1", DraftingShapeKind::Ellipse, ellipse, "default"), // index 10 — appended to keep prior indices stable
    };

    // Non-default styling and metadata on one object to exercise those fields.
    DraftingObject &styled = document.objects[2];
    styled.stroke.width = 2.5;
    styled.stroke.opacity = 0.8;
    styled.stroke.color = "#ff8800";
    styled.stroke.lineStyle = "dash";
    styled.fill.opacity = 0.4;
    styled.fill.color = "#00ff00";
    styled.styleId = "style-special";
    styled.transform.translateX = 0.05;
    styled.transform.rotationDeg = 45.0;
    styled.locked = true;
    styled.metadata.author = "ace";
    styled.metadata.source = "import";
    styled.metadata.createdAt = "2026-06-10";
    styled.metadata.toolProvenance = "rectangle_tool";
    styled.metadata.measurementNote = "wall";
    styled.metadata.measurement.unit = MeasurementUnit::Millimeter;
    styled.metadata.measurement.canvasUnitsPerRealUnit = 10.0;
    styled.metadata.measurement.label = "10mm";
    styled.metadata.role = ObjectRole::Cutout;
    styled.metadata.material = "oak";
    styled.metadata.exportGroup = "frame";
    styled.metadata.tags = {"load-bearing", "visible"};

    // The line object carries the N2 arrow flag, to round-trip lineVisual.
    document.objects[1].metadata.lineVisual.endArrow = true;

    DraftingObject &guideObject = document.objects[7];
    guideObject.metadata.guideVisual.label = "centerline";
    guideObject.metadata.guideVisual.color = "#abcdef";
    guideObject.metadata.guideVisual.dashStyle = "dot";
    guideObject.metadata.guideVisual.showLabel = false;
    guideObject.visible = false;

    document.selectedObjectIds = {"rect-1", "circle-1"};
    document.activeObjectId = "rect-1";
    document.revision = 17;
    return document;
}

void assertDocumentsEqual(const DraftingDocument &a, const DraftingDocument &b)
{
    assert(a.id == b.id);
    assert(a.title == b.title);
    assert(a.activeLayerId == b.activeLayerId);
    assert(a.revision == b.revision);
    assert(a.selectedObjectIds == b.selectedObjectIds);
    assert(a.activeObjectId == b.activeObjectId);

    assert(a.layers.size() == b.layers.size());
    for (std::size_t i = 0; i < a.layers.size(); ++i) {
        const auto &la = a.layers[i];
        const auto &lb = b.layers[i];
        assert(la.id == lb.id && la.name == lb.name && la.order == lb.order);
        assert(la.visible == lb.visible && la.locked == lb.locked);
        assert(la.plot.penId == lb.plot.penId && la.plot.strokeColor == lb.plot.strokeColor);
        assert(la.plot.strokeWidth == lb.plot.strokeWidth && la.plot.plotEnabled == lb.plot.plotEnabled);
    }

    assert(a.objects.size() == b.objects.size());
    for (std::size_t i = 0; i < a.objects.size(); ++i) {
        const auto &oa = a.objects[i];
        const auto &ob = b.objects[i];
        assert(oa.id == ob.id);
        assert(oa.kind == ob.kind);
        assert(oa.layerId == ob.layerId);
        assert(oa.visible == ob.visible && oa.locked == ob.locked);
        assert(oa.styleId == ob.styleId);
        assert(oa.stroke.width == ob.stroke.width && oa.stroke.opacity == ob.stroke.opacity);
        assert(oa.stroke.color == ob.stroke.color && oa.stroke.lineStyle == ob.stroke.lineStyle);
        assert(oa.fill.opacity == ob.fill.opacity && oa.fill.color == ob.fill.color);
        assert(oa.transform.translateX == ob.transform.translateX);
        assert(oa.transform.rotationDeg == ob.transform.rotationDeg);
        assert(oa.metadata.author == ob.metadata.author);
        assert(oa.metadata.measurement.unit == ob.metadata.measurement.unit);
        assert(oa.metadata.measurement.canvasUnitsPerRealUnit == ob.metadata.measurement.canvasUnitsPerRealUnit);
        assert(oa.metadata.guideVisual.label == ob.metadata.guideVisual.label);
        assert(oa.metadata.guideVisual.showLabel == ob.metadata.guideVisual.showLabel);
        assert(oa.metadata.dimensionVisual.showLabel == ob.metadata.dimensionVisual.showLabel);
        assert(oa.metadata.lineVisual.endArrow == ob.metadata.lineVisual.endArrow);
        assert(oa.metadata.role == ob.metadata.role);
        assert(oa.metadata.material == ob.metadata.material);
        assert(oa.metadata.exportGroup == ob.metadata.exportGroup);
        assert(oa.metadata.tags == ob.metadata.tags);
        assert(boundsEqual(oa.bounds, ob.bounds));
    }
}

} // namespace

int main()
{
    DraftingDocument document = buildSampleDocument();

    // Value-level round-trip: every field preserved.
    {
        MsgPackValue value = draftingDocumentToValue(document);
        auto restored = draftingDocumentFromValue(value);
        assert(restored.ok);
        assert(restored.value);
        assertDocumentsEqual(document, *restored.value);
    }

    // The read seam clamps untrusted alpha: a file carrying out-of-range or
    // non-finite opacity decodes to a sane [0,1] value — every downstream
    // consumer (projection, inspector spin, SVG emit gate) sees only that.
    {
        DraftingDocument hostile = buildSampleDocument();
        hostile.objects[2].stroke.opacity = 7.0;   // encoder writes raw...
        hostile.objects[2].fill.opacity = -3.0;
        hostile.objects[3].stroke.opacity = std::numeric_limits<double>::quiet_NaN();
        auto decoded = decodeDraftingDocument(encodeDraftingDocument(hostile), "fixture");
        assert(decoded.ok);
        assert(decoded.value->objects[2].stroke.opacity == 1.0); // ...the decoder clamps
        assert(decoded.value->objects[2].fill.opacity == 0.0);
        assert(decoded.value->objects[3].stroke.opacity == 1.0); // NaN -> fully opaque
    }

    // Byte-level round-trip is byte-stable across save/open/save.
    {
        auto bytes1 = encodeDraftingDocument(document);
        auto decoded = decodeDraftingDocument(bytes1, "fixture");
        assert(decoded.ok);
        assert(decoded.value);
        assertDocumentsEqual(document, *decoded.value);
        auto bytes2 = encodeDraftingDocument(*decoded.value);
        assert(bytes1 == bytes2);
    }

    // Geometry fidelity spot checks across alternatives.
    {
        auto decoded = decodeDraftingDocument(encodeDraftingDocument(document), "fixture");
        assert(decoded.ok);
        const auto &objects = decoded.value->objects;
        assert(std::get<GuideGeometry>(objects[7].geometry).orientation == GuideOrientation::Vertical);
        assert(std::get<GuideGeometry>(objects[7].geometry).position == 0.42);
        assert(std::get<DimensionGeometry>(objects[9].geometry).kind == DimensionKind::Radius);
        assert(std::get<DimensionGeometry>(objects[9].geometry).offset == 0.07);
        assert(pointsEqual(std::get<LineGeometry>(objects[1].geometry).b, {0.5, 0.6}));
        const auto &rectGeo = std::get<RectangleGeometry>(objects[2].geometry);
        assert(rectGeo.cornerRadius == 0.04 && rectGeo.inset == 0.015);
        assert(std::get<PolygonGeometry>(objects[5].geometry).vertices.size() == 4);
        const auto &arcGeo = std::get<ArcGeometry>(objects[4].geometry);
        assert(arcGeo.startAngleDeg == 15.0 && arcGeo.endAngleDeg == 120.0);
        const auto &ellipseGeo = std::get<EllipseGeometry>(objects[10].geometry);
        assert(ellipseGeo.rx == 0.18 && ellipseGeo.ry == 0.1);
    }

    // Unknown-field tolerance: extra keys at every level are ignored on read.
    {
        MsgPackValue value = draftingDocumentToValue(document);
        value.mapValue.emplace_back("future_top", MsgPackValue::text("ignored"));
        MsgPackValue *documentMap = nullptr;
        for (auto &entry : value.mapValue) {
            if (entry.first == "document") {
                documentMap = &entry.second;
            }
        }
        assert(documentMap);
        documentMap->mapValue.emplace_back("future_doc", MsgPackValue::integer(99));
        for (auto &entry : documentMap->mapValue) {
            if (entry.first == "objects" && !entry.second.arrayValue.empty()) {
                entry.second.arrayValue[0].mapValue.emplace_back("future_obj", MsgPackValue::boolean(true));
            }
        }
        auto restored = draftingDocumentFromValue(value);
        assert(restored.ok);
        assertDocumentsEqual(document, *restored.value);
    }

    // Bad schema rejected.
    {
        MsgPackValue value = draftingDocumentToValue(document);
        for (auto &entry : value.mapValue) {
            if (entry.first == "schema") {
                entry.second = MsgPackValue::text("edi.wrong");
            }
        }
        auto restored = draftingDocumentFromValue(value);
        assert(!restored.ok);
        assert(restored.code == edi::formats::FormatResultCode::UnsupportedSchema);
    }

    // Bad version rejected.
    {
        MsgPackValue value = draftingDocumentToValue(document);
        for (auto &entry : value.mapValue) {
            if (entry.first == "version") {
                entry.second = MsgPackValue::integer(999);
            }
        }
        auto restored = draftingDocumentFromValue(value);
        assert(!restored.ok);
        assert(restored.code == edi::formats::FormatResultCode::UnsupportedVersion);
    }

    // Missing document section rejected.
    {
        MsgPackValue value = MsgPackValue::map({
            {"schema", MsgPackValue::text(kDraftingDocumentSchema)},
            {"version", MsgPackValue::integer(kDraftingDocumentVersion)},
        });
        auto restored = draftingDocumentFromValue(value);
        assert(!restored.ok);
    }

    // Unknown object kind rejected.
    {
        MsgPackValue value = draftingDocumentToValue(document);
        for (auto &entry : value.mapValue) {
            if (entry.first != "document") continue;
            for (auto &docEntry : entry.second.mapValue) {
                if (docEntry.first == "objects") {
                    for (auto &obj : docEntry.second.arrayValue) {
                        for (auto &field : obj.mapValue) {
                            if (field.first == "kind") {
                                field.second = MsgPackValue::text("nonsense");
                            }
                        }
                    }
                }
            }
        }
        auto restored = draftingDocumentFromValue(value);
        assert(!restored.ok);
    }

    // Corrupt envelope magic rejected.
    {
        auto bytes = encodeDraftingDocument(document);
        bytes[1] = 'X';
        auto decoded = decodeDraftingDocument(bytes, "fixture");
        assert(!decoded.ok);
        assert(decoded.code == edi::formats::FormatResultCode::UnsupportedSchema);
    }

    // Corrupting a structural payload byte (the top map's framing tag, the
    // first byte after the 5-byte envelope) is detected: decode rejects.
    {
        auto bytes = encodeDraftingDocument(document);
        bytes[5] ^= 0x40;
        auto decoded = decodeDraftingDocument(bytes, "fixture");
        assert(!decoded.ok);
    }

    return 0;
}
