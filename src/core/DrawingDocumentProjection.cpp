#include "core/DrawingDocumentProjection.h"

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
    QVariantMap result {
        {QStringLiteral("id"), qStringFromStdString(object.id)},
        {QStringLiteral("kind"), QString::fromLatin1(shapeKindName(object.kind))},
        {QStringLiteral("visible"), object.visible},
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

QVariantMap draftingDocumentToModelProjection(const DraftingDocument &document, const DraftingSnapSettings &snapSettings)
{
    QVariantList objects;
    for (const DraftingObject &object : document.objects) {
        objects.push_back(draftingObjectToCanvasProjection(object));
    }

    return {
        {QStringLiteral("engine"), QStringLiteral("cpp_drafting_document")},
        {QStringLiteral("drawing_objects"), objects},
        {QStringLiteral("revision"), static_cast<int>(document.revision)},
        {QStringLiteral("snap"), QVariantMap{
            {QStringLiteral("grid_enabled"), snapSettings.gridEnabled},
            {QStringLiteral("object_enabled"), snapSettings.objectSnapEnabled},
            {QStringLiteral("grid_step"), snapSettings.gridStep},
            {QStringLiteral("object_tolerance"), snapSettings.objectTolerance},
        }},
        {QStringLiteral("validation"), QVariantList{}},
    };
}

} // namespace drawing_core
