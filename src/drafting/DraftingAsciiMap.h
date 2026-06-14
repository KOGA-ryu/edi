#pragma once

#include "drafting/DraftingDocument.h"

#include <functional>
#include <string>
#include <vector>

namespace edi::drafting {

// What a cell IS, structurally. The glyph that produced it is a separate, data
// concern (the table below) — the parser/builder switch on KIND, never on the
// character, so the glyph vocabulary is swappable without touching them. This is
// the recipe system's "the glyphs are data" principle (RecipeOpsAscii) carried
// into 2D maps.
enum class AsciiCellKind {
    Floor,   // empty space — drawn as nothing (v1)
    Wall,    // solid — runs of these become wall bands
    Door,    // an opening hosted in a wall line — a gap + a marker
    Feature  // treasure / monster / npc / trap / … — a tagged placement
};

// One glyph's meaning. `tag` is the NEUTRAL classification the engine resolves
// (door/secret for a Door; treasure/monster/trap/npc for a Feature). Drafting
// stores it; it never interprets it.
struct AsciiGlyph {
    char glyph = ' ';
    AsciiCellKind kind = AsciiCellKind::Floor;
    std::string tag;
};

// The default vocabulary. Swap or extend it (a custom glyph project) without
// touching the parser or the geometry builder.
std::vector<AsciiGlyph> defaultAsciiGlyphs();

// One parsed cell: the RAW glyph (so a re-render round-trips exactly, even for
// glyphs outside the table) plus its resolved kind + tag.
struct AsciiCell {
    char glyph = ' ';
    AsciiCellKind kind = AsciiCellKind::Floor;
    std::string tag;
};

struct AsciiMap {
    int rows = 0;
    int cols = 0;
    std::vector<AsciiCell> cells; // row-major, rows*cols
    const AsciiCell &at(int row, int col) const;
};

struct AsciiMapParseResult {
    bool ok = false;
    std::string message;
    AsciiMap map;
};

// Parse a glyph grid into a typed cell grid. Lines become rows; cols is the
// widest line; short lines pad with floor. Unknown glyphs resolve to Floor kind
// but keep their raw character (so the proof re-render is faithful).
AsciiMapParseResult parseAsciiMap(const std::string &text,
                                  const std::vector<AsciiGlyph> &glyphs = defaultAsciiGlyphs());

// THE CONTROL GATE: re-render the parsed map back to ASCII. A clean grid
// round-trips, so "what you see is what gets built" — the proof stage before any
// geometry is committed (the recipe doctrine: recipe is truth, ASCII is proof).
std::string renderAsciiMap(const AsciiMap &map);

struct AsciiMapBuildResult {
    bool ok = false;
    DraftingResultCode code = DraftingResultCode::None;
    std::string message;
    std::vector<DraftingObject> objects;
};

// Derive drafting geometry from the grid (canvas units): maximal runs of Wall
// cells become thick wall bands (thickness = one cell, so corners fill by
// overlap); Door cells break the run and leave a tagged marker in the gap;
// Feature cells become tagged point markers the engine resolves later. `mintId`
// supplies a fresh id per emitted object (the caller owns id minting).
AsciiMapBuildResult planAsciiMapGeometry(const AsciiMap &map, Point2D origin, double cellSize,
                                         const std::function<DraftingObjectId()> &mintId);

} // namespace edi::drafting
