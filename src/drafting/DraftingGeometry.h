#pragma once

#include "drafting/DraftingTypes.h"

#include <optional>
#include <string>
#include <vector>

namespace edi::drafting {

struct HandleAnchor {
    std::string id;
    Point2D point;
};

struct GeometryValidationResult {
    bool ok = false;
    DraftingResultCode code = DraftingResultCode::InvalidGeometry;
    std::string message;

    static GeometryValidationResult accepted();
    static GeometryValidationResult rejected(std::string message);
};

bool isFinite(Point2D point);
bool isFinite(const Bounds2D &bounds);
GeometryValidationResult validateGeometry(const DraftingGeometry &geometry);
Bounds2D computeBounds(const DraftingGeometry &geometry);
Bounds2D includeBounds(Bounds2D bounds, Bounds2D next);
bool boundsContainsPoint(Bounds2D bounds, Point2D point);
DraftingGeometry translateGeometry(const DraftingGeometry &geometry, double dx, double dy);
Point2D translatePoint(Point2D point, double dx, double dy);
bool translationHasEffect(double dx, double dy);
double distance(Point2D a, Point2D b);
double lineAngleDegrees(const LineGeometry &line);
double dimensionAngleDegrees(const DimensionGeometry &dimension);
double displayedDimensionLength(double storedLength, DimensionKind kind);
double storedDimensionLength(double displayedLength, DimensionKind kind);
double area(const DraftingGeometry &geometry);
std::vector<HandleAnchor> handleAnchors(const DraftingGeometry &geometry);

// Arc helpers (shared by bounds, hit test, projection, painter, and plot flatten
// so the tessellation/angle convention lives in one place). Angles are degrees
// measured by atan2(dy, dx) in canvas (y-down) space; the sweep runs from
// startAngleDeg to endAngleDeg in increasing order.
Point2D arcPointAtAngle(Point2D center, double radius, double angleDeg);
double arcMidAngleDeg(const ArcGeometry &arc);
std::vector<Point2D> sampleArc(const ArcGeometry &arc, double maxStepDeg = 2.0);

// Closed polyline approximating an axis-aligned ellipse (shared by hit test, plot
// flatten, and canvas projection) so the tessellation lives in one place.
// Returns `segments` distinct points around the perimeter.
std::vector<Point2D> sampleEllipse(const EllipseGeometry &ellipse, int segments = 64);

// Catmull-Rom interpolation of a spline's control points into an open chain of
// sampled points (shared by bounds, hit test, plot flatten, and canvas
// projection — the curve is computed in ONE place, never stored). The curve
// passes through every control point; 0/1 points return as-is and 2 points are
// a straight segment (nothing to curve). stepsPerSegment sets the smoothness.
std::vector<Point2D> sampleSpline(const SplineGeometry &spline, int stepsPerSegment = 16);

// Crossing of two line SEGMENTS: the point only when it lies within BOTH (the
// parametric t and u each in [0,1]); a parallel/collinear or degenerate pair, or
// a crossing off either segment, returns nullopt. General geometry primitives —
// born in DraftingModify for trim/fillet, promoted here once the snap engine
// (a second, unrelated consumer) needed them too.
std::optional<Point2D> segmentIntersection(const LineGeometry &a, const LineGeometry &b);

// Crossing of the two INFINITE lines through these segments — even past an
// endpoint. Only a parallel/collinear (or zero-length) pair returns nullopt.
std::optional<Point2D> lineIntersection(const LineGeometry &a, const LineGeometry &b);

} // namespace edi::drafting
