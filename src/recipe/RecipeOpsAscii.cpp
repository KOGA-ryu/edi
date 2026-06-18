#include "recipe/RecipeOpsAscii.h"

#include <algorithm>
#include <cfenv>
#include <cmath>
#include <functional>
#include <limits>

namespace edi::recipe {

namespace {

// Python's round() is round-half-to-EVEN; std::lround is half-away-from-
// zero. The goldens were rendered by python, so the port must round the
// same way on the exact .5 cell boundaries — std::nearbyint under the
// default FE_TONEAREST mode is ties-to-even.
int pyRound(double value)
{
    return static_cast<int>(std::nearbyint(value));
}

struct Bounds {
    double minX, maxX, minY, maxY, minZ, maxZ;
};

double boxZCenter(const AddBoxOp &op)
{
    return op.zMode == ZMode::Base ? op.z + op.height / 2.0 : op.z;
}

double cylinderZCenter(const AddCylinderOp &op)
{
    return (op.zMode == ZMode::Base && op.axis == Axis::Z) ? op.z + op.height / 2.0 : op.z;
}

void cylinderExtents(const AddCylinderOp &op, double out[6])
{
    const double z = cylinderZCenter(op);
    if (op.axis == Axis::X) {
        out[0] = op.x - op.height / 2;
        out[1] = op.x + op.height / 2;
        out[2] = op.y - op.radius;
        out[3] = op.y + op.radius;
        out[4] = z - op.radius;
        out[5] = z + op.radius;
        return;
    }
    if (op.axis == Axis::Y) {
        out[0] = op.x - op.radius;
        out[1] = op.x + op.radius;
        out[2] = op.y - op.height / 2;
        out[3] = op.y + op.height / 2;
        out[4] = z - op.radius;
        out[5] = z + op.radius;
        return;
    }
    out[0] = op.x - op.radius;
    out[1] = op.x + op.radius;
    out[2] = op.y - op.radius;
    out[3] = op.y + op.radius;
    out[4] = z - op.height / 2;
    out[5] = z + op.height / 2;
}

// Exhaustive bounds visitor — one operator() per RecipeOp arm, the same
// named-overload-struct pattern ProjectionDrawer uses below. This replaces
// the old 5-of-10 get_if ladder: the compiler now forces a framing decision
// for EVERY op (the "compiler is the checklist" property the other
// interpreters already have — docs/architecture/edi-blender-lab.md §2), so a
// future op arm cannot silently inherit no ASCII framing. The five geometry
// arms accumulate the SAME extents as before (identical numbers); the other
// five are explicit no-ops, matching today's silent fall-through exactly.
// `Include` is templated on the caller's capturing lambda so the call stays a
// direct, inlinable invocation (zero runtime cost vs. the old chain).
template <typename Include>
struct BoundsEstimator {
    const Include &include;
    double inf;

    void operator()(const AddBoxOp &box) const
    {
        const double z = boxZCenter(box);
        include(box.x - box.width / 2, box.x + box.width / 2,
                box.y - box.depth / 2, box.y + box.depth / 2,
                z - box.height / 2, z + box.height / 2);
    }

    void operator()(const AddCylinderOp &cylinder) const
    {
        double extents[6];
        cylinderExtents(cylinder, extents);
        include(extents[0], extents[1], extents[2], extents[3], extents[4], extents[5]);
    }

    void operator()(const AddSphereOp &sphere) const
    {
        include(sphere.x - sphere.radius, sphere.x + sphere.radius,
                sphere.y - sphere.radius, sphere.y + sphere.radius,
                sphere.z - sphere.radius, sphere.z + sphere.radius);
    }

    void operator()(const AddRingOp &ring) const
    {
        const double radius = ring.radius + ring.overhang;
        include(ring.x - radius, ring.x + radius,
                ring.y - radius, ring.y + radius,
                ring.z - ring.tubeHeight / 2, ring.z + ring.tubeHeight / 2);
    }

    void operator()(const AddMouldingOp &moulding) const
    {
        if (moulding.profile.empty()) {
            return; // empty-profile early-out, as before
        }
        double radius = 0.0;
        double minLocalZ = inf;
        double maxLocalZ = -inf;
        for (const MouldingPoint &point : moulding.profile) {
            radius = std::max(radius, point.radius);
            minLocalZ = std::min(minLocalZ, point.z);
            maxLocalZ = std::max(maxLocalZ, point.z);
        }
        include(moulding.x - radius, moulding.x + radius,
                moulding.y - radius, moulding.y + radius,
                moulding.baseZ + minLocalZ, moulding.baseZ + maxLocalZ);
    }

    // The remaining five arms contribute no framing today — explicit no-ops,
    // exactly as the old ladder fell through. They are spelled out so a future
    // op arm can't silently join this set.
    void operator()(const AddProfileMouldingOp &) const {}
    void operator()(const AddRevolvedProfileOp &) const {}
    // AddExtrudedProfile (BL-01) is not framed pre-lowering, like the lathe:
    // renderOpsProjection refuses it before dispatch, so this stays a no-op.
    void operator()(const AddExtrudedProfileOp &) const {}
    void operator()(const AddSweepProfileOp &) const {} // BL-08: refused pre-lowering, like the extrude
    void operator()(const CutFlutesOp &) const {}
    // AddBoolean (BL-11) adds no new geometry — its operands are framed by their
    // own ops — so it contributes nothing to the bounds.
    void operator()(const AddBooleanOp &) const {}
    // AddLabel stays a no-op HERE even though Python's bounds_of
    // (edi_craft.py ~:440) folds the label point into its frame — the C++ and
    // Python framing rigs intentionally differ; do NOT change behavior to match.
    void operator()(const AddLabelOp &) const {}

    // M1 (roadmap): a custom craftsman's TRUE bounds come from its Python
    // proof_mesh, which the C++ ASCII layer cannot access. A ±0.5 unit cube
    // at the placement is the placeholder extent — just enough to frame the
    // canvas so the Script marker is never clipped by the default bounds.
    void operator()(const ScriptOp &op) const
    {
        include(op.x - 0.5, op.x + 0.5, op.y - 0.5, op.y + 0.5, op.z - 0.5, op.z + 0.5);
    }

    // AddPrism (BL-03) is a buildable carrier whose proof tier is the OBJ mesh
    // (BL-04), not 2D ASCII — like ScriptOp it frames nothing here.
    void operator()(const AddPrismOp &) const {}
};

// v0's estimate_bounds for the architectural core: CutFlutes and AddLabel
// contribute nothing; the taper/entasis bulge is deliberately ignored
// (v0's choice — the 2.0 pad absorbs it; preserved for byte fidelity).
Bounds estimateBounds(const std::vector<RecipeOp> &ops)
{
    const double inf = std::numeric_limits<double>::infinity();
    Bounds bounds{inf, -inf, inf, -inf, inf, -inf};
    const auto include = [&bounds](double x0, double x1, double y0, double y1, double z0, double z1) {
        bounds.minX = std::min(bounds.minX, x0);
        bounds.maxX = std::max(bounds.maxX, x1);
        bounds.minY = std::min(bounds.minY, y0);
        bounds.maxY = std::max(bounds.maxY, y1);
        bounds.minZ = std::min(bounds.minZ, z0);
        bounds.maxZ = std::max(bounds.maxZ, z1);
    };

    for (const RecipeOp &op : ops) {
        std::visit(BoundsEstimator<decltype(include)>{include, inf}, op);
    }

    if (bounds.minX == inf) {
        return {-1, 1, -1, 1, 0, 1};
    }
    const double pad = 2.0;
    return {bounds.minX - pad, bounds.maxX + pad, bounds.minY - pad,
            bounds.maxY + pad, bounds.minZ - pad, bounds.maxZ + pad};
}

// Cells are UTF-8 glyph strings, not chars — the vocabulary is data.
class AsciiCanvas {
public:
    AsciiCanvas(int width, int height, std::string empty)
        : m_width(width), m_height(height),
          m_cells(static_cast<std::size_t>(width) * height, std::move(empty)) {}

    int width() const { return m_width; }
    int height() const { return m_height; }

    void set(int x, int y, const std::string &glyph)
    {
        if (x >= 0 && x < m_width && y >= 0 && y < m_height) {
            m_cells[static_cast<std::size_t>(y) * m_width + x] = glyph;
        }
    }

    void lineH(int y, int x1, int x2, const std::string &glyph)
    {
        for (int x = std::min(x1, x2); x <= std::max(x1, x2); ++x) {
            set(x, y, glyph);
        }
    }

    void lineV(int x, int y1, int y2, const std::string &glyph)
    {
        for (int y = std::min(y1, y2); y <= std::max(y1, y2); ++y) {
            set(x, y, glyph);
        }
    }

    void rect(int x1, int y1, int x2, int y2, const std::string &fill, const std::string &border)
    {
        const int xa = std::min(x1, x2);
        const int xb = std::max(x1, x2);
        const int ya = std::min(y1, y2);
        const int yb = std::max(y1, y2);
        for (int y = ya; y <= yb; ++y) {
            for (int x = xa; x <= xb; ++x) {
                set(x, y, (x == xa || x == xb || y == ya || y == yb) ? border : fill);
            }
        }
    }

    void circle(int cx, int cy, int r, const std::string &fill, const std::string &border)
    {
        if (r <= 0) {
            return;
        }
        for (int y = cy - r; y <= cy + r; ++y) {
            for (int x = cx - r; x <= cx + r; ++x) {
                const double d = std::sqrt(double(x - cx) * (x - cx) + double(y - cy) * (y - cy));
                if (d <= r) {
                    set(x, y, std::abs(d - r) < 1.0 ? border : fill);
                }
            }
        }
    }

    // Bresenham, exactly as v0 wrote it — the proof must rasterize the
    // same cells the goldens hold.
    void line(int x1, int y1, int x2, int y2, const std::string &glyph)
    {
        const int dx = std::abs(x2 - x1);
        const int dy = -std::abs(y2 - y1);
        const int sx = x1 < x2 ? 1 : -1;
        const int sy = y1 < y2 ? 1 : -1;
        int err = dx + dy;
        int x = x1;
        int y = y1;
        while (true) {
            set(x, y, glyph);
            if (x == x2 && y == y2) {
                break;
            }
            const int e2 = 2 * err;
            if (e2 >= dy) {
                err += dy;
                x += sx;
            }
            if (e2 <= dx) {
                err += dx;
                y += sy;
            }
        }
    }

    void text(int x, int y, const std::string &ascii)
    {
        for (std::size_t i = 0; i < ascii.size(); ++i) {
            set(x + static_cast<int>(i), y, std::string(1, ascii[i]));
        }
    }

    std::string render(const std::string &empty) const
    {
        std::string out;
        for (int y = 0; y < m_height; ++y) {
            int lastUsed = -1;
            for (int x = 0; x < m_width; ++x) {
                if (m_cells[static_cast<std::size_t>(y) * m_width + x] != empty) {
                    lastUsed = x;
                }
            }
            for (int x = 0; x <= lastUsed; ++x) {
                out += m_cells[static_cast<std::size_t>(y) * m_width + x];
            }
            if (y + 1 < m_height) {
                out += '\n';
            }
        }
        return out;
    }

private:
    int m_width;
    int m_height;
    std::vector<std::string> m_cells;
};

using Mapper = std::function<std::pair<int, int>(double, double)>;

void drawCylinderElevation(AsciiCanvas &canvas, const Mapper &mapper, const AsciiGlyphSet &glyphs,
                           double centerAxis, double centerZ, double radius, double topRadius,
                           double height, bool entasis, double entasisRatio)
{
    const bool shaped = entasis || std::abs(topRadius - radius) > 1e-9;
    const int segments = shaped ? 20 : 1;
    std::vector<std::pair<int, int>> leftPoints;
    std::vector<std::pair<int, int>> rightPoints;
    for (int segment = 0; segment <= segments; ++segment) {
        const double t = static_cast<double>(segment) / segments;
        const double z = centerZ - height / 2 + height * t;
        const double linearRadius = radius + (topRadius - radius) * t;
        // v0 hardcoded the 0.045 bulge here too; the op's entasisRatio
        // feeds it now (the default IS v0's constant).
        const double bulge = entasis ? std::sin(M_PI * t) * radius * entasisRatio : 0.0;
        const double rr = std::max(0.001, linearRadius + bulge);
        const auto left = mapper(centerAxis - rr, z);
        const auto right = mapper(centerAxis + rr, z);
        leftPoints.push_back(left);
        rightPoints.push_back(right);
        canvas.lineH(left.second, left.first, right.first, glyphs.cylinderFill);
    }
    canvas.lineH(leftPoints.front().second, leftPoints.front().first, rightPoints.front().first, glyphs.capBottom);
    canvas.lineH(leftPoints.back().second, leftPoints.back().first, rightPoints.back().first, glyphs.capTop);
    for (int index = 0; index < segments; ++index) {
        canvas.line(leftPoints[index].first, leftPoints[index].second,
                    leftPoints[index + 1].first, leftPoints[index + 1].second, glyphs.border);
        canvas.line(rightPoints[index].first, rightPoints[index].second,
                    rightPoints[index + 1].first, rightPoints[index + 1].second, glyphs.border);
    }
}

void drawMouldingElevation(AsciiCanvas &canvas, const Mapper &mapper, const AsciiGlyphSet &glyphs,
                           double centerAxis, double baseZ, const std::vector<MouldingPoint> &profile)
{
    std::vector<std::pair<int, int>> leftPoints;
    std::vector<std::pair<int, int>> rightPoints;
    for (const MouldingPoint &point : profile) {
        const double z = baseZ + point.z;
        const auto left = mapper(centerAxis - point.radius, z);
        const auto right = mapper(centerAxis + point.radius, z);
        leftPoints.push_back(left);
        rightPoints.push_back(right);
        canvas.lineH(left.second, left.first, right.first, glyphs.mouldingFill);
    }
    for (std::size_t index = 0; index + 1 < leftPoints.size(); ++index) {
        canvas.line(leftPoints[index].first, leftPoints[index].second,
                    leftPoints[index + 1].first, leftPoints[index + 1].second, glyphs.border);
        canvas.line(rightPoints[index].first, rightPoints[index].second,
                    rightPoints[index + 1].first, rightPoints[index + 1].second, glyphs.border);
    }
}

struct ProjectionDrawer {
    AsciiCanvas &canvas;
    const Mapper &mapper;
    const AsciiGlyphSet &glyphs;
    AsciiProjection projection;

    void operator()(const AddBoxOp &op) const
    {
        if (projection == AsciiProjection::Top) {
            const auto a = mapper(op.x - op.width / 2, op.y - op.depth / 2);
            const auto b = mapper(op.x + op.width / 2, op.y + op.depth / 2);
            canvas.rect(a.first, a.second, b.first, b.second, glyphs.boxFill, glyphs.border);
            return;
        }
        const double z = boxZCenter(op);
        const double axis = projection == AsciiProjection::Front ? op.x : op.y;
        const double half = projection == AsciiProjection::Front ? op.width / 2 : op.depth / 2;
        const auto a = mapper(axis - half, z - op.height / 2);
        const auto b = mapper(axis + half, z + op.height / 2);
        canvas.rect(a.first, a.second, b.first, b.second, glyphs.boxFill, glyphs.border);
    }

    void operator()(const AddCylinderOp &op) const
    {
        const double z = cylinderZCenter(op);
        const double topRadius = op.taperTopRadius.value_or(op.radius);
        if (projection == AsciiProjection::Top) {
            if (op.axis == Axis::X || op.axis == Axis::Y) {
                const double halfW = op.axis == Axis::X ? op.height / 2 : op.radius;
                const double halfD = op.axis == Axis::X ? op.radius : op.height / 2;
                const auto a = mapper(op.x - halfW, op.y - halfD);
                const auto b = mapper(op.x + halfW, op.y + halfD);
                canvas.rect(a.first, a.second, b.first, b.second, glyphs.cylinderFill, glyphs.border);
            } else {
                const auto center = mapper(op.x, op.y);
                const auto edge = mapper(op.x + op.radius, op.y);
                canvas.circle(center.first, center.second, std::abs(edge.first - center.first),
                              glyphs.cylinderFill, glyphs.border);
            }
            return;
        }
        const bool front = projection == AsciiProjection::Front;
        const Axis flatAxis = front ? Axis::X : Axis::Y;   // rect when along this axis
        const Axis roundAxis = front ? Axis::Y : Axis::X;  // circle when along this one
        const double axisCenter = front ? op.x : op.y;
        if (op.axis == flatAxis) {
            const auto a = mapper(axisCenter - op.height / 2, z - op.radius);
            const auto b = mapper(axisCenter + op.height / 2, z + op.radius);
            canvas.rect(a.first, a.second, b.first, b.second, glyphs.cylinderFill, glyphs.border);
        } else if (op.axis == roundAxis) {
            const auto center = mapper(axisCenter, z);
            const auto edge = mapper(axisCenter + op.radius, z);
            canvas.circle(center.first, center.second, std::abs(edge.first - center.first),
                          glyphs.cylinderFill, glyphs.border);
        } else {
            drawCylinderElevation(canvas, mapper, glyphs, axisCenter, z, op.radius, topRadius,
                                  op.height, op.entasis, op.entasisRatio);
        }
    }

    void operator()(const AddSphereOp &op) const
    {
        const double axisCenter = projection == AsciiProjection::Front ? op.x
            : projection == AsciiProjection::Side ? op.y
                                                  : op.x;
        const double other = projection == AsciiProjection::Top ? op.y : op.z;
        const auto center = mapper(axisCenter, other);
        const auto edge = mapper(axisCenter + op.radius, other);
        canvas.circle(center.first, center.second, std::abs(edge.first - center.first),
                      glyphs.sphereFill, glyphs.border);
    }

    void operator()(const AddRingOp &op) const
    {
        const double r = op.radius + op.overhang;
        if (projection == AsciiProjection::Top) {
            const auto center = mapper(op.x, op.y);
            const auto edge = mapper(op.x + r, op.y);
            canvas.circle(center.first, center.second, std::abs(edge.first - center.first),
                          glyphs.ringFill, glyphs.border);
            return;
        }
        const double axisCenter = projection == AsciiProjection::Front ? op.x : op.y;
        const auto a = mapper(axisCenter - r, op.z - op.tubeHeight / 2);
        const auto b = mapper(axisCenter + r, op.z + op.tubeHeight / 2);
        canvas.rect(a.first, a.second, b.first, b.second, glyphs.ringFill, glyphs.border);
    }

    void operator()(const AddMouldingOp &op) const
    {
        if (projection == AsciiProjection::Top) {
            // Port divergence: v0's bare max() raises on an empty profile;
            // the 0.0-seeded fold draws nothing instead. Unreachable past
            // validation (short_moulding_profile), kept calm regardless.
            double radius = 0.0;
            for (const MouldingPoint &point : op.profile) {
                radius = std::max(radius, point.radius);
            }
            const auto center = mapper(op.x, op.y);
            const auto edge = mapper(op.x + radius, op.y);
            canvas.circle(center.first, center.second, std::abs(edge.first - center.first),
                          glyphs.mouldingFill, glyphs.border);
            return;
        }
        const double axisCenter = projection == AsciiProjection::Front ? op.x : op.y;
        drawMouldingElevation(canvas, mapper, glyphs, axisCenter, op.baseZ, op.profile);
    }

    void operator()(const AddProfileMouldingOp &) const {} // refused before dispatch
    void operator()(const AddRevolvedProfileOp &) const {} // refused before dispatch
    void operator()(const AddExtrudedProfileOp &) const {} // refused before dispatch
    void operator()(const AddSweepProfileOp &) const {}    // refused before dispatch (BL-08)

    void operator()(const CutFlutesOp &op) const
    {
        if (projection == AsciiProjection::Front) {
            // v0's deliberate schematic: "rhythm markers only, because actual
            // radial boolean cuts belong to the Blender backend."
            const int count = std::min(op.count, 32);
            for (int i = 0; i < count; ++i) {
                const int x = pyRound(canvas.width() * (0.30 + 0.40 * (static_cast<double>(i) / std::max(1, count - 1))));
                canvas.lineV(x, pyRound(canvas.height() * 0.20), pyRound(canvas.height() * 0.78), glyphs.fluteMark);
            }
        } else if (projection == AsciiProjection::Top) {
            const int cx = canvas.width() / 2;
            const int cy = canvas.height() / 2;
            const int r1 = pyRound(std::min(canvas.width(), canvas.height()) * 0.12);
            const int r2 = pyRound(std::min(canvas.width(), canvas.height()) * 0.28);
            for (int i = 0; i < op.count; ++i) {
                const double angle = 2.0 * M_PI * i / op.count;
                canvas.line(pyRound(cx + std::cos(angle) * r1), pyRound(cy + std::sin(angle) * r1),
                            pyRound(cx + std::cos(angle) * r2), pyRound(cy + std::sin(angle) * r2),
                            glyphs.fluteMark);
            }
        }
        // Side projection: v0 draws nothing — the rhythm is a front/top fact.
    }

    void operator()(const AddLabelOp &) const {} // labels are Blender-side text

    // M1 (roadmap): unit-bbox placeholder at the op's placement. A custom
    // craftsman's real shape is its Python proof_mesh — the C++ ASCII layer
    // has no access to the manifest dimensions — so a ±0.5 cube is the
    // placement marker. OBJ (edi_craft --obj-out) is the real proof tier.
    // The four edge lines (not a filled rect) avoid clobbering any other op
    // that may have drawn inside this placeholder region.
    void operator()(const ScriptOp &op) const
    {
        // drawOutline maps two world corners to canvas and draws the 4 bounding
        // edges using lineH/lineV — the same axis-aligned primitives that
        // drawCylinderElevation and the flute markers use.
        const auto drawOutline = [&](double h0, double h1, double v0, double v1) {
            const auto a = mapper(h0, v0);
            const auto b = mapper(h1, v1);
            const int xa = std::min(a.first, b.first);
            const int xb = std::max(a.first, b.first);
            const int ya = std::min(a.second, b.second);
            const int yb = std::max(a.second, b.second);
            canvas.lineH(ya, xa, xb, glyphs.border);
            canvas.lineH(yb, xa, xb, glyphs.border);
            canvas.lineV(xa, ya, yb, glyphs.border);
            canvas.lineV(xb, ya, yb, glyphs.border);
        };
        if (projection == AsciiProjection::Front) {
            drawOutline(op.x - 0.5, op.x + 0.5, op.z - 0.5, op.z + 0.5);
        } else if (projection == AsciiProjection::Side) {
            drawOutline(op.y - 0.5, op.y + 0.5, op.z - 0.5, op.z + 0.5);
        } else { // Top
            drawOutline(op.x - 0.5, op.x + 0.5, op.y - 0.5, op.y + 0.5);
        }
    }

    // AddPrism (BL-03): the lowered extrude carrier. Like ScriptOp its proof is
    // the OBJ mesh (BL-04), not a 2D silhouette, so this draws nothing — and,
    // unlike the profile-reference ops, it is NOT refused before dispatch.
    void operator()(const AddPrismOp &) const {}

    // AddBoolean (BL-11): an OBJ-tier composer — its operands are drawn by their
    // own ops, and the CSG result is execution-only. So it draws nothing here
    // and, unlike the profile-reference ops, is NOT refused before dispatch.
    void operator()(const AddBooleanOp &) const {}
};

} // namespace

AsciiRenderResult renderOpsProjection(const std::vector<RecipeOp> &ops,
                                      AsciiProjection projection,
                                      int width,
                                      int height,
                                      const AsciiGlyphSet &glyphs)
{
    AsciiRenderResult result;
    for (const RecipeOp &op : ops) {
        if (const auto *uncompiled = std::get_if<AddProfileMouldingOp>(&op)) {
            // Port divergence: v0 silently skipped these, so the proof could
            // show a column missing its mouldings without saying so.
            result.message = "AddProfileMoulding must be compiled before preview: " + uncompiled->name;
            return result;
        }
        // Same contract for the lathe reference (R1-B04): a proof tool must
        // refuse what it cannot show, and an unresolved profile has no points.
        if (const auto *unresolved = std::get_if<AddRevolvedProfileOp>(&op)) {
            result.message = "AddRevolvedProfile must be resolved before preview: " + unresolved->name;
            return result;
        }
        // Same contract for the extrude (BL-01): unlowered, it has no mesh to
        // silhouette, so refuse it before dispatch — the empty ProjectionDrawer
        // and BoundsEstimator arms above are thereby unreachable.
        if (const auto *unresolved = std::get_if<AddExtrudedProfileOp>(&op)) {
            result.message = "AddExtrudedProfile must be resolved before preview: " + unresolved->name;
            return result;
        }
        // Same contract for the Follow-Me sweep (BL-08).
        if (const auto *unresolved = std::get_if<AddSweepProfileOp>(&op)) {
            result.message = "AddSweepProfile must be resolved before preview: " + unresolved->name;
            return result;
        }
    }

    const Bounds bounds = estimateBounds(ops);
    AsciiCanvas canvas(width, height, glyphs.empty);

    const double spanX = std::max(1e-9, bounds.maxX - bounds.minX);
    const double spanY = std::max(1e-9, bounds.maxY - bounds.minY);
    const double spanZ = std::max(1e-9, bounds.maxZ - bounds.minZ);
    const Mapper mapper = [&](double a, double b) -> std::pair<int, int> {
        switch (projection) {
        case AsciiProjection::Front:
            return {pyRound((a - bounds.minX) / spanX * (width - 1)),
                    pyRound((1.0 - (b - bounds.minZ) / spanZ) * (height - 1))};
        case AsciiProjection::Side:
            return {pyRound((a - bounds.minY) / spanY * (width - 1)),
                    pyRound((1.0 - (b - bounds.minZ) / spanZ) * (height - 1))};
        case AsciiProjection::Top:
        default:
            return {pyRound((a - bounds.minX) / spanX * (width - 1)),
                    pyRound((b - bounds.minY) / spanY * (height - 1))};
        }
    };

    for (const RecipeOp &op : ops) {
        std::visit(ProjectionDrawer{canvas, mapper, glyphs, projection}, op);
    }

    const char *title = projection == AsciiProjection::Front ? "FRONT PROJECTION"
        : projection == AsciiProjection::Side ? "SIDE PROJECTION"
                                              : "TOP PROJECTION";
    canvas.text(1, 1, title);

    result.ok = true;
    result.text = canvas.render(glyphs.empty);
    return result;
}

} // namespace edi::recipe
