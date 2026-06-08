#pragma once

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
    Polyline
};

enum class DraftingResultCode {
    None,
    EmptyObjectId,
    DuplicateObjectId,
    ObjectNotFound,
    LayerNotFound,
    KindGeometryMismatch,
    InvalidGeometry,
    InvalidSelectionTarget
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

struct ObjectMetadata {
    std::string author;
    std::string source;
    std::string createdAt;
    std::string toolProvenance;
    std::string measurementNote;
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

using DraftingGeometry = std::variant<
    PointGeometry,
    LineGeometry,
    RectangleGeometry,
    CircleGeometry,
    PolygonGeometry,
    PolylineGeometry>;

const char *shapeKindName(DraftingShapeKind kind);
const char *draftingResultCodeName(DraftingResultCode code);
bool isValidDraftingObjectId(const DraftingObjectId &id);
bool isValidDraftingDocumentId(const DraftingDocumentId &id);
bool isValidLayerId(const LayerId &id);
DraftingShapeKind geometryKind(const DraftingGeometry &geometry);
bool kindMatchesGeometry(DraftingShapeKind kind, const DraftingGeometry &geometry);

} // namespace edi::drafting
