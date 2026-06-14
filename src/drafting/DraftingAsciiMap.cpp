#include "drafting/DraftingAsciiMap.h"

#include "drafting/DraftingGeometry.h"

#include <algorithm>
#include <unordered_map>
#include <utility>

namespace edi::drafting {

std::vector<AsciiGlyph> defaultAsciiGlyphs()
{
    // The starter vocabulary. THE GLYPHS ARE DATA: extend or replace this table
    // (a custom glyph project) and the parser/builder are unchanged.
    return {
        {'#', AsciiCellKind::Wall, ""},
        {'.', AsciiCellKind::Floor, ""},
        {' ', AsciiCellKind::Floor, ""},
        {'+', AsciiCellKind::Door, "door"},
        {'s', AsciiCellKind::Door, "secret"},
        {'$', AsciiCellKind::Feature, "treasure"},
        {'M', AsciiCellKind::Feature, "monster"},
        {'N', AsciiCellKind::Feature, "npc"},
        {'T', AsciiCellKind::Feature, "trap"},
    };
}

const AsciiCell &AsciiMap::at(int row, int col) const
{
    return cells[static_cast<std::size_t>(row) * static_cast<std::size_t>(cols) + static_cast<std::size_t>(col)];
}

AsciiMapParseResult parseAsciiMap(const std::string &text, const std::vector<AsciiGlyph> &glyphs)
{
    AsciiMapParseResult out;

    std::unordered_map<char, std::pair<AsciiCellKind, std::string>> table;
    for (const AsciiGlyph &g : glyphs) {
        table[g.glyph] = {g.kind, g.tag};
    }

    // Split into lines (tolerate CRLF); drop a single trailing empty line so a
    // text ending in '\n' does not add a phantom blank row.
    std::vector<std::string> lines;
    std::string line;
    for (const char ch : text) {
        if (ch == '\n') {
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            lines.push_back(std::move(line));
            line.clear();
        } else {
            line.push_back(ch);
        }
    }
    if (!line.empty()) {
        if (line.back() == '\r') {
            line.pop_back();
        }
        lines.push_back(std::move(line));
    }
    while (!lines.empty() && lines.back().empty()) {
        lines.pop_back();
    }
    if (lines.empty()) {
        out.message = "ascii map is empty";
        return out;
    }

    int cols = 0;
    for (const std::string &l : lines) {
        cols = std::max(cols, static_cast<int>(l.size()));
    }
    if (cols == 0) {
        out.message = "ascii map is empty";
        return out;
    }

    AsciiMap map;
    map.rows = static_cast<int>(lines.size());
    map.cols = cols;
    map.cells.reserve(static_cast<std::size_t>(map.rows) * static_cast<std::size_t>(map.cols));
    for (const std::string &l : lines) {
        for (int c = 0; c < cols; ++c) {
            const char ch = c < static_cast<int>(l.size()) ? l[c] : ' ';
            AsciiCell cell;
            cell.glyph = ch;
            const auto it = table.find(ch);
            if (it != table.end()) {
                cell.kind = it->second.first;
                cell.tag = it->second.second;
            } // unknown glyph -> Floor kind, raw glyph preserved for the proof
            map.cells.push_back(std::move(cell));
        }
    }

    out.ok = true;
    out.map = std::move(map);
    return out;
}

std::string renderAsciiMap(const AsciiMap &map)
{
    std::string out;
    for (int r = 0; r < map.rows; ++r) {
        std::string row;
        for (int c = 0; c < map.cols; ++c) {
            row.push_back(map.at(r, c).glyph);
        }
        while (!row.empty() && row.back() == ' ') {
            row.pop_back(); // trailing floor is invisible; trim so the proof is clean
        }
        out += row;
        if (r + 1 < map.rows) {
            out.push_back('\n');
        }
    }
    return out;
}

namespace {

constexpr double kEps = 1e-9;

} // namespace

AsciiMapBuildResult planAsciiMapGeometry(const AsciiMap &map, Point2D origin, double cellSize,
                                         const std::function<DraftingObjectId()> &mintId)
{
    AsciiMapBuildResult out;
    if (!(cellSize > 0.0)) {
        out.code = DraftingResultCode::InvalidGeometry;
        out.message = "cellSize must be positive";
        return out;
    }

    const auto left = [&](int c) { return origin.x + c * cellSize; };
    const auto right = [&](int c) { return origin.x + (c + 1) * cellSize; };
    const auto top = [&](int r) { return origin.y + r * cellSize; };
    const auto bottom = [&](int r) { return origin.y + (r + 1) * cellSize; };
    const auto midX = [&](int c) { return origin.x + (c + 0.5) * cellSize; };
    const auto midY = [&](int r) { return origin.y + (r + 0.5) * cellSize; };
    const auto isWall = [&](int r, int c) {
        return r >= 0 && r < map.rows && c >= 0 && c < map.cols && map.at(r, c).kind == AsciiCellKind::Wall;
    };

    std::vector<bool> covered(static_cast<std::size_t>(map.rows) * static_cast<std::size_t>(map.cols), false);
    const auto mark = [&](int r, int c) { covered[static_cast<std::size_t>(r) * map.cols + c] = true; };
    const auto isCovered = [&](int r, int c) { return covered[static_cast<std::size_t>(r) * map.cols + c]; };

    bool failed = false;
    const auto emitWall = [&](Point2D a, Point2D b) {
        DraftingObjectBuildResult built = buildDraftingObject(mintId(), DraftingShapeKind::Wall, WallGeometry{a, b, cellSize});
        if (!built.ok) {
            failed = true;
            return;
        }
        built.object.bounds = computeBounds(built.object.geometry);
        built.object.metadata.toolProvenance = "ascii-wall";
        out.objects.push_back(std::move(built.object));
    };
    const auto emitMarker = [&](int r, int c, const std::string &provenance, const std::string &tag) {
        DraftingObjectBuildResult built = buildDraftingObject(mintId(), DraftingShapeKind::Point, PointGeometry{{midX(c), midY(r)}});
        if (!built.ok) {
            failed = true;
            return;
        }
        built.object.bounds = computeBounds(built.object.geometry);
        built.object.metadata.toolProvenance = provenance;
        if (!tag.empty()) {
            built.object.metadata.tags = {tag}; // neutral classification the engine resolves
        }
        out.objects.push_back(std::move(built.object));
    };

    // Maximal HORIZONTAL runs of >=2 wall cells -> a thick band one cell tall.
    for (int r = 0; r < map.rows && !failed; ++r) {
        for (int c = 0; c < map.cols;) {
            if (!isWall(r, c)) {
                ++c;
                continue;
            }
            int c2 = c;
            while (c2 + 1 < map.cols && isWall(r, c2 + 1)) {
                ++c2;
            }
            if (c2 > c) {
                emitWall({left(c), midY(r)}, {right(c2), midY(r)});
                for (int cc = c; cc <= c2; ++cc) {
                    mark(r, cc);
                }
            }
            c = c2 + 1;
        }
    }
    // Maximal VERTICAL runs of >=2 wall cells. Corner cells land in both a
    // horizontal and a vertical band; the thick bands OVERLAP and fill the
    // corner, so no miter pass is needed for the ASCII tile aesthetic.
    for (int c = 0; c < map.cols && !failed; ++c) {
        for (int r = 0; r < map.rows;) {
            if (!isWall(r, c)) {
                ++r;
                continue;
            }
            int r2 = r;
            while (r2 + 1 < map.rows && isWall(r2 + 1, c)) {
                ++r2;
            }
            if (r2 > r) {
                emitWall({midX(c), top(r)}, {midX(c), bottom(r2)});
                for (int rr = r; rr <= r2; ++rr) {
                    mark(rr, c);
                }
            }
            r = r2 + 1;
        }
    }
    // Isolated wall cells (no run) -> a single-cell square band.
    for (int r = 0; r < map.rows && !failed; ++r) {
        for (int c = 0; c < map.cols; ++c) {
            if (isWall(r, c) && !isCovered(r, c)) {
                emitWall({left(c), midY(r)}, {right(c), midY(r)});
            }
        }
    }

    // Doors break the wall run (a Door cell is not a Wall cell, so the run
    // already stops) and leave a tagged marker in the gap; features are tagged
    // placements. Both are cheap Point markers carrying a neutral tag.
    for (int r = 0; r < map.rows && !failed; ++r) {
        for (int c = 0; c < map.cols; ++c) {
            const AsciiCell &cell = map.at(r, c);
            if (cell.kind == AsciiCellKind::Door) {
                emitMarker(r, c, "ascii-door", cell.tag);
            } else if (cell.kind == AsciiCellKind::Feature) {
                emitMarker(r, c, "ascii-feature", cell.tag);
            }
        }
    }

    if (failed) {
        out.code = DraftingResultCode::DuplicateObjectId;
        out.message = "could not build an ascii-map object";
        return out;
    }
    if (out.objects.empty()) {
        out.code = DraftingResultCode::InvalidGeometry;
        out.message = "ascii map has no walls, doors, or features";
        return out;
    }
    out.ok = true;
    return out;
}

} // namespace edi::drafting
