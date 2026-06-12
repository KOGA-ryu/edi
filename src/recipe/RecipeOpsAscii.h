#pragma once

#include "recipe/RecipeOps.h"

#include <string>
#include <vector>

namespace edi::recipe {

// The ASCII dry-run backend, ported from v0's ascii_backend.py — the
// PROOF stage of "Recipe is truth. ASCII preview is proof. Blender script
// is execution." In v0's own words it "intentionally renders approximate
// symbols rather than high art": its job is catching bad scale, missing
// parts, broken symmetry, wrong order, and incorrect footprint before
// Blender is ever opened.
//
// THE GLYPHS ARE DATA. Every mark the renderer makes is a named role in
// this table — the user's custom glyph project replaces the vocabulary
// without touching a line of rendering code. Defaults are v0's exact
// characters (the committed previews are byte-goldens against them).
struct AsciiGlyphSet {
    std::string empty = " ";
    std::string boxFill = "░";
    std::string border = "█";
    std::string cylinderFill = "▒";
    std::string mouldingFill = "▓";
    std::string ringFill = "▓";
    std::string sphereFill = "▒";
    std::string capBottom = "▄";
    std::string capTop = "▀";
    std::string fluteMark = "░";
};

enum class AsciiProjection { Front, Side, Top };

struct AsciiRenderResult {
    bool ok = false;
    std::string text;
    std::string message;
};

// Renders one orthographic projection of a COMPILED op stream (v0
// convention: X/Z front, Y/Z side, X/Y top; auto-fit bounds + 2.0 pad).
// Port divergence: an AddProfileMoulding here is an error — v0's renderer
// silently SKIPPED uncompiled mouldings, which made the proof lie about
// missing parts; a proof tool must refuse what it cannot show. An
// unresolved AddRevolvedProfile (R1-B04) is refused on the same contract:
// a profile reference has no points until the resolve pass reads the
// drawing.
AsciiRenderResult renderOpsProjection(const std::vector<RecipeOp> &ops,
                                      AsciiProjection projection,
                                      int width = 96,
                                      int height = 72,
                                      const AsciiGlyphSet &glyphs = {});

} // namespace edi::recipe
