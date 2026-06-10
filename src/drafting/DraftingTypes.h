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

enum class DraftingShapeKind {
    Point,
    Line,
    Rectangle,
    Circle,
    Arc,
    Polygon,
    Polyline,
    Guide,
    ConstructionLine,
    Dimension
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
    Diameter
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

// N3: the legacy object "role" — a semantic tag the 3D/export pipeline reads
// (a wall extrudes, a cutout subtracts, a collider is invisible but solid).
// An enum, not a free string, because the set is closed and downstream code
// switches on it; material / export_group / tags below stay free-form
// because those vocabularies are open and user-defined.
enum class ObjectRole {
    None,
    Wall,
    Floor,
    Cutout,
    Collider
};

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
    double width = 1.0;
    double opacity = 1.0;
    std::string color = "#000000";
    std::string lineStyle = "solid";
};

struct FillStyle {
    double opacity = 0.0;
    std::string color = "#ffffff";
};

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
};

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
    // N3 semantic/export metadata (restored from legacy). role is closed
    // (enum); material/exportGroup/tags are open vocabularies (free text).
    ObjectRole role = ObjectRole::None;
    std::string material;
    std::string exportGroup;
    std::vector<std::string> tags;
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

using DraftingGeometry = std::variant<
    PointGeometry,
    LineGeometry,
    RectangleGeometry,
    CircleGeometry,
    ArcGeometry,
    PolygonGeometry,
    PolylineGeometry,
    GuideGeometry,
    ConstructionLineGeometry,
    DimensionGeometry>;

const char *shapeKindName(DraftingShapeKind kind);
// Inverse of shapeKindName; unknown names fall back to Point (the serializer
// validates names separately before trusting the result).
DraftingShapeKind shapeKindFromName(const std::string &name);
const char *guideOrientationName(GuideOrientation orientation);
const char *dimensionKindName(DimensionKind kind);
const char *objectRoleName(ObjectRole role);
// Inverse of objectRoleName; unknown names fall back to None.
ObjectRole objectRoleFromName(const std::string &name);
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
template <> constexpr DraftingShapeKind shapeKindOf<PolygonGeometry>() { return DraftingShapeKind::Polygon; }
template <> constexpr DraftingShapeKind shapeKindOf<PolylineGeometry>() { return DraftingShapeKind::Polyline; }
template <> constexpr DraftingShapeKind shapeKindOf<GuideGeometry>() { return DraftingShapeKind::Guide; }
template <> constexpr DraftingShapeKind shapeKindOf<ConstructionLineGeometry>() { return DraftingShapeKind::ConstructionLine; }
template <> constexpr DraftingShapeKind shapeKindOf<DimensionGeometry>() { return DraftingShapeKind::Dimension; }

} // namespace edi::drafting
