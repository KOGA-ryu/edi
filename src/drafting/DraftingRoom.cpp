#include "drafting/DraftingRoom.h"

#include "drafting/DraftingGeometry.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace edi::drafting {

DraftingRoomPlan DraftingRoomPlan::accepted(std::vector<DraftingObject> objects)
{
    DraftingRoomPlan plan;
    plan.ok = true;
    plan.code = DraftingResultCode::None;
    plan.objects = std::move(objects);
    return plan;
}

DraftingRoomPlan DraftingRoomPlan::rejected(DraftingResultCode code, std::string message)
{
    DraftingRoomPlan plan;
    plan.ok = false;
    plan.code = code;
    plan.message = std::move(message);
    return plan;
}

namespace {

constexpr double kEps = 1e-9;

// One edge as a start point + unit direction + length, so an opening's offset
// maps to a point with `start + dir * offset` regardless of which way the edge
// runs. Edges run clockwise (see RoomEdge), so consecutive edges share a corner.
struct EdgeGeometry {
    Point2D start;
    double dirX;
    double dirY;
    double length;
};

EdgeGeometry edgeGeometry(const RoomSpec &spec, RoomEdge edge)
{
    const double x = spec.origin.x;
    const double y = spec.origin.y;
    switch (edge) {
    case RoomEdge::North:
        return {{x, y}, 1.0, 0.0, spec.width};
    case RoomEdge::East:
        return {{x + spec.width, y}, 0.0, 1.0, spec.height};
    case RoomEdge::South:
        return {{x + spec.width, y + spec.height}, -1.0, 0.0, spec.width};
    case RoomEdge::West:
        return {{x, y + spec.height}, 0.0, -1.0, spec.height};
    }
    return {{x, y}, 1.0, 0.0, spec.width};
}

} // namespace

DraftingRoomPlan planDraftingRoom(const RoomSpec &spec,
                                  const std::function<DraftingObjectId()> &mintId)
{
    if (!std::isfinite(spec.origin.x) || !std::isfinite(spec.origin.y)
        || !std::isfinite(spec.width) || !std::isfinite(spec.height)
        || !std::isfinite(spec.wallThickness)) {
        return DraftingRoomPlan::rejected(DraftingResultCode::InvalidGeometry, "room fields must be finite");
    }
    if (spec.width <= 0.0 || spec.height <= 0.0) {
        return DraftingRoomPlan::rejected(DraftingResultCode::InvalidGeometry, "room must have positive width and height");
    }
    if (spec.wallThickness < 0.0) {
        return DraftingRoomPlan::rejected(DraftingResultCode::InvalidGeometry, "wall thickness must be non-negative");
    }

    std::vector<DraftingObject> walls;
    const RoomEdge edges[] = {RoomEdge::North, RoomEdge::East, RoomEdge::South, RoomEdge::West};
    for (const RoomEdge edge : edges) {
        const EdgeGeometry g = edgeGeometry(spec, edge);

        // Collect this edge's openings as [start, end] intervals along it.
        std::vector<std::pair<double, double>> gaps;
        for (const RoomOpening &opening : spec.openings) {
            if (opening.edge != edge) {
                continue;
            }
            if (!std::isfinite(opening.center) || !std::isfinite(opening.width) || opening.width <= 0.0) {
                return DraftingRoomPlan::rejected(DraftingResultCode::InvalidGeometry, "opening width must be positive");
            }
            const double s = opening.center - opening.width / 2.0;
            const double e = opening.center + opening.width / 2.0;
            if (s < -kEps || e > g.length + kEps) {
                return DraftingRoomPlan::rejected(DraftingResultCode::InvalidGeometry, "opening exceeds its wall");
            }
            gaps.emplace_back(std::max(0.0, s), std::min(g.length, e));
        }
        std::sort(gaps.begin(), gaps.end());
        for (std::size_t i = 1; i < gaps.size(); ++i) {
            if (gaps[i].first < gaps[i - 1].second - kEps) {
                return DraftingRoomPlan::rejected(DraftingResultCode::InvalidGeometry, "openings overlap on a wall");
            }
        }

        // Emit a wall segment for each SOLID span (the complement of the gaps).
        // A degenerate span (an opening that reaches a corner) is skipped, which
        // leaves that corner open — the right behaviour.
        bool buildFailed = false;
        const auto emitSpan = [&](double from, double to) {
            if (to - from <= kEps) {
                return;
            }
            const Point2D a{g.start.x + g.dirX * from, g.start.y + g.dirY * from};
            const Point2D b{g.start.x + g.dirX * to, g.start.y + g.dirY * to};
            DraftingObjectBuildResult built = buildDraftingObject(
                mintId(), DraftingShapeKind::Wall, WallGeometry{a, b, spec.wallThickness});
            if (!built.ok) {
                buildFailed = true;
                return;
            }
            built.object.bounds = computeBounds(built.object.geometry);
            built.object.metadata.material = spec.wallMaterial; // neutral classification
            built.object.metadata.toolProvenance = "room";
            walls.push_back(std::move(built.object));
        };

        double pos = 0.0;
        for (const auto &gap : gaps) {
            if (gap.first > pos + kEps) {
                emitSpan(pos, gap.first);
            }
            pos = std::max(pos, gap.second);
        }
        if (g.length > pos + kEps) {
            emitSpan(pos, g.length);
        }
        if (buildFailed) {
            return DraftingRoomPlan::rejected(DraftingResultCode::DuplicateObjectId, "could not build a room wall segment");
        }
    }

    if (walls.empty()) {
        return DraftingRoomPlan::rejected(DraftingResultCode::InvalidGeometry, "room has no solid walls");
    }

    // Plug markers: one Point per authored plug, at its edge offset. The marker is
    // a real document object (so it renders and selects); the plug record in the
    // map graph anchors to it by id. Emitted AFTER the wall check so a wall-less
    // room is still rejected, and INTO `walls` so the caller creates everything in
    // one atomic batch. The same emitMarker shape DraftingAsciiMap uses: a Point
    // with toolProvenance "plug" and a neutral "plug:<name>" tag.
    std::vector<RoomPlugPlacement> plugs;
    plugs.reserve(spec.plugs.size());
    for (const RoomPlugSpec &plug : spec.plugs) {
        const EdgeGeometry g = edgeGeometry(spec, plug.edge);
        if (!std::isfinite(plug.at) || plug.at < -kEps || plug.at > g.length + kEps) {
            return DraftingRoomPlan::rejected(DraftingResultCode::InvalidGeometry, "plug is off its wall");
        }
        const Point2D at{g.start.x + g.dirX * plug.at, g.start.y + g.dirY * plug.at};
        DraftingObjectBuildResult built = buildDraftingObject(mintId(), DraftingShapeKind::Point, PointGeometry{at});
        if (!built.ok) {
            return DraftingRoomPlan::rejected(DraftingResultCode::DuplicateObjectId, "could not build a plug marker");
        }
        built.object.bounds = computeBounds(built.object.geometry);
        built.object.metadata.toolProvenance = "plug";
        if (!plug.name.empty()) {
            built.object.metadata.tags = {"plug:" + plug.name}; // neutral, engine-resolved
        }

        RoomPlugPlacement placement;
        placement.anchorObjectId = built.object.id;
        placement.name = plug.name;
        placement.type = plug.type;
        placement.anchor = at;
        placement.edge = plug.edge;
        plugs.push_back(std::move(placement));
        walls.push_back(std::move(built.object));
    }

    DraftingRoomPlan plan = DraftingRoomPlan::accepted(std::move(walls));
    plan.plugs = std::move(plugs);
    return plan;
}

} // namespace edi::drafting
