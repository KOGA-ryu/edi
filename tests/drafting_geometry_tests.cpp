#include "drafting/DraftingGeometry.h"
#include "drafting/DraftingMeasurement.h"

#include <cassert>
#include <cmath>

using namespace edi::drafting;

int main()
{
    DraftingGeometry line = LineGeometry{{0.0, 0.0}, {3.0, 4.0}};
    Bounds2D bounds = computeBounds(line);
    assert(bounds.width == 3.0);
    assert(bounds.height == 4.0);
    assert(distance({0.0, 0.0}, {3.0, 4.0}) == 5.0);

    DraftingGeometry rect = RectangleGeometry{{2.0, 3.0}, 10.0, 5.0};
    Bounds2D rectBounds = computeBounds(rect);
    assert(rectBounds.x == 2.0);
    assert(rectBounds.y == 3.0);
    assert(area(rect) == 50.0);

    auto handles = handleAnchors(line);
    assert(handles.size() == 2);
    assert(handles.front().id == "line_start");

    ScaleCalibration calibration{2.0, MeasurementUnit::Centimeter};
    MeasurementValue measured = measureDistance({0.0, 0.0}, {0.0, 10.0}, calibration);
    assert(measured.value == 5.0);

    return 0;
}
