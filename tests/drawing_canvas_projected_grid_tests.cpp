#include "widgets/DrawingCanvasProjectedGrid.h"

#include <QVariantList>
#include <QVariantMap>

#include "EdiAssert.h"
#include <limits>

using namespace drawing_canvas;

namespace {

QVariantMap point(double x, double y)
{
    return QVariantMap{
        {QStringLiteral("x"), x},
        {QStringLiteral("y"), y},
    };
}

QVariantMap bounds(double x, double y, double width, double height)
{
    return QVariantMap{
        {QStringLiteral("x"), x},
        {QStringLiteral("y"), y},
        {QStringLiteral("width"), width},
        {QStringLiteral("height"), height},
    };
}

QVariantMap gridModel(const QVariantMap &grid)
{
    return QVariantMap{{QStringLiteral("grid"), grid}};
}

} // namespace

int main()
{
    const DrawingCanvasProjectedGrid missing = projectedGrid({});
    EDI_CHECK(missing.lines.empty());
    EDI_CHECK(!missing.drawableBounds.visible);
    EDI_CHECK(!missing.origin.visible);

    const DrawingCanvasProjectedGrid parsed = projectedGrid(gridModel(QVariantMap{
        {QStringLiteral("lines"), QVariantList{
            QVariantMap{
                {QStringLiteral("axis"), QStringLiteral("vertical")},
                {QStringLiteral("position"), 0.25},
                {QStringLiteral("major"), true},
            },
            QVariantMap{
                {QStringLiteral("axis"), QStringLiteral("horizontal")},
                {QStringLiteral("position"), 0.75},
                {QStringLiteral("major"), false},
            },
            QVariantMap{
                {QStringLiteral("axis"), QStringLiteral("diagonal")},
                {QStringLiteral("position"), 0.5},
            },
            QVariantMap{
                {QStringLiteral("axis"), QStringLiteral("vertical")},
                {QStringLiteral("position"), std::numeric_limits<double>::quiet_NaN()},
            },
            QStringLiteral("ignored"),
        }},
        {QStringLiteral("drawable_bounds"), bounds(0.1, 0.2, 0.7, 0.6)},
        {QStringLiteral("origin"), point(0.5, 0.5)},
    }));
    EDI_CHECK(parsed.lines.size() == 2);
    EDI_CHECK(parsed.lines[0].axis == QStringLiteral("vertical"));
    EDI_CHECK(parsed.lines[0].position == 0.25);
    EDI_CHECK(parsed.lines[0].major);
    EDI_CHECK(parsed.lines[1].axis == QStringLiteral("horizontal"));
    EDI_CHECK(parsed.lines[1].position == 0.75);
    EDI_CHECK(!parsed.lines[1].major);
    EDI_CHECK(parsed.drawableBounds.visible);
    EDI_CHECK(parsed.drawableBounds.x == 0.1);
    EDI_CHECK(parsed.drawableBounds.y == 0.2);
    EDI_CHECK(parsed.drawableBounds.width == 0.7);
    EDI_CHECK(parsed.drawableBounds.height == 0.6);
    EDI_CHECK(parsed.origin.visible);
    EDI_CHECK(parsed.origin.x == 0.5);
    EDI_CHECK(parsed.origin.y == 0.5);

    const DrawingCanvasProjectedGrid badBounds = projectedGrid(gridModel(QVariantMap{
        {QStringLiteral("drawable_bounds"), bounds(0.1, 0.2, -0.7, 0.6)},
        {QStringLiteral("origin"), point(0.5, 0.5)},
    }));
    EDI_CHECK(!badBounds.drawableBounds.visible);
    EDI_CHECK(badBounds.origin.visible);

    const DrawingCanvasProjectedGrid badOrigin = projectedGrid(gridModel(QVariantMap{
        {QStringLiteral("drawable_bounds"), bounds(0.1, 0.2, 0.7, 0.6)},
        {QStringLiteral("origin"), QVariantMap{
            {QStringLiteral("x"), 0.5},
            {QStringLiteral("y"), QStringLiteral("bad")},
        }},
    }));
    EDI_CHECK(badOrigin.drawableBounds.visible);
    EDI_CHECK(!badOrigin.origin.visible);

    return 0;
}
