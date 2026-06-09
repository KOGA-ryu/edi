#include "core/DrawingDocumentProjection.h"

#include "drafting/DraftingMeasurement.h"
#include "drafting/DraftingMeasurementFormat.h"

#include <QVariantList>

#include <type_traits>

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

} // namespace

QString qStringFromStdString(const std::string &value)
{
    return QString::fromStdString(value);
}

QVariantMap draftingObjectToCanvasProjection(const DraftingObject &object)
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
        {QStringLiteral("measurement_lines"), measurementLines},
    };

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
        } else if constexpr (std::is_same_v<Geometry, GuideGeometry>) {
            result.insert(QStringLiteral("orientation"), QString::fromLatin1(guideOrientationName(geometry.orientation)));
            result.insert(QStringLiteral("position"), geometry.position);
            result.insert(QStringLiteral("plot_ready"), false);
        } else if constexpr (std::is_same_v<Geometry, ConstructionLineGeometry>) {
            result.insert(QStringLiteral("x1"), geometry.a.x);
            result.insert(QStringLiteral("y1"), geometry.a.y);
            result.insert(QStringLiteral("x2"), geometry.b.x);
            result.insert(QStringLiteral("y2"), geometry.b.y);
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

QVariantMap draftingDocumentToModelProjection(const DraftingDocument &document, const DraftingSnapSettings &snapSettings, const DraftingObject *previewObject)
{
    QVariantList objects;
    for (const DraftingObject &object : document.objects) {
        objects.push_back(draftingObjectToCanvasProjection(object));
    }
    QVariantList selectedObjectIds;
    for (const DraftingObjectId &id : document.selectedObjectIds) {
        selectedObjectIds.push_back(qStringFromStdString(id));
    }

    QVariantMap result {
        {QStringLiteral("engine"), QStringLiteral("cpp_drafting_document")},
        {QStringLiteral("drawing_objects"), objects},
        {QStringLiteral("selected_object_ids"), selectedObjectIds},
        {QStringLiteral("active_object_id"), document.activeObjectId ? qStringFromStdString(*document.activeObjectId) : QString()},
        {QStringLiteral("revision"), static_cast<int>(document.revision)},
        {QStringLiteral("snap"), QVariantMap{
            {QStringLiteral("grid_enabled"), snapSettings.gridEnabled},
            {QStringLiteral("object_enabled"), snapSettings.objectSnapEnabled},
            {QStringLiteral("endpoint_enabled"), snapSettings.endpointEnabled},
            {QStringLiteral("vertex_enabled"), snapSettings.vertexEnabled},
            {QStringLiteral("midpoint_enabled"), snapSettings.midpointEnabled},
            {QStringLiteral("center_enabled"), snapSettings.centerEnabled},
            {QStringLiteral("object_priority_before_grid"), snapSettings.objectPriorityBeforeGrid},
            {QStringLiteral("grid_step"), snapSettings.gridStep},
            {QStringLiteral("grid_step_x"), snapSettings.gridStepX},
            {QStringLiteral("grid_step_y"), snapSettings.gridStepY},
            {QStringLiteral("object_tolerance"), snapSettings.objectTolerance},
        }},
        {QStringLiteral("validation"), QVariantList{}},
    };
    if (previewObject != nullptr) {
        result.insert(QStringLiteral("preview_object"), draftingObjectToCanvasProjection(*previewObject));
    }
    return result;
}

} // namespace drawing_core
