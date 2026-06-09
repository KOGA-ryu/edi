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
};

struct CircleGeometry {
    Point2D center;
    double radius = 0.0;
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
    PolygonGeometry,
    PolylineGeometry,
    GuideGeometry,
    ConstructionLineGeometry,
    DimensionGeometry>;

const char *shapeKindName(DraftingShapeKind kind);
const char *guideOrientationName(GuideOrientation orientation);
const char *dimensionKindName(DimensionKind kind);
const char *draftingResultCodeName(DraftingResultCode code);
bool isValidDraftingObjectId(const DraftingObjectId &id);
bool isValidDraftingDocumentId(const DraftingDocumentId &id);
bool isValidDraftingDocumentTitle(const std::string &title);
bool isValidLayerId(const LayerId &id);
bool isValidLayerName(const std::string &name);
DraftingShapeKind geometryKind(const DraftingGeometry &geometry);
bool kindMatchesGeometry(DraftingShapeKind kind, const DraftingGeometry &geometry);

} // namespace edi::drafting
