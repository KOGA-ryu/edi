#include "core/DrawingDocumentProjection.h"

#include "drafting/DraftingGeometry.h"
#include "drafting/DraftingMeasurement.h"
#include "drafting/DraftingMeasurementFormat.h"
#include "drafting/DraftingNumericEdit.h"
#include "drafting/DraftingObjectEdit.h"

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

Point2D dimensionOffsetVector(const DimensionGeometry &geometry)
{
    const double length = distance(geometry.a, geometry.b);
    if (length <= 0.000001) {
        return {};
    }
    return {
        -(geometry.b.y - geometry.a.y) / length * geometry.offset,
        (geometry.b.x - geometry.a.x) / length * geometry.offset,
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

QVariantList numericFieldsForObject(const DraftingObject &object)
{
    QVariantList fields;
    switch (object.kind) {
    case DraftingShapeKind::Point:
        fields.push_back(numericField(QStringLiteral("x"), QStringLiteral("X")));
        fields.push_back(numericField(QStringLiteral("y"), QStringLiteral("Y")));
        break;
    case DraftingShapeKind::Line:
        fields.push_back(numericField(QStringLiteral("x1"), QStringLiteral("X1")));
        fields.push_back(numericField(QStringLiteral("y1"), QStringLiteral("Y1")));
        fields.push_back(numericField(QStringLiteral("x2"), QStringLiteral("X2")));
        fields.push_back(numericField(QStringLiteral("y2"), QStringLiteral("Y2")));
        fields.push_back(numericField(QStringLiteral("line_length"), QStringLiteral("Length"), 0.0));
        fields.push_back(numericField(QStringLiteral("line_angle_deg"), QStringLiteral("Angle"), -360.0, 360.0, 1.0, 2));
        break;
    case DraftingShapeKind::Rectangle:
        fields.push_back(numericField(QStringLiteral("x"), QStringLiteral("X")));
        fields.push_back(numericField(QStringLiteral("y"), QStringLiteral("Y")));
        fields.push_back(numericField(QStringLiteral("width"), QStringLiteral("Width"), 0.0));
        fields.push_back(numericField(QStringLiteral("height"), QStringLiteral("Height"), 0.0));
        fields.push_back(numericField(QStringLiteral("rotation_deg"), QStringLiteral("Rotation"), -360.0, 360.0, 1.0, 2));
        break;
    case DraftingShapeKind::Circle:
        fields.push_back(numericField(QStringLiteral("cx"), QStringLiteral("CX")));
        fields.push_back(numericField(QStringLiteral("cy"), QStringLiteral("CY")));
        fields.push_back(numericField(QStringLiteral("radius"), QStringLiteral("Radius"), 0.0));
        fields.push_back(numericField(QStringLiteral("diameter"), QStringLiteral("Diameter"), 0.0));
        break;
    case DraftingShapeKind::Guide:
        fields.push_back(numericField(QStringLiteral("position"), QStringLiteral("Position"), 0.0, 1.0));
        break;
    case DraftingShapeKind::ConstructionLine:
        fields.push_back(numericField(QStringLiteral("x1"), QStringLiteral("X1")));
        fields.push_back(numericField(QStringLiteral("y1"), QStringLiteral("Y1")));
        fields.push_back(numericField(QStringLiteral("x2"), QStringLiteral("X2")));
        fields.push_back(numericField(QStringLiteral("y2"), QStringLiteral("Y2")));
        break;
    case DraftingShapeKind::Dimension:
        fields.push_back(numericField(QStringLiteral("x1"), QStringLiteral("X1")));
        fields.push_back(numericField(QStringLiteral("y1"), QStringLiteral("Y1")));
        fields.push_back(numericField(QStringLiteral("x2"), QStringLiteral("X2")));
        fields.push_back(numericField(QStringLiteral("y2"), QStringLiteral("Y2")));
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
            fields.push_back(numericField(QStringLiteral("dimension_length"), lengthLabel, 0.0));
            if (dimension->kind != DimensionKind::Width && dimension->kind != DimensionKind::Height) {
                fields.push_back(numericField(QStringLiteral("dimension_angle_deg"), QStringLiteral("Angle"), -360.0, 360.0, 1.0, 2));
            }
        }
        fields.push_back(numericField(QStringLiteral("offset"), QStringLiteral("Offset")));
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
    if (a.kind != DraftingShapeKind::Guide || b.kind != DraftingShapeKind::Guide) {
        return false;
    }
    const auto *guideA = std::get_if<GuideGeometry>(&a.geometry);
    const auto *guideB = std::get_if<GuideGeometry>(&b.geometry);
    if (guideA == nullptr || guideB == nullptr || guideA->orientation != guideB->orientation) {
        return false;
    }
    constexpr double epsilon = 0.000001;
    return std::abs(guideA->position - guideB->position) <= epsilon;
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

double physicalX(Point2D point, const DraftingGridProjection &grid)
{
    return point.x * grid.settings.width;
}

double physicalY(Point2D point, const DraftingGridProjection &grid)
{
    return point.y * grid.settings.height;
}

double physicalWidth(double normalizedWidth, const DraftingGridProjection &grid)
{
    return normalizedWidth * grid.settings.width;
}

double physicalHeight(double normalizedHeight, const DraftingGridProjection &grid)
{
    return normalizedHeight * grid.settings.height;
}

double physicalDistance(Point2D a, Point2D b, const DraftingGridProjection &grid)
{
    const double dx = physicalWidth(b.x - a.x, grid);
    const double dy = physicalHeight(b.y - a.y, grid);
    return std::sqrt(dx * dx + dy * dy);
}

double physicalDimensionOffset(const DimensionGeometry &geometry, const DraftingGridProjection &grid)
{
    const Point2D offset = dimensionOffsetVector(geometry);
    const double dx = physicalWidth(offset.x, grid);
    const double dy = physicalHeight(offset.y, grid);
    const double magnitude = std::sqrt(dx * dx + dy * dy);
    return geometry.offset < 0.0 ? -magnitude : magnitude;
}

double physicalAngleDegrees(Point2D a, Point2D b, const DraftingGridProjection &grid)
{
    const double dx = physicalWidth(b.x - a.x, grid);
    const double dy = physicalHeight(b.y - a.y, grid);
    constexpr double pi = 3.14159265358979323846;
    return std::atan2(dy, dx) * 180.0 / pi;
}

double displayedDimensionDistance(double distanceValue, DimensionKind kind)
{
    return kind == DimensionKind::Diameter ? distanceValue * 2.0 : distanceValue;
}

QString physicalDimensionLabel(double distanceValue, DimensionKind kind, const DraftingGridProjection &grid)
{
    const double value = displayedDimensionDistance(distanceValue, kind);
    return QStringLiteral("%1 %2")
        .arg(QString::number(value, 'g', 6),
            QString::fromLatin1(draftingGridUnitLabel(grid.settings.unit)));
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
            result.insert(QStringLiteral("x1"), physicalX(geometry.a, grid));
            result.insert(QStringLiteral("y1"), physicalY(geometry.a, grid));
            result.insert(QStringLiteral("x2"), physicalX(geometry.b, grid));
            result.insert(QStringLiteral("y2"), physicalY(geometry.b, grid));
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
        } else if constexpr (std::is_same_v<Geometry, GuideGeometry>) {
            if (geometry.orientation == GuideOrientation::Horizontal) {
                result.insert(QStringLiteral("position"), geometry.position * grid.settings.height);
            } else {
                result.insert(QStringLiteral("position"), geometry.position * grid.settings.width);
            }
        } else if constexpr (std::is_same_v<Geometry, ConstructionLineGeometry>) {
            result.insert(QStringLiteral("x1"), physicalX(geometry.a, grid));
            result.insert(QStringLiteral("y1"), physicalY(geometry.a, grid));
            result.insert(QStringLiteral("x2"), physicalX(geometry.b, grid));
            result.insert(QStringLiteral("y2"), physicalY(geometry.b, grid));
        } else if constexpr (std::is_same_v<Geometry, DimensionGeometry>) {
            result.insert(QStringLiteral("x1"), physicalX(geometry.a, grid));
            result.insert(QStringLiteral("y1"), physicalY(geometry.a, grid));
            result.insert(QStringLiteral("x2"), physicalX(geometry.b, grid));
            result.insert(QStringLiteral("y2"), physicalY(geometry.b, grid));
            result.insert(QStringLiteral("offset"), physicalDimensionOffset(geometry, grid));
            result.insert(QStringLiteral("dimension_distance"), displayedDimensionDistance(physicalDistance(geometry.a, geometry.b, grid), geometry.kind));
            result.insert(QStringLiteral("dimension_length"), displayedDimensionDistance(physicalDistance(geometry.a, geometry.b, grid), geometry.kind));
            result.insert(QStringLiteral("dimension_angle_deg"), physicalAngleDegrees(geometry.a, geometry.b, grid));
            result.insert(QStringLiteral("dimension_label"), physicalDimensionLabel(physicalDistance(geometry.a, geometry.b, grid), geometry.kind, grid));
        }
    }, object.geometry);

    return result;
}

QVariantList editHandlesForObject(const DraftingObject &object)
{
    QVariantList result;
    for (const DraftingHandleDescriptor &handle : draftingHandlesForObject(object)) {
        QVariantMap projected {
            {QStringLiteral("id"), QString::fromStdString(handle.id)},
            {QStringLiteral("role"), QString::fromStdString(handle.role)},
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

} // namespace

QString qStringFromStdString(const std::string &value)
{
    return QString::fromStdString(value);
}

QVariantMap draftingObjectToCanvasProjection(const DraftingObject &object, const DraftingGridProjection *grid)
{
    QVariantList measurementLines;
    const auto measurement = summarizeObjectMeasurement(object);
    if (measurement.ok) {
        for (const std::string &line : formatObjectMeasurementSummary(measurement.value)) {
            measurementLines.push_back(qStringFromStdString(line));
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
        {QStringLiteral("measurement_lines"), measurementLines},
        {QStringLiteral("numeric_fields"), numericFieldsForObject(object)},
        {QStringLiteral("edit_handles"), editHandlesForObject(object)},
        {QStringLiteral("guide_drawable_controls"), object.kind == DraftingShapeKind::Guide},
        {QStringLiteral("construction_drawable_controls"), constructionLineCanFitDrawable(object)},
        {QStringLiteral("bounds_guide_controls"), canCreateGuideFromBounds(object)},
        {QStringLiteral("align_to_guide_controls"), canAlignToGuide(object)},
    };
    result.insert(QStringLiteral("editable_handle_count"), result.value(QStringLiteral("edit_handles")).toList().size());

    if (grid != nullptr) {
        result.insert(QStringLiteral("physical_geometry"), physicalGeometryForObject(object, *grid));
    }

    std::visit([&](const auto &geometry) {
        using Geometry = std::decay_t<decltype(geometry)>;
        if constexpr (std::is_same_v<Geometry, PointGeometry>) {
            result.insert(QStringLiteral("x"), geometry.point.x);
            result.insert(QStringLiteral("y"), geometry.point.y);
        } else if constexpr (std::is_same_v<Geometry, LineGeometry>) {
            result.insert(QStringLiteral("x1"), geometry.a.x);
            result.insert(QStringLiteral("y1"), geometry.a.y);
            result.insert(QStringLiteral("x2"), geometry.b.x);
            result.insert(QStringLiteral("y2"), geometry.b.y);
            result.insert(QStringLiteral("line_length"), distance(geometry.a, geometry.b));
            result.insert(QStringLiteral("line_angle_deg"), lineAngleDegrees(geometry));
        } else if constexpr (std::is_same_v<Geometry, RectangleGeometry>) {
            result.insert(QStringLiteral("x"), geometry.origin.x);
            result.insert(QStringLiteral("y"), geometry.origin.y);
            result.insert(QStringLiteral("width"), geometry.width);
            result.insert(QStringLiteral("height"), geometry.height);
            result.insert(QStringLiteral("rotation_deg"), geometry.rotationDeg);
        } else if constexpr (std::is_same_v<Geometry, CircleGeometry>) {
            result.insert(QStringLiteral("cx"), geometry.center.x);
            result.insert(QStringLiteral("cy"), geometry.center.y);
            result.insert(QStringLiteral("radius"), geometry.radius);
            result.insert(QStringLiteral("diameter"), geometry.radius * 2.0);
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
            result.insert(QStringLiteral("x1"), geometry.a.x);
            result.insert(QStringLiteral("y1"), geometry.a.y);
            result.insert(QStringLiteral("x2"), geometry.b.x);
            result.insert(QStringLiteral("y2"), geometry.b.y);
            result.insert(QStringLiteral("plot_ready"), false);
        } else if constexpr (std::is_same_v<Geometry, DimensionGeometry>) {
            const Point2D offset = dimensionOffsetVector(geometry);
            const Point2D dimA{geometry.a.x + offset.x, geometry.a.y + offset.y};
            const Point2D dimB{geometry.b.x + offset.x, geometry.b.y + offset.y};
            const MeasurementValue measuredDistance = measureDistance(geometry.a, geometry.b, scaleCalibrationFromMetadata(object.metadata.measurement));
            const double normalizedDistance = displayedDimensionDistance(distance(geometry.a, geometry.b), geometry.kind);
            MeasurementValue displayedDistance = measuredDistance;
            displayedDistance.value = displayedDimensionDistance(measuredDistance.value, geometry.kind);
            result.insert(QStringLiteral("x1"), geometry.a.x);
            result.insert(QStringLiteral("y1"), geometry.a.y);
            result.insert(QStringLiteral("x2"), geometry.b.x);
            result.insert(QStringLiteral("y2"), geometry.b.y);
            result.insert(QStringLiteral("offset"), geometry.offset);
            result.insert(QStringLiteral("dimension_kind"), QString::fromLatin1(dimensionKindName(geometry.kind)));
            result.insert(QStringLiteral("dimension_length"), normalizedDistance);
            result.insert(QStringLiteral("dimension_angle_deg"), dimensionAngleDegrees(geometry));
            result.insert(QStringLiteral("dimension_x1"), dimA.x);
            result.insert(QStringLiteral("dimension_y1"), dimA.y);
            result.insert(QStringLiteral("dimension_x2"), dimB.x);
            result.insert(QStringLiteral("dimension_y2"), dimB.y);
            result.insert(QStringLiteral("extension_x1"), geometry.a.x);
            result.insert(QStringLiteral("extension_y1"), geometry.a.y);
            result.insert(QStringLiteral("extension_x2"), dimA.x);
            result.insert(QStringLiteral("extension_y2"), dimA.y);
            result.insert(QStringLiteral("extension2_x1"), geometry.b.x);
            result.insert(QStringLiteral("extension2_y1"), geometry.b.y);
            result.insert(QStringLiteral("extension2_x2"), dimB.x);
            result.insert(QStringLiteral("extension2_y2"), dimB.y);
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
        QVariantMap projected = draftingObjectToCanvasProjection(object, grid);
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
        projected.insert(QStringLiteral("effective_pen_id"), layer == nullptr ? QString() : qStringFromStdString(layer->plot.penId));
        projected.insert(QStringLiteral("effective_stroke_color"), layer == nullptr ? QString() : qStringFromStdString(layer->plot.strokeColor));
        projected.insert(QStringLiteral("effective_stroke_width"), layer == nullptr ? 0.0 : layer->plot.strokeWidth);
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
        if (object.kind != DraftingShapeKind::Guide || !kindMatchesGeometry(object.kind, object.geometry)) {
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
