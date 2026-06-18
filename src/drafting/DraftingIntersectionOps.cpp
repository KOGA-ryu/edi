#include "drafting/DraftingIntersectionOps.h"

#include "drafting/DraftingGeometry.h"  // segmentIntersection, LineGeometry
#include "drafting/DraftingLayerOps.h"  // draftingObjectEffectivelyVisible

#include <algorithm> // std::find
#include <optional>

namespace edi::drafting {

// Two intersection points closer than this are merged to one representative.
// 1e-6 is well below sub-pixel resolution at any workable zoom level; it
// handles the floating-point spread that a triple-crossing produces (three
// pair-wise hits at essentially the same mathematical point).
static constexpr double kIntersectionDedupTolerance = 1e-6;

namespace {

// Extract the a→b segment from `object` if it is VISIBLE and of a participating
// kind (Line or ConstructionLine).  Returns empty for every other kind — in
// particular for Guide (infinite line, no b endpoint).
//
// ConstructionLine stores its two points as `.a` and `.b` (same field names as
// LineGeometry), so the conversion is a trivial repack into LineGeometry.
std::optional<LineGeometry> visibleSegmentOf(const DraftingDocument &document,
                                              const DraftingObject &object)
{
    if (!draftingObjectEffectivelyVisible(document, object)) {
        return {};
    }
    if (object.kind == DraftingShapeKind::Line) {
        return std::get<LineGeometry>(object.geometry);
    }
    if (object.kind == DraftingShapeKind::ConstructionLine) {
        const auto &cl = std::get<ConstructionLineGeometry>(object.geometry);
        // ConstructionLineGeometry{a,b} ↔ LineGeometry{a,b} — same layout,
        // different type.  Repack so segmentIntersection can accept it.
        return LineGeometry{cl.a, cl.b};
    }
    return {};
}

// Dedup: keep only the first representative of each near-coincident cluster.
// A squared-distance compare avoids a sqrt on every pair.
std::vector<Point2D> dedup(std::vector<Point2D> hits)
{
    constexpr double eps2 = kIntersectionDedupTolerance * kIntersectionDedupTolerance;
    std::vector<Point2D> unique;
    unique.reserve(hits.size());
    for (const Point2D &p : hits) {
        bool close = false;
        for (const Point2D &u : unique) {
            const double dx = p.x - u.x;
            const double dy = p.y - u.y;
            if (dx * dx + dy * dy < eps2) {
                close = true;
                break;
            }
        }
        if (!close) {
            unique.push_back(p);
        }
    }
    return unique;
}

// Core worker: pairwise segmentIntersection over `segments`, dedup, return.
std::vector<Point2D> computeFromSegments(const std::vector<LineGeometry> &segments)
{
    std::vector<Point2D> hits;
    for (std::size_t i = 0; i < segments.size(); ++i) {
        for (std::size_t j = i + 1; j < segments.size(); ++j) {
            const auto pt = segmentIntersection(segments[i], segments[j]);
            if (pt) {
                hits.push_back(*pt);
            }
        }
    }
    return dedup(std::move(hits));
}

} // namespace

std::vector<Point2D> documentIntersectionPoints(const DraftingDocument &document)
{
    std::vector<LineGeometry> segments;
    for (const DraftingObject &object : document.objects) {
        auto seg = visibleSegmentOf(document, object);
        if (seg) {
            segments.push_back(std::move(*seg));
        }
    }
    return computeFromSegments(segments);
}

std::vector<Point2D> subsetIntersectionPoints(const DraftingDocument &document,
                                               const std::vector<DraftingObjectId> &ids)
{
    std::vector<LineGeometry> segments;
    for (const DraftingObject &object : document.objects) {
        // Only objects whose id is in the requested subset participate.
        if (std::find(ids.begin(), ids.end(), object.id) == ids.end()) {
            continue;
        }
        auto seg = visibleSegmentOf(document, object);
        if (seg) {
            segments.push_back(std::move(*seg));
        }
    }
    return computeFromSegments(segments);
}

std::vector<Point2D> filterExistingIntersections(const DraftingDocument &document,
                                                  const std::vector<Point2D> &candidates)
{
    // Same squared-distance threshold as the dedup pass: "same tolerance"
    // means a point dropped in a previous call lands within this radius of
    // the computed intersection, so a second call sees it and skips.
    constexpr double eps2 = kIntersectionDedupTolerance * kIntersectionDedupTolerance;

    std::vector<Point2D> result;
    result.reserve(candidates.size());
    for (const Point2D &candidate : candidates) {
        bool exists = false;
        for (const DraftingObject &obj : document.objects) {
            if (obj.kind != DraftingShapeKind::Point) {
                continue;
            }
            const auto *pg = std::get_if<PointGeometry>(&obj.geometry);
            if (pg == nullptr) {
                continue;
            }
            const double dx = pg->point.x - candidate.x;
            const double dy = pg->point.y - candidate.y;
            if (dx * dx + dy * dy < eps2) {
                exists = true;
                break;
            }
        }
        if (!exists) {
            result.push_back(candidate);
        }
    }
    return result;
}

} // namespace edi::drafting
