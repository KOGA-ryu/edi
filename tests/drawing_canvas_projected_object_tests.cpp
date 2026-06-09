#include "widgets/DrawingCanvasProjectedObject.h"

#include <QVariantList>

#include <cassert>
#include <limits>

using namespace drawing_canvas;

int main()
{
    QVariantList points;
    points.push_back(QVariantMap{{QStringLiteral("x"), 0.1}, {QStringLiteral("y"), 0.2}});
    points.push_back(QVariantList{0.3, 0.4});
    points.push_back(QVariantMap{{QStringLiteral("x"), std::numeric_limits<double>::quiet_NaN()}, {QStringLiteral("y"), 0.6}});
    points.push_back(QVariantMap{});
    points.push_back(QStringLiteral("ignored"));

    const std::vector<DrawingCanvasProjectedPoint> parsed = projectedObjectPoints(QVariantMap{
        {QStringLiteral("points"), points}
    });

    assert(parsed.size() == 3);
    assert(parsed[0].x == 0.1);
    assert(parsed[0].y == 0.2);
    assert(parsed[1].x == 0.3);
    assert(parsed[1].y == 0.4);
    assert(parsed[2].x == 0.0);
    assert(parsed[2].y == 0.6);

    const std::vector<DrawingCanvasProjectedPoint> missing = projectedObjectPoints({});
    assert(missing.empty());

    return 0;
}
