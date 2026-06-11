#include "recipe/RecipeAscii.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>

namespace edi::recipe {

namespace {

// The silhouette model: the recipe's footprint on the XY plane is a small
// set of primitive shapes. One struct with a kind tag, not a hierarchy —
// there are exactly two shapes and one question (is this point inside?).
struct FootprintShape {
    bool circle = false;
    double cx = 0.0;
    double cy = 0.0;
    double halfWidth = 0.0; // half size for rects; radius for circles
    double halfHeight = 0.0;
};

bool insideShape(const FootprintShape &shape, double x, double y)
{
    const double dx = x - shape.cx;
    const double dy = y - shape.cy;
    if (shape.circle) {
        return dx * dx + dy * dy <= shape.halfWidth * shape.halfWidth;
    }
    return std::abs(dx) <= shape.halfWidth && std::abs(dy) <= shape.halfHeight;
}

double paramValue(const ResolvedStep &step, const std::string &id, double fallback)
{
    for (const ResolvedParam &param : step.params) {
        if (param.id == id) {
            return param.value;
        }
    }
    return fallback;
}

// Walk the steps the way the shop does: stock first, then each shaper in
// order. Only footprint-changing shapers act here; bevel shapes edges in Z
// and leaves the top view alone.
std::vector<FootprintShape> footprintOf(const ResolvedRecipe &resolved)
{
    std::vector<FootprintShape> shapes;
    for (const ResolvedStep &step : resolved.steps) {
        if (step.shaperId == "cube") {
            shapes.push_back({false, 0.0, 0.0,
                paramValue(step, "size_x", 1.0) / 2.0, paramValue(step, "size_y", 1.0) / 2.0});
        } else if (step.shaperId == "cylinder") {
            const double radius = paramValue(step, "radius", 0.5);
            shapes.push_back({true, 0.0, 0.0, radius, radius});
        } else if (step.shaperId == "lathe") {
            // A turned part's footprint is the circle of its widest radius;
            // radial grooves are surface detail like bevel, no footprint.
            double maxRadius = 0.0;
            for (const auto &point : step.profilePoints) {
                maxRadius = std::max(maxRadius, point.x);
            }
            if (maxRadius > 0.0) {
                shapes.push_back({true, 0.0, 0.0, maxRadius, maxRadius});
            }
        } else if (step.shaperId == "array") {
            const int count = std::max(1, static_cast<int>(std::lround(paramValue(step, "count", 1.0))));
            const double offset = paramValue(step, "offset_x", 0.0);
            // Duplicate everything built so far at the exact spacing.
            const std::vector<FootprintShape> base = shapes;
            for (int copy = 1; copy < count; ++copy) {
                for (FootprintShape shape : base) {
                    shape.cx += offset * copy;
                    shapes.push_back(shape);
                }
            }
        }
    }
    return shapes;
}

std::string numberText(double value)
{
    char buffer[64];
    if (std::isfinite(value) && value == std::floor(value) && std::abs(value) < 1e15) {
        std::snprintf(buffer, sizeof(buffer), "%.0f", value);
    } else {
        std::snprintf(buffer, sizeof(buffer), "%.4g", value);
    }
    return buffer;
}

} // namespace

RecipeAsciiResult renderRecipeAscii(const ResolvedRecipe &resolved, int columns, int rows)
{
    RecipeAsciiResult result;
    if (!resolved.ok || resolved.steps.empty()) {
        result.message = "nothing to preview: recipe is empty or unresolved";
        return result;
    }
    if (columns < 1 || rows < 1) {
        result.message = "preview grid needs at least one cell";
        return result;
    }

    const std::vector<FootprintShape> shapes = footprintOf(resolved);
    if (shapes.empty()) {
        result.message = "recipe has no footprint to preview";
        return result;
    }

    // Bounding box of the whole footprint; degenerate extents get a floor so
    // a zero-size shape still maps onto the grid instead of dividing by zero.
    double minX = shapes.front().cx, maxX = minX, minY = shapes.front().cy, maxY = minY;
    for (const FootprintShape &shape : shapes) {
        minX = std::min(minX, shape.cx - shape.halfWidth);
        maxX = std::max(maxX, shape.cx + shape.halfWidth);
        minY = std::min(minY, shape.cy - shape.halfHeight);
        maxY = std::max(maxY, shape.cy + shape.halfHeight);
    }
    constexpr double kMinExtent = 1e-9;
    const double width = std::max(maxX - minX, kMinExtent);
    const double height = std::max(maxY - minY, kMinExtent);

    // Sample each cell at its center: a cell is '#' when that point is
    // inside any shape. Center sampling keeps edges stable — a shape
    // touching a cell border doesn't flicker the whole row on.
    std::string text;
    text.reserve(static_cast<std::size_t>((columns + 1) * rows) + 64);
    for (int row = 0; row < rows; ++row) {
        for (int column = 0; column < columns; ++column) {
            const double x = minX + (column + 0.5) * (width / columns);
            const double y = minY + (row + 0.5) * (height / rows);
            bool inside = false;
            for (const FootprintShape &shape : shapes) {
                if (insideShape(shape, x, y)) {
                    inside = true;
                    break;
                }
            }
            text += inside ? '#' : '.';
        }
        text += '\n';
    }
    text += "footprint: " + numberText(maxX - minX) + " x " + numberText(maxY - minY) + "\n";

    result.ok = true;
    result.text = std::move(text);
    return result;
}

} // namespace edi::recipe
