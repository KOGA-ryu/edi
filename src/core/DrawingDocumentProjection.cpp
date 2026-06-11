#include "core/DrawingDocumentProjection.h"

#include "drafting/DraftingLayerOps.h"
#include "drafting/DraftingGeometry.h"
#include "drafting/DraftingGuideOps.h"
#include "drafting/DraftingMeasurement.h"
#include "drafting/DraftingMeasurementFormat.h"
#include "drafting/DraftingObjectEdit.h"
#include "drafting/DraftingPhysicalGeometry.h"

#include <QVariantList>

#include <algorithm>
#include <cmath>
#include <limits>
#include <type_traits>
#include <vector>

namespace drawing_core {
namespace {

using namespace edi::drafting;

QVariantMap pointToMap(Point2D point)
{
    return {
        {QStringLiteral("x"), point.x},
        {QStringLiteral("y"), point.y},
    };
}

QVariantMap layerToMap(const DraftingLayer &layer)
{
    return {
        {QStringLiteral("id"), QString::fromStdString(layer.id)},
        {QStringLiteral("name"), QString::fromStdString(layer.name)},
        {QStringLiteral("order"), layer.order},
        {QStringLiteral("visible"), layer.visible},
        {QStringLiteral("locked"), layer.locked},
        {QStringLiteral("plot_enabled"), layer.plot.plotEnabled},
        {QStringLiteral("pen_id"), QString::fromStdString(layer.plot.penId)},
        {QStringLiteral("stroke_color"), QString::fromStdString(layer.plot.strokeColor)},
        {QStringLiteral("stroke_width"), layer.plot.strokeWidth},
    };
}

QVariantMap numericField(
    const QString &id,
    const QString &label,
    double minimum = -10.0,
    double maximum = 10.0,
    double step = 0.01,
    int decimals = 4)
{
    return {
        {QStringLiteral("id"), id},
        {QStringLiteral("label"), label},
        {QStringLiteral("minimum"), minimum},
        {QStringLiteral("maximum"), maximum},
        {QStringLiteral("step"), step},
        {QStringLiteral("decimals"), decimals},
    };
}

bool isAngleField(const QString &id)
{
    return id == QStringLiteral("line_angle_deg")
        || id == QStringLiteral("dimension_angle_deg")
        || id == QStringLiteral("rotation_deg")
        || id == QStringLiteral("start_angle_deg")
        || id == QStringLiteral("end_angle_deg");
}

bool isNonNegativePhysicalField(const QString &id)
{
    return id == QStringLiteral("width")
        || id == QStringLiteral("height")
        || id == QStringLiteral("radius")
        || id == QStringLiteral("diameter")
        || id == QStringLiteral("line_length")
        || id == QStringLiteral("dimension_length");
}

void addPhysicalEditorMetadata(QVariantMap &field, const DraftingGridProjection *grid)
{
    if (grid == nullptr) {
        field.insert(QStringLiteral("physical_editable"), false);
        return;
    }

    const QString id = field.value(QStringLiteral("id")).toString();
    const bool angle = isAngleField(id);
    field.insert(QStringLiteral("physical_editable"), true);
    field.insert(QStringLiteral("physical_unit_kind"), angle ? QStringLiteral("angle") : QStringLiteral("length"));
    field.insert(QStringLiteral("physical_unit_label"), angle
            ? QStringLiteral("deg")
            : QString::fromLatin1(draftingGridUnitLabel(grid->settings.unit)));
    field.insert(QStringLiteral("physical_decimals"), angle ? 2 : field.value(QStringLiteral("decimals"), 4).toInt());
    field.insert(QStringLiteral("physical_step"), angle ? 1.0 : field.value(QStringLiteral("step"), 0.01).toDouble());
    if (isNonNegativePhysicalField(id)) {
        field.insert(QStringLiteral("physical_minimum"), 0.0);
        field.insert(QStringLiteral("physical_maximum"), 100000.0);
    } else if (angle) {
        field.insert(QStringLiteral("physical_minimum"), -360.0);
        field.insert(QStringLiteral("physical_maximum"), 360.0);
    } else {
        field.insert(QStringLiteral("physical_minimum"), -100000.0);
        field.insert(QStringLiteral("physical_maximum"), 100000.0);
    }
}

void pushNumericField(QVariantList &fields, QVariantMap field, const DraftingGridProjection *grid)
{
    addPhysicalEditorMetadata(field, grid);
    fields.push_back(field);
}

QVariantList numericFieldsForObject(const DraftingObject &object, const DraftingGridProjection *grid)
{
    QVariantList fields;
    switch (object.kind) {
    case DraftingShapeKind::Point:
        pushNumericField(fields, numericField(QStringLiteral("x"), QStringLiteral("X")), grid);
        pushNumericField(fields, numericField(QStringLiteral("y"), QStringLiteral("Y")), grid);
        break;
    case DraftingShapeKind::Line:
        pushNumericField(fields, numericField(QStringLiteral("x1"), QStringLiteral("X1")), grid);
        pushNumericField(fields, numericField(QStringLiteral("y1"), QStringLiteral("Y1")), grid);
        pushNumericField(fields, numericField(QStringLiteral("x2"), QStringLiteral("X2")), grid);
        pushNumericField(fields, numericField(QStringLiteral("y2"), QStringLiteral("Y2")), grid);
        pushNumericField(fields, numericField(QStringLiteral("line_length"), QStringLiteral("Length"), 0.0), grid);
        pushNumericField(fields, numericField(QStringLiteral("line_angle_deg"), QStringLiteral("Angle"), -360.0, 360.0, 1.0, 2), grid);
        break;
    case DraftingShapeKind::Rectangle:
        pushNumericField(fields, numericField(QStringLiteral("x"), QStringLiteral("X")), grid);
        pushNumericField(fields, numericField(QStringLiteral("y"), QStringLiteral("Y")), grid);
        pushNumericField(fields, numericField(QStringLiteral("width"), QStringLiteral("Width"), 0.0), grid);
        pushNumericField(fields, numericField(QStringLiteral("height"), QStringLiteral("Height"), 0.0), grid);
        pushNumericField(fields, numericField(QStringLiteral("rotation_deg"), QStringLiteral("Rotation"), -360.0, 360.0, 1.0, 2), grid);
        break;
    case DraftingShapeKind::Circle:
        pushNumericField(fields, numericField(QStringLiteral("cx"), QStringLiteral("CX")), grid);
        pushNumericField(fields, numericField(QStringLiteral("cy"), QStringLiteral("CY")), grid);
        pushNumericField(fields, numericField(QStringLiteral("radius"), QStringLiteral("Radius"), 0.0), grid);
        pushNumericField(fields, numericField(QStringLiteral("diameter"), QStringLiteral("Diameter"), 0.0), grid);
        break;
    case DraftingShapeKind::Arc:
        pushNumericField(fields, numericField(QStringLiteral("cx"), QStringLiteral("CX")), grid);
        pushNumericField(fields, numericField(QStringLiteral("cy"), QStringLiteral("CY")), grid);
        pushNumericField(fields, numericField(QStringLiteral("radius"), QStringLiteral("Radius"), 0.0), grid);
        pushNumericField(fields, numericField(QStringLiteral("start_angle_deg"), QStringLiteral("Start"), -360.0, 360.0, 1.0, 2), grid);
        pushNumericField(fields, numericField(QStringLiteral("end_angle_deg"), QStringLiteral("End"), -360.0, 360.0, 1.0, 2), grid);
        break;
    case DraftingShapeKind::Guide:
        pushNumericField(fields, numericField(QStringLiteral("position"), QStringLiteral("Position"), 0.0, 1.0), grid);
        break;
    case DraftingShapeKind::ConstructionLine:
        pushNumericField(fields, numericField(QStringLiteral("x1"), QStringLiteral("X1")), grid);
        pushNumericField(fields, numericField(QStringLiteral("y1"), QStringLiteral("Y1")), grid);
        pushNumericField(fields, numericField(QStringLiteral("x2"), QStringLiteral("X2")), grid);
        pushNumericField(fields, numericField(QStringLiteral("y2"), QStringLiteral("Y2")), grid);
        break;
    case DraftingShapeKind::Dimension:
        pushNumericField(fields, numericField(QStringLiteral("x1"), QStringLiteral("X1")), grid);
        pushNumericField(fields, numericField(QStringLiteral("y1"), QStringLiteral("Y1")), grid);
        pushNumericField(fields, numericField(QStringLiteral("x2"), QStringLiteral("X2")), grid);
        pushNumericField(fields, numericField(QStringLiteral("y2"), QStringLiteral("Y2")), grid);
        if (const auto *dimension = std::get_if<DimensionGeometry>(&object.geometry)) {
            QString lengthLabel = QStringLiteral("Distance");
            if (dimension->kind == DimensionKind::Width) {
                lengthLabel = QStringLiteral("Width");
            } else if (dimension->kind == DimensionKind::Height) {
                lengthLabel = QStringLiteral("Height");
            } else if (dimension->kind == DimensionKind::Radius) {
                lengthLabel = QStringLiteral("Radius");
            } else if (dimension->kind == DimensionKind::Diameter) {
                lengthLabel = QStringLiteral("Diameter");
            }
            pushNumericField(fields, numericField(QStringLiteral("dimension_length"), lengthLabel, 0.0), grid);
            if (dimension->kind != DimensionKind::Width && dimension->kind != DimensionKind::Height) {
                pushNumericField(fields, numericField(QStringLiteral("dimension_angle_deg"), QStringLiteral("Angle"), -360.0, 360.0, 1.0, 2), grid);
            }
        }
        pushNumericField(fields, numericField(QStringLiteral("offset"), QStringLiteral("Offset")), grid);
        break;
    case DraftingShapeKind::Polygon:
    case DraftingShapeKind::Polyline:
        break;
    }
    return fields;
}

int layerOrderForObject(const DraftingDocument &document, const DraftingObject &object)
{
    const DraftingLayer *layer = findLayer(document, object.layerId);
    return layer == nullptr ? std::numeric_limits<int>::max() : layer->order;
}

bool shapeCanPlot(DraftingShapeKind kind)
{
    return kind != DraftingShapeKind::Guide
        && kind != DraftingShapeKind::ConstructionLine
        && kind != DraftingShapeKind::Dimension;
}

bool constructionLineCanFitDrawable(const DraftingObject &object)
{
    if (object.kind != DraftingShapeKind::ConstructionLine) {
        return false;
    }
    const auto *line = std::get_if<ConstructionLineGeometry>(&object.geometry);
    if (line == nullptr) {
        return false;
    }
    constexpr double epsilon = 0.0000001;
    return std::abs(line->a.y - line->b.y) < epsilon
        || std::abs(line->a.x - line->b.x) < epsilon;
}

bool canCreateGuideFromBounds(const DraftingObject &object)
{
    return object.kind != DraftingShapeKind::Guide
        && object.kind != DraftingShapeKind::ConstructionLine
        && object.kind != DraftingShapeKind::Dimension
        && isFinite(object.bounds);
}

bool canAlignToGuide(const DraftingObject &object)
{
    return canCreateGuideFromBounds(object);
}

bool equivalentGuide(const DraftingObject &a, const DraftingObject &b)
{
    if (!isGuideObject(a) || !isGuideObject(b)) {
        return false;
    }
    const auto *guideA = std::get_if<GuideGeometry>(&a.geometry);
    const auto *guideB = std::get_if<GuideGeometry>(&b.geometry);
    if (guideA == nullptr || guideB == nullptr) {
        return false;
    }
    return sameGuide(*guideA, *guideB);
}

bool hasPriorEquivalentGuide(const std::vector<const DraftingObject *> &objects, std::size_t index)
{
    for (std::size_t previous = 0; previous < index; ++previous) {
        if (equivalentGuide(*objects[previous], *objects[index])) {
            return true;
        }
    }
    return false;
}

QString defaultGuideLabel(const DraftingObject &object, const GuideGeometry &geometry)
{
    const QString prefix = geometry.orientation == GuideOrientation::Horizontal
        ? QStringLiteral("H")
        : QStringLiteral("V");
    return QStringLiteral("%1 guide %2%3")
        .arg(prefix,
            QString::number(geometry.position, 'f', 3),
            object.locked ? QStringLiteral(" locked") : QString());
}

QString physicalDimensionLabel(double distanceValue, DimensionKind kind, const DraftingGridProjection &grid)
{
    const double value = displayedDimensionLength(distanceValue, kind);
    return QStringLiteral("%1 %2")
        .arg(QString::number(value, 'g', 6),
            QString::fromLatin1(draftingGridUnitLabel(grid.settings.unit)));
}

void insertPhysicalSegment(QVariantMap &map, Point2D a, Point2D b, const DraftingGridProjection &grid)
{
    map.insert(QStringLiteral("x1"), physicalX(a, grid));
    map.insert(QStringLiteral("y1"), physicalY(a, grid));
    map.insert(QStringLiteral("x2"), physicalX(b, grid));
    map.insert(QStringLiteral("y2"), physicalY(b, grid));
}

QVariantMap physicalGeometryForObject(const DraftingObject &object, const DraftingGridProjection &grid)
{
    QVariantMap result {
        {QStringLiteral("unit"), QString::fromLatin1(draftingGridUnitName(grid.settings.unit))},
        {QStringLiteral("unit_label"), QString::fromLatin1(draftingGridUnitLabel(grid.settings.unit))},
    };

    std::visit([&](const auto &geometry) {
        using Geometry = std::decay_t<decltype(geometry)>;
        if constexpr (std::is_same_v<Geometry, PointGeometry>) {
            result.insert(QStringLiteral("x"), physicalX(geometry.point, grid));
            result.insert(QStringLiteral("y"), physicalY(geometry.point, grid));
        } else if constexpr (std::is_same_v<Geometry, LineGeometry>) {
            insertPhysicalSegment(result, geometry.a, geometry.b, grid);
            result.insert(QStringLiteral("line_length"), physicalDistance(geometry.a, geometry.b, grid));
            result.insert(QStringLiteral("line_angle_deg"), physicalAngleDegrees(geometry.a, geometry.b, grid));
        } else if constexpr (std::is_same_v<Geometry, RectangleGeometry>) {
            result.insert(QStringLiteral("x"), physicalX(geometry.origin, grid));
            result.insert(QStringLiteral("y"), physicalY(geometry.origin, grid));
            result.insert(QStringLiteral("width"), physicalWidth(geometry.width, grid));
            result.insert(QStringLiteral("height"), physicalHeight(geometry.height, grid));
            result.insert(QStringLiteral("rotation_deg"), geometry.rotationDeg);
        } else if constexpr (std::is_same_v<Geometry, CircleGeometry>) {
            result.insert(QStringLiteral("cx"), physicalX(geometry.center, grid));
            result.insert(QStringLiteral("cy"), physicalY(geometry.center, grid));
            result.insert(QStringLiteral("radius"), physicalWidth(geometry.radius, grid));
            result.insert(QStringLiteral("diameter"), physicalWidth(geometry.radius * 2.0, grid));
            result.insert(QStringLiteral("radius_y"), physicalHeight(geometry.radius, grid));
            result.insert(QStringLiteral("diameter_y"), physicalHeight(geometry.radius * 2.0, grid));
        } else if constexpr (std::is_same_v<Geometry, ArcGeometry>) {
            result.insert(QStringLiteral("cx"), physicalX(geometry.center, grid));
            result.insert(QStringLiteral("cy"), physicalY(geometry.center, grid));
            result.insert(QStringLiteral("radius"), physicalWidth(geometry.radius, grid));
            result.insert(QStringLiteral("start_angle_deg"), geometry.startAngleDeg);
            result.insert(QStringLiteral("end_angle_deg"), geometry.endAngleDeg);
        } else if constexpr (std::is_same_v<Geometry, GuideGeometry>) {
            if (geometry.orientation == GuideOrientation::Horizontal) {
                result.insert(QStringLiteral("position"), geometry.position * grid.settings.height);
            } else {
                result.insert(QStringLiteral("position"), geometry.position * grid.settings.width);
            }
        } else if constexpr (std::is_same_v<Geometry, ConstructionLineGeometry>) {
            insertPhysicalSegment(result, geometry.a, geometry.b, grid);
        } else if constexpr (std::is_same_v<Geometry, DimensionGeometry>) {
            insertPhysicalSegment(result, geometry.a, geometry.b, grid);
            result.insert(QStringLiteral("offset"), physicalDimensionOffset(geometry, grid));
            result.insert(QStringLiteral("dimension_distance"), displayedDimensionLength(physicalDistance(geometry.a, geometry.b, grid), geometry.kind));
            result.insert(QStringLiteral("dimension_length"), displayedDimensionLength(physicalDistance(geometry.a, geometry.b, grid), geometry.kind));
            result.insert(QStringLiteral("dimension_angle_deg"), physicalAngleDegrees(geometry.a, geometry.b, grid));
            result.insert(QStringLiteral("dimension_label"), physicalDimensionLabel(physicalDistance(geometry.a, geometry.b, grid), geometry.kind, grid));
        }
    }, object.geometry);

    return result;
}

struct HandleVisual {
    QString cursor;
    QString shape;
    double sizePx = 8.0;
    double hitTolerancePx = 14.0;
};

HandleVisual handleVisualFor(const DraftingHandleDescriptor &handle)
{
    if (handle.readOnly) {
        return {QStringLiteral("default"), QStringLiteral("square"), 6.0, 0.0};
    }
    if (handle.role == "rotate") {
        return {QStringLiteral("rotate"), QStringLiteral("circle"), 10.0, 18.0};
    }
    if (handle.role == "offset") {
        return {QStringLiteral("move"), QStringLiteral("diamond"), 8.0, 14.0};
    }
    if (handle.role == "point" || handle.role == "center") {
        return {QStringLiteral("move"), QStringLiteral("circle"), 8.0, 14.0};
    }
    return {QStringLiteral("resize"), QStringLiteral("circle"), 8.0, 14.0};
}

QVariantList editHandlesForObject(const DraftingObject &object)
{
    QVariantList result;
    for (const DraftingHandleDescriptor &handle : draftingHandlesForObject(object)) {
        const HandleVisual visual = handleVisualFor(handle);
        QVariantMap projected {
            {QStringLiteral("id"), QString::fromStdString(handle.id)},
            {QStringLiteral("role"), QString::fromStdString(handle.role)},
            {QStringLiteral("cursor"), visual.cursor},
            {QStringLiteral("shape"), visual.shape},
            {QStringLiteral("size_px"), visual.sizePx},
            {QStringLiteral("hit_tolerance_px"), visual.hitTolerancePx},
            {QStringLiteral("editable"), !handle.readOnly},
            {QStringLiteral("x"), handle.point.x},
            {QStringLiteral("y"), handle.point.y},
            {QStringLiteral("read_only"), handle.readOnly},
            {QStringLiteral("has_anchor"), handle.hasAnchor},
        };
        if (handle.hasAnchor) {
            projected.insert(QStringLiteral("anchor_x"), handle.anchor.x);
            projected.insert(QStringLiteral("anchor_y"), handle.anchor.y);
        }
        result.push_back(projected);
    }
    return result;
}

struct SegmentKeys {
    QString x1;
    QString y1;
    QString x2;
    QString y2;
};

const SegmentKeys kSegmentKeys{QStringLiteral("x1"), QStringLiteral("y1"), QStringLiteral("x2"), QStringLiteral("y2")};
const SegmentKeys kDimensionSegmentKeys{QStringLiteral("dimension_x1"), QStringLiteral("dimension_y1"), QStringLiteral("dimension_x2"), QStringLiteral("dimension_y2")};
const SegmentKeys kExtensionSegmentKeys{QStringLiteral("extension_x1"), QStringLiteral("extension_y1"), QStringLiteral("extension_x2"), QStringLiteral("extension_y2")};
const SegmentKeys kExtension2SegmentKeys{QStringLiteral("extension2_x1"), QStringLiteral("extension2_y1"), QStringLiteral("extension2_x2"), QStringLiteral("extension2_y2")};

void insertSegment(QVariantMap &map, const SegmentKeys &keys, Point2D a, Point2D b)
{
    map.insert(keys.x1, a.x);
    map.insert(keys.y1, a.y);
    map.insert(keys.x2, b.x);
    map.insert(keys.y2, b.y);
}

} // namespace

QString qStringFromStdString(const std::string &value)
{
    return QString::fromStdString(value);
}

QVariantMap draftingObjectToCanvasProjection(const DraftingObject &object, const DraftingGridProjection *grid, bool includeEditorFields)
{
    QVariantList measurementLines;
    const auto measurement = summarizeObjectMeasurement(object);
    if (measurement.ok) {
        for (const std::string &line : formatObjectMeasurementSummary(measurement.value)) {
            measurementLines.push_back(qStringFromStdString(line));
        }
    }
    const QVariantList editHandles = editHandlesForObject(object);
    int editableHandleCount = 0;
    for (const QVariant &handleValue : editHandles) {
        if (handleValue.toMap().value(QStringLiteral("editable")).toBool()) {
            ++editableHandleCount;
        }
    }

    QVariantMap result {
        {QStringLiteral("id"), qStringFromStdString(object.id)},
        {QStringLiteral("kind"), QString::fromLatin1(shapeKindName(object.kind))},
        {QStringLiteral("visible"), object.visible},
        {QStringLiteral("bounds"), QVariantMap{
            {QStringLiteral("x"), object.bounds.x},
            {QStringLiteral("y"), object.bounds.y},
            {QStringLiteral("width"), object.bounds.width},
            {QStringLiteral("height"), object.bounds.height},
        }},
        {QStringLiteral("layer_id"), qStringFromStdString(object.layerId)},
        {QStringLiteral("locked"), object.locked},
        {QStringLiteral("tool_provenance"), qStringFromStdString(object.metadata.toolProvenance)},
        {QStringLiteral("measurement_note"), qStringFromStdString(object.metadata.measurementNote)},
        {QStringLiteral("role"), QString::fromLatin1(objectRoleName(object.metadata.role))},
        {QStringLiteral("material"), qStringFromStdString(object.metadata.material)},
        {QStringLiteral("export_group"), qStringFromStdString(object.metadata.exportGroup)},
        {QStringLiteral("tags"), [&]() {
            QStringList tagList;
            for (const std::string &tag : object.metadata.tags) {
                tagList.push_back(qStringFromStdString(tag));
            }
            return tagList.join(QStringLiteral(", "));
        }()},
        {QStringLiteral("measurement_lines"), measurementLines},
        {QStringLiteral("edit_handles"), editHandles},
        {QStringLiteral("handle_count"), editHandles.size()},
        {QStringLiteral("editable_handle_count"), editableHandleCount},
        {QStringLiteral("guide_drawable_controls"), object.kind == DraftingShapeKind::Guide},
        {QStringLiteral("construction_drawable_controls"), constructionLineCanFitDrawable(object)},
        {QStringLiteral("bounds_guide_controls"), canCreateGuideFromBounds(object)},
        {QStringLiteral("align_to_guide_controls"), canAlignToGuide(object)},
    };

    // Editor metadata (numeric spinner specs, unit conversions) is rich and
    // expensive — and only the geometry editor reads it, only for the ACTIVE
    // object. Building it for every object made the projection O(objects)
    // in editor-map construction: the dominant cost in the 1000-line bench.
    if (includeEditorFields) {
        result.insert(QStringLiteral("numeric_fields"), numericFieldsForObject(object, grid));
        if (grid != nullptr) {
            result.insert(QStringLiteral("physical_geometry"), physicalGeometryForObject(object, *grid));
        }
    } else {
        result.insert(QStringLiteral("numeric_fields"), QVariantList{});
    }

    std::visit([&](const auto &geometry) {
        using Geometry = std::decay_t<decltype(geometry)>;
        if constexpr (std::is_same_v<Geometry, PointGeometry>) {
            result.insert(QStringLiteral("x"), geometry.point.x);
            result.insert(QStringLiteral("y"), geometry.point.y);
        } else if constexpr (std::is_same_v<Geometry, LineGeometry>) {
            insertSegment(result, kSegmentKeys, geometry.a, geometry.b);
            result.insert(QStringLiteral("line_length"), distance(geometry.a, geometry.b));
            result.insert(QStringLiteral("line_angle_deg"), lineAngleDegrees(geometry));
            // N2: the arrow variant's distinguishing projected flag. The
            // painter reads it to draw a head; the inspector could expose it.
            result.insert(QStringLiteral("end_arrow"), object.metadata.lineVisual.endArrow);
        } else if constexpr (std::is_same_v<Geometry, RectangleGeometry>) {
            result.insert(QStringLiteral("x"), geometry.origin.x);
            result.insert(QStringLiteral("y"), geometry.origin.y);
            result.insert(QStringLiteral("width"), geometry.width);
            result.insert(QStringLiteral("height"), geometry.height);
            result.insert(QStringLiteral("rotation_deg"), geometry.rotationDeg);
            // N4 variant params — the painter reads these to round/frame.
            result.insert(QStringLiteral("corner_radius"), geometry.cornerRadius);
            result.insert(QStringLiteral("inset"), geometry.inset);
        } else if constexpr (std::is_same_v<Geometry, CircleGeometry>) {
            result.insert(QStringLiteral("cx"), geometry.center.x);
            result.insert(QStringLiteral("cy"), geometry.center.y);
            result.insert(QStringLiteral("radius"), geometry.radius);
            result.insert(QStringLiteral("diameter"), geometry.radius * 2.0);
        } else if constexpr (std::is_same_v<Geometry, ArcGeometry>) {
            result.insert(QStringLiteral("cx"), geometry.center.x);
            result.insert(QStringLiteral("cy"), geometry.center.y);
            result.insert(QStringLiteral("radius"), geometry.radius);
            result.insert(QStringLiteral("start_angle_deg"), geometry.startAngleDeg);
            result.insert(QStringLiteral("end_angle_deg"), geometry.endAngleDeg);
            // Flattened polyline so the canvas painter can draw the arc directly.
            QVariantList points;
            for (Point2D point : sampleArc(geometry)) {
                points.push_back(pointToMap(point));
            }
            result.insert(QStringLiteral("points"), points);
        } else if constexpr (std::is_same_v<Geometry, GuideGeometry>) {
            result.insert(QStringLiteral("orientation"), QString::fromLatin1(guideOrientationName(geometry.orientation)));
            result.insert(QStringLiteral("position"), geometry.position);
            result.insert(QStringLiteral("guide_label"), object.metadata.guideVisual.label.empty()
                    ? defaultGuideLabel(object, geometry)
                    : qStringFromStdString(object.metadata.guideVisual.label));
            result.insert(QStringLiteral("guide_custom_label"), qStringFromStdString(object.metadata.guideVisual.label));
            result.insert(QStringLiteral("guide_color"), qStringFromStdString(object.metadata.guideVisual.color));
            result.insert(QStringLiteral("guide_dash_style"), qStringFromStdString(object.metadata.guideVisual.dashStyle));
            result.insert(QStringLiteral("guide_show_label"), object.metadata.guideVisual.showLabel);
            result.insert(QStringLiteral("guide_visual_controls"), true);
            result.insert(QStringLiteral("plot_ready"), false);
        } else if constexpr (std::is_same_v<Geometry, ConstructionLineGeometry>) {
            insertSegment(result, kSegmentKeys, geometry.a, geometry.b);
            result.insert(QStringLiteral("plot_ready"), false);
        } else if constexpr (std::is_same_v<Geometry, DimensionGeometry>) {
            const Point2D offset = dimensionOffsetVector(geometry);
            const Point2D dimA{geometry.a.x + offset.x, geometry.a.y + offset.y};
            const Point2D dimB{geometry.b.x + offset.x, geometry.b.y + offset.y};
            const MeasurementValue measuredDistance = measureDistance(geometry.a, geometry.b, scaleCalibrationFromMetadata(object.metadata.measurement));
            const double normalizedDistance = displayedDimensionLength(distance(geometry.a, geometry.b), geometry.kind);
            MeasurementValue displayedDistance = measuredDistance;
            displayedDistance.value = displayedDimensionLength(measuredDistance.value, geometry.kind);
            insertSegment(result, kSegmentKeys, geometry.a, geometry.b);
            result.insert(QStringLiteral("offset"), geometry.offset);
            result.insert(QStringLiteral("dimension_kind"), QString::fromLatin1(dimensionKindName(geometry.kind)));
            result.insert(QStringLiteral("dimension_length"), normalizedDistance);
            result.insert(QStringLiteral("dimension_angle_deg"), dimensionAngleDegrees(geometry));
            insertSegment(result, kDimensionSegmentKeys, dimA, dimB);
            insertSegment(result, kExtensionSegmentKeys, geometry.a, dimA);
            insertSegment(result, kExtension2SegmentKeys, geometry.b, dimB);
            result.insert(QStringLiteral("label_x"), (dimA.x + dimB.x) / 2.0);
            result.insert(QStringLiteral("label_y"), (dimA.y + dimB.y) / 2.0);
            result.insert(QStringLiteral("dimension_distance"), normalizedDistance);
            result.insert(QStringLiteral("dimension_show_label"), object.metadata.dimensionVisual.showLabel);
            result.insert(QStringLiteral("dimension_visual_controls"), true);
            result.insert(QStringLiteral("label"), qStringFromStdString(formatMeasurementValue(displayedDistance)));
            result.insert(QStringLiteral("plot_ready"), false);
        } else {
            QVariantList points;
            for (Point2D point : geometry.vertices) {
                points.push_back(pointToMap(point));
            }
            result.insert(QStringLiteral("points"), points);
        }
    }, object.geometry);

    return result;
}

QVariantMap draftingDocumentToModelProjection(
    const DraftingDocument &document,
    const DraftingSnapSettings &snapSettings,
    const DraftingGridProjection *grid,
    const DraftingObject *previewObject)
{
    QVariantList objects;
    std::vector<const DraftingObject *> sortedObjects;
    sortedObjects.reserve(document.objects.size());
    for (const DraftingObject &object : document.objects) {
        sortedObjects.push_back(&object);
    }
    std::stable_sort(sortedObjects.begin(), sortedObjects.end(), [&](const DraftingObject *a, const DraftingObject *b) {
        return layerOrderForObject(document, *a) < layerOrderForObject(document, *b);
    });
    for (const DraftingObject *objectPointer : sortedObjects) {
        const DraftingObject &object = *objectPointer;
        const bool isActive = document.activeObjectId && object.id == *document.activeObjectId;
        QVariantMap projected = draftingObjectToCanvasProjection(object, grid, isActive);
        const DraftingLayer *layer = findLayer(document, object.layerId);
        const bool layerVisible = layer == nullptr ? false : layer->visible;
        const bool layerLocked = layer == nullptr ? false : layer->locked;
        const bool layerPlotEnabled = layer != nullptr && layer->plot.plotEnabled;
        const bool effectivePlotReady = layerPlotEnabled && shapeCanPlot(object.kind);
        projected.insert(QStringLiteral("layer_visible"), layerVisible);
        projected.insert(QStringLiteral("layer_locked"), layerLocked);
        projected.insert(QStringLiteral("effective_visible"), object.visible && layerVisible);
        projected.insert(QStringLiteral("effective_locked"), object.locked || layerLocked);
        projected.insert(QStringLiteral("effective_plot_enabled"), layerPlotEnabled);
        projected.insert(QStringLiteral("effective_plot_ready"), effectivePlotReady);
        // Per-object styling: the object's stroke wins where set, the layer
        // fills the rest (effectiveObjectStroke). The OWN values ride along
        // so the style controls can show set-vs-inherit honestly.
        const StrokeStyle resolvedStroke = layer == nullptr
            ? object.stroke
            : effectiveObjectStroke(object, *layer);
        projected.insert(QStringLiteral("effective_pen_id"), layer == nullptr
            ? QString()
            : qStringFromStdString(penIdForStrokeColor(resolvedStroke.color, layer->plot.penId)));
        projected.insert(QStringLiteral("effective_stroke_color"), qStringFromStdString(resolvedStroke.color));
        projected.insert(QStringLiteral("effective_stroke_width"), resolvedStroke.width);
        projected.insert(QStringLiteral("effective_line_style"), qStringFromStdString(resolvedStroke.lineStyle));
        projected.insert(QStringLiteral("own_stroke_color"), qStringFromStdString(object.stroke.color));
        projected.insert(QStringLiteral("own_stroke_width"), object.stroke.width);
        projected.insert(QStringLiteral("own_line_style"), qStringFromStdString(object.stroke.lineStyle));
        if (!shapeCanPlot(object.kind)) {
            projected.insert(QStringLiteral("plot_ready"), false);
        } else {
            projected.insert(QStringLiteral("plot_ready"), effectivePlotReady);
        }
        objects.push_back(projected);
    }
    QVariantList selectedObjectIds;
    for (const DraftingObjectId &id : document.selectedObjectIds) {
        selectedObjectIds.push_back(qStringFromStdString(id));
    }
    QVariantList layers;
    std::vector<const DraftingLayer *> sortedLayers;
    sortedLayers.reserve(document.layers.size());
    for (const DraftingLayer &layer : document.layers) {
        sortedLayers.push_back(&layer);
    }
    std::stable_sort(sortedLayers.begin(), sortedLayers.end(), [](const DraftingLayer *a, const DraftingLayer *b) {
        return a->order < b->order;
    });
    for (const DraftingLayer *layer : sortedLayers) {
        layers.push_back(layerToMap(*layer));
    }

    int guideCount = 0;
    int visibleGuideCount = 0;
    int duplicateGuideCount = 0;
    for (std::size_t index = 0; index < sortedObjects.size(); ++index) {
        const DraftingObject &object = *sortedObjects[index];
        if (!isGuideObject(object)) {
            continue;
        }
        ++guideCount;
        const DraftingLayer *layer = findLayer(document, object.layerId);
        if (object.visible && layer != nullptr && layer->visible) {
            ++visibleGuideCount;
        }
        if (hasPriorEquivalentGuide(sortedObjects, index)) {
            ++duplicateGuideCount;
        }
    }

    QVariantMap result {
        {QStringLiteral("engine"), QStringLiteral("cpp_drafting_document")},
        {QStringLiteral("drawing_objects"), objects},
        {QStringLiteral("layers"), layers},
        {QStringLiteral("active_layer_id"), qStringFromStdString(document.activeLayerId)},
        {QStringLiteral("selected_object_ids"), selectedObjectIds},
        {QStringLiteral("active_object_id"), document.activeObjectId ? qStringFromStdString(*document.activeObjectId) : QString()},
        {QStringLiteral("revision"), static_cast<int>(document.revision)},
        {QStringLiteral("guide_count"), guideCount},
        {QStringLiteral("visible_guide_count"), visibleGuideCount},
        {QStringLiteral("duplicate_guide_count"), duplicateGuideCount},
        {QStringLiteral("snap"), QVariantMap{
            {QStringLiteral("grid_enabled"), snapSettings.gridEnabled},
            {QStringLiteral("object_enabled"), snapSettings.objectSnapEnabled},
            {QStringLiteral("endpoint_enabled"), snapSettings.endpointEnabled},
            {QStringLiteral("vertex_enabled"), snapSettings.vertexEnabled},
            {QStringLiteral("midpoint_enabled"), snapSettings.midpointEnabled},
            {QStringLiteral("center_enabled"), snapSettings.centerEnabled},
            {QStringLiteral("guide_enabled"), snapSettings.guideEnabled},
            {QStringLiteral("object_priority_before_grid"), snapSettings.objectPriorityBeforeGrid},
            {QStringLiteral("grid_step"), snapSettings.gridStep},
            {QStringLiteral("grid_step_x"), snapSettings.gridStepX},
            {QStringLiteral("grid_step_y"), snapSettings.gridStepY},
            {QStringLiteral("object_tolerance"), snapSettings.objectTolerance},
        }},
        {QStringLiteral("validation"), QVariantList{}},
    };
    if (previewObject != nullptr) {
        result.insert(QStringLiteral("preview_object"), draftingObjectToCanvasProjection(*previewObject, grid));
    }
    return result;
}

} // namespace drawing_core
