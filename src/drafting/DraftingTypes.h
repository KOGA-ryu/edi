#pragma once

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

namespace edi::drafting {

using DraftingDocumentId = std::string;
using DraftingObjectId = std::string;
using LayerId = std::string;
using StyleId = std::string;
// Map-graph / block-library id aliases (DraftingPlugId, DraftingConnectionId,
// DraftingBlockId) moved to DraftingMapTypes.h, included mid-file below.

enum class DraftingShapeKind {
    Point,
    Line,
    Rectangle,
    Circle,
    Arc,
    Ellipse,
    Polygon,
    Polyline,
    Guide,
    ConstructionLine,
    Dimension,
    TextAnnotation,
    Spline,
    Wall
};

enum class GuideOrientation {
    Horizontal,
    Vertical
};

enum class DimensionKind {
    Distance,
    Width,
    Height,
    Radius,
    Diameter,
    Angular // arc-angle between two rays; `offset` stores the signed included angle in degrees
};

enum class DraftingResultCode {
    None,
    EmptyObjectId,
    DuplicateObjectId,
    DuplicateLayerId,
    ObjectNotFound,
    LayerNotFound,
    KindGeometryMismatch,
    InvalidGeometry,
    InvalidSelectionTarget,
    InvalidMetadata
};

enum class MeasurementUnit {
    None,
    CanvasUnit,
    Millimeter,
    Centimeter,
    Meter,
    Inch,
    Foot
};

// ObjectRole and WallType (closed map/render enums) moved to DraftingMapTypes.h,
// included mid-file below (just before ObjectMetadata, which embeds them).

struct Point2D {
    double x = 0.0;
    double y = 0.0;
};

struct Bounds2D {
    double x = 0.0;
    double y = 0.0;
    double width = 0.0;
    double height = 0.0;
};

struct Transform2D {
    double translateX = 0.0;
    double translateY = 0.0;
    double scaleX = 1.0;
    double scaleY = 1.0;
    double rotationDeg = 0.0;
};

struct StrokeStyle {
    // width <= 0 and an empty color mean INHERIT from the layer's plot
    // style; effectiveObjectStroke resolves them. lineStyle is a purely
    // per-object axis (layers have none): "solid" | "dash" | "dot".
    double width = 0.0;
    double opacity = 1.0;
    std::string color;
    std::string lineStyle = "solid";
};

inline bool operator==(const StrokeStyle &a, const StrokeStyle &b)
{
    return a.width == b.width && a.opacity == b.opacity && a.color == b.color
        && a.lineStyle == b.lineStyle;
}

struct FillStyle {
    double opacity = 0.0;
    std::string color = "#ffffff";
};

inline bool operator==(const FillStyle &a, const FillStyle &b)
{
    return a.opacity == b.opacity && a.color == b.color;
}

struct LayerPlotStyle {
    bool plotEnabled = true;
    std::string penId = "pen_black";
    std::string strokeColor = "#d7dde8";
    double strokeWidth = 2.0;
};

struct MeasurementMetadata {
    MeasurementUnit unit = MeasurementUnit::None;
    double canvasUnitsPerRealUnit = 0.0;
    std::string label;
};

struct GuideVisualMetadata {
    std::string label;
    std::string color = "#83aeca";
    std::string dashStyle = "dash";
    bool showLabel = true;
};

struct DimensionVisualMetadata {
    bool showLabel = true;
};

// N2: the line tool's arrow variant. An arrowhead is decoration on a plain
// LineGeometry, not a new geometry kind — so it lives in metadata beside the
// other per-object visual flags (guide color, dimension label), keeping the
// geometry variant focused on shape. endArrow draws a head at the line's b
// endpoint (the second click).
struct LineVisualMetadata {
    bool endArrow = false;
    bool startArrow = false;
};

// WallVisualMetadata and BlockPlacementMetadata moved to DraftingMapTypes.h. It is
// included here, at FILE SCOPE, because ObjectMetadata below embeds those structs
// (and ObjectRole/WallType) by value — they must be complete first. The include sits
// here rather than at the top because the map document-record structs it also
// carries (DraftingPlug/MapRoom/Block) hold Point2D/Bounds2D by value, so those core
// types must be defined first (above). Namespace is closed/reopened so the
// self-namespaced header is included cleanly.
}  // namespace edi::drafting — closed so DraftingMapTypes.h is included at file scope

#include "drafting/DraftingMapTypes.h"

namespace edi::drafting {

struct ObjectMetadata {
    std::uint32_t schemaVersion = 1;
    std::string author;
    std::string source;
    std::string createdAt;
    std::string toolProvenance;
    std::string measurementNote;
    MeasurementMetadata measurement;
    GuideVisualMetadata guideVisual;
    DimensionVisualMetadata dimensionVisual;
    LineVisualMetadata lineVisual;
    WallVisualMetadata wallVisual;
    // N3 semantic/export metadata (restored from legacy). role is closed
    // (enum); material/exportGroup/tags are open vocabularies (free text).
    ObjectRole role = ObjectRole::None;
    std::string material;
    std::string exportGroup;
    std::vector<std::string> tags;
    BlockPlacementMetadata blockPlacement; // Seam B: set only on block-placed objects
};

struct PointGeometry {
    Point2D point;
};

struct LineGeometry {
    Point2D a;
    Point2D b;
};

struct RectangleGeometry {
    Point2D origin;
    double width = 0.0;
    double height = 0.0;
    double rotationDeg = 0.0;
    // N4 variants on the same geometry (box is both zero): cornerRadius rounds
    // the corners; inset hollows the rectangle into a frame of that wall
    // thickness. Decoration on the box, not three geometry kinds — every
    // line/bounds/snap path keeps treating it as the outer rectangle.
    double cornerRadius = 0.0;
    double inset = 0.0;
};

struct CircleGeometry {
    Point2D center;
    double radius = 0.0;
};

struct ArcGeometry {
    Point2D center;
    double radius = 0.0;
    double startAngleDeg = 0.0;
    double endAngleDeg = 0.0;
};

struct EllipseGeometry {
    Point2D center;
    double rx = 0.0;
    double ry = 0.0;
};

struct PolygonGeometry {
    std::vector<Point2D> vertices;
};

struct PolylineGeometry {
    std::vector<Point2D> vertices;
};

struct GuideGeometry {
    GuideOrientation orientation = GuideOrientation::Horizontal;
    double position = 0.0;
};

struct ConstructionLineGeometry {
    Point2D a;
    Point2D b;
};

struct DimensionGeometry {
    DimensionKind kind = DimensionKind::Distance;
    Point2D a;
    Point2D b;
    double offset = 0.04;
};

struct TextAnnotationGeometry {
    Point2D position; // top-left anchor of the text box
    std::string content;
    double height = 0.04; // cap height in canvas units
};

// A smooth curve through a variable-length list of CONTROL points (the user's
// multi-click trail). Named controlPoints, not vertices, on purpose: a spline's
// drawn form is a Catmull-Rom interpolation of these knots, not straight
// segments between them. That difference is why spline does NOT share the
// polygon/polyline `||` arms at the sampling sites (bounds, hit, plot, paint)
// — those operate on sampleSpline(), while translate/handles/serialize keep
// working on the raw controlPoints. See sampleSpline in DraftingGeometry.h.
struct SplineGeometry {
    std::vector<Point2D> controlPoints;
};

// M1.1 dungeon-map keystone: a wall is a THICK LINE. a/b are the centerline
// endpoints (mirrors LineGeometry exactly); thickness is the band width in
// CANVAS units (mirrors circle's scalar radius). Default 0.1 is non-zero so a
// freshly created wall renders as a visible band rather than a zero-width line.
// Miter joins between abutting walls are a later slice (M1.2); v1 caps free
// ends square.
struct WallGeometry {
    Point2D a;
    Point2D b;
    double thickness = 0.1;
};

// Compile-time exhaustiveness guard. A std::visit branch that should be
// unreachable ends in `else { static_assert(always_false_v<Geometry>, "…"); }`,
// so adding a DraftingGeometry alternative without handling it at that site is a
// BUILD error naming the function — not a silent runtime default.
// See docs/adding-a-drafting-tool.md.
template <class>
inline constexpr bool always_false_v = false;

using DraftingGeometry = std::variant<
    PointGeometry,
    LineGeometry,
    RectangleGeometry,
    CircleGeometry,
    ArcGeometry,
    EllipseGeometry,
    PolygonGeometry,
    PolylineGeometry,
    GuideGeometry,
    ConstructionLineGeometry,
    DimensionGeometry,
    TextAnnotationGeometry,
    SplineGeometry,
    WallGeometry>;

static_assert(std::variant_size_v<DraftingGeometry> == 14,
    "DraftingGeometry gained or lost an alternative — that is an exhaustive change. "
    "Every std::visit over it carries an always_false_v guard that names any site you "
    "missed; every switch over DraftingShapeKind is a site too. See "
    "docs/adding-a-drafting-tool.md, then update this count.");

const char *shapeKindName(DraftingShapeKind kind);
// Inverse of shapeKindName; unknown names fall back to Point (the serializer
// validates names separately before trusting the result).
DraftingShapeKind shapeKindFromName(const std::string &name);
const char *guideOrientationName(GuideOrientation orientation);
const char *dimensionKindName(DimensionKind kind);
// objectRoleName/objectRoleFromName/wallTypeName/wallTypeFromName declared in
// DraftingMapTypes.h (included mid-file above), beside the enums they name.
const char *draftingResultCodeName(DraftingResultCode code);
bool isValidDraftingObjectId(const DraftingObjectId &id);
bool isValidDraftingDocumentId(const DraftingDocumentId &id);
bool isValidDraftingDocumentTitle(const std::string &title);
bool isValidLayerId(const LayerId &id);
bool isValidLayerName(const std::string &name);
DraftingShapeKind geometryKind(const DraftingGeometry &geometry);
bool kindMatchesGeometry(DraftingShapeKind kind, const DraftingGeometry &geometry);

template <typename Geometry>
constexpr DraftingShapeKind shapeKindOf();
template <> constexpr DraftingShapeKind shapeKindOf<PointGeometry>() { return DraftingShapeKind::Point; }
template <> constexpr DraftingShapeKind shapeKindOf<LineGeometry>() { return DraftingShapeKind::Line; }
template <> constexpr DraftingShapeKind shapeKindOf<RectangleGeometry>() { return DraftingShapeKind::Rectangle; }
template <> constexpr DraftingShapeKind shapeKindOf<CircleGeometry>() { return DraftingShapeKind::Circle; }
template <> constexpr DraftingShapeKind shapeKindOf<ArcGeometry>() { return DraftingShapeKind::Arc; }
template <> constexpr DraftingShapeKind shapeKindOf<EllipseGeometry>() { return DraftingShapeKind::Ellipse; }
template <> constexpr DraftingShapeKind shapeKindOf<PolygonGeometry>() { return DraftingShapeKind::Polygon; }
template <> constexpr DraftingShapeKind shapeKindOf<PolylineGeometry>() { return DraftingShapeKind::Polyline; }
template <> constexpr DraftingShapeKind shapeKindOf<GuideGeometry>() { return DraftingShapeKind::Guide; }
template <> constexpr DraftingShapeKind shapeKindOf<ConstructionLineGeometry>() { return DraftingShapeKind::ConstructionLine; }
template <> constexpr DraftingShapeKind shapeKindOf<DimensionGeometry>() { return DraftingShapeKind::Dimension; }
template <> constexpr DraftingShapeKind shapeKindOf<TextAnnotationGeometry>() { return DraftingShapeKind::TextAnnotation; }
template <> constexpr DraftingShapeKind shapeKindOf<SplineGeometry>() { return DraftingShapeKind::Spline; }
template <> constexpr DraftingShapeKind shapeKindOf<WallGeometry>() { return DraftingShapeKind::Wall; }

} // namespace edi::drafting
