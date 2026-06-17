#pragma once

#include "drafting/DraftingDocument.h"

#include <string>
#include <vector>

namespace edi::drafting {

struct DraftingTrimResult {
    bool ok = false;
    DraftingResultCode code = DraftingResultCode::None;
    std::string message;
    LineGeometry geometry; // the trimmed line — valid only when ok

    static DraftingTrimResult accepted(LineGeometry geometry);
    static DraftingTrimResult rejected(DraftingResultCode code, std::string message);
};

// Trim a target line back to where a boundary line crosses it, removing the end
// the user clicked nearer. Among the boundaries, the one whose crossing sits
// nearest the pick wins (so clicking near a particular cut selects it). Rejects
// when no boundary crosses the target, or when the trim would collapse the line
// to zero length. Pure: the controller supplies the other lines as boundaries.
DraftingTrimResult trimLineAtPoint(const LineGeometry &target,
                                   const std::vector<LineGeometry> &boundaries,
                                   Point2D pick);

struct DraftingFilletResult {
    bool ok = false;
    DraftingResultCode code = DraftingResultCode::None;
    std::string message;
    // On success: the two lines trimmed/extended to their tangent points, plus
    // the rounding arc that joins them. Caller applies all three atomically.
    LineGeometry line1;
    LineGeometry line2;
    ArcGeometry arc;

    static DraftingFilletResult accepted(LineGeometry line1, LineGeometry line2, ArcGeometry arc);
    static DraftingFilletResult rejected(DraftingResultCode code, std::string message);
};

// Round the corner between two lines with an arc of the given radius, tangent to
// both. `pick` selects which of the (up to four) corners to fillet — the arc
// sits in the corner the pick points at — and which other line to use. Each line
// is trimmed/extended so its corner-side end meets the arc's tangent point.
// Rejects parallel/collinear lines, a non-positive radius, or a fillet so large
// it would collapse a line. Pure: the controller supplies both lines + radius.
DraftingFilletResult filletLines(const LineGeometry &line1,
                                 const LineGeometry &line2,
                                 double radius,
                                 Point2D pick);

struct DraftingChamferResult {
    bool ok = false;
    DraftingResultCode code = DraftingResultCode::None;
    std::string message;
    // On success: the two lines trimmed/extended to their setback points, plus the
    // straight bevel that joins them. Caller applies all three atomically.
    LineGeometry line1;
    LineGeometry line2;
    LineGeometry bevel;

    static DraftingChamferResult accepted(LineGeometry line1, LineGeometry line2, LineGeometry bevel);
    static DraftingChamferResult rejected(DraftingResultCode code, std::string message);
};

// Bevel the corner between two lines — the angular sibling of filletLines: instead
// of a tangent arc it sets each line back from the corner by setback1/setback2
// along the line and joins those two setback points with a straight `bevel`. `pick`
// selects which of the (up to four) corners — the bevel sits in the corner the pick
// points at — and which other line. Rejects parallel/collinear lines, a
// non-positive/non-finite setback, or a setback that overruns its line. Pure.
DraftingChamferResult chamferLines(const LineGeometry &line1,
                                   const LineGeometry &line2,
                                   double setback1,
                                   double setback2,
                                   Point2D pick);

struct DraftingExtendResult {
    bool ok = false;
    DraftingResultCode code = DraftingResultCode::None;
    std::string message;
    LineGeometry geometry; // the target line, lengthened — valid only when ok

    static DraftingExtendResult accepted(LineGeometry geometry);
    static DraftingExtendResult rejected(DraftingResultCode code, std::string message);
};

// Lengthen the END of `target` nearer the `pick` along the line's direction until
// it reaches the nearest boundary the line's INFINITE extension crosses (on the
// boundary's segment) — the geometric mirror of trimLineAtPoint. Among the
// boundaries, the crossing nearest the pick wins. Rejects when no boundary crosses
// the extension (or the target is zero-length). Pure: the controller supplies the
// other lines as boundaries.
DraftingExtendResult extendLineToBoundary(const LineGeometry &target,
                                          const std::vector<LineGeometry> &boundaries,
                                          Point2D pick);

} // namespace edi::drafting
