// The ASCII proof backend. The headline assertion: the doric column's three
// projections are BYTE-IDENTICAL to the prototype's own generated previews
// (ascii_blender_dryrun_v0/out/doric_*_preview.txt, committed under
// samples/doric_column/previews). Same Bresenham cells, same banker's
// rounding, same glyphs, same rstrip — "port" means the pixels transfer.
#include "recipe/RecipeOpsAscii.h"

#include "recipe_doric_fixture.h"

#include <cassert>
#include <fstream>
#include <iterator>
#include <string>

using namespace edi::recipe;

namespace {

std::string slurp(const std::string &path)
{
    std::ifstream in(path, std::ios::binary);
    assert(in.is_open());
    return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

} // namespace

int main()
{
    const RecipeCompileResult compiled = compileRecipeOps(doricColumnOpStream().ops);
    assert(compiled.ok);

    // ---- The three byte-goldens: front (X/Z), side (Y/Z), top (X/Y). ----
    {
        const AsciiRenderResult front = renderOpsProjection(compiled.ops, AsciiProjection::Front);
        assert(front.ok);
        assert(front.text == slurp(EDI_SAMPLES_DIR "/doric_column/previews/doric_front_preview.txt"));

        const AsciiRenderResult side = renderOpsProjection(compiled.ops, AsciiProjection::Side);
        assert(side.ok);
        assert(side.text == slurp(EDI_SAMPLES_DIR "/doric_column/previews/doric_side_preview.txt"));

        const AsciiRenderResult top = renderOpsProjection(compiled.ops, AsciiProjection::Top);
        assert(top.ok);
        assert(top.text == slurp(EDI_SAMPLES_DIR "/doric_column/previews/doric_top_preview.txt"));
    }

    // ---- The glyph vocabulary is data: swapping the table changes the
    // marks without touching the renderer — the seam the user's custom
    // glyph project plugs into. ----
    {
        AsciiGlyphSet custom;
        custom.boxFill = "o";
        custom.border = "#";
        custom.cylinderFill = ".";
        custom.mouldingFill = "%";
        custom.capTop = "^";
        custom.capBottom = "_";
        custom.fluteMark = "'";
        const AsciiRenderResult render = renderOpsProjection(compiled.ops, AsciiProjection::Front, 96, 72, custom);
        assert(render.ok);
        assert(render.text.find("#") != std::string::npos);
        assert(render.text.find("o") != std::string::npos);
        assert(render.text.find("█") == std::string::npos); // defaults fully replaced
        assert(render.text.find("░") == std::string::npos);
        assert(render.text.find("FRONT PROJECTION") != std::string::npos); // title intact
    }

    // ---- Port divergence pinned: an UNCOMPILED stream is refused by name.
    // v0 silently skipped AddProfileMoulding here — a proof tool that omits
    // parts without saying so proves nothing. ----
    {
        const AsciiRenderResult refused = renderOpsProjection(doricColumnOpStream().ops, AsciiProjection::Front);
        assert(!refused.ok);
        assert(refused.message.find("base.torus_scotia_moulding") != std::string::npos);
        assert(refused.text.empty());
    }

    // ---- Same contract for the lathe reference (R1-B04): an UNRESOLVED
    // AddRevolvedProfile has no points to show, so the proof refuses it. ----
    {
        AddRevolvedProfileOp unresolved;
        unresolved.name = "shaft.turned";
        unresolved.profile = "shaft";
        const AsciiRenderResult refused =
            renderOpsProjection({RecipeOp{unresolved}}, AsciiProjection::Front);
        assert(!refused.ok);
        assert(refused.message == "AddRevolvedProfile must be resolved before preview: shaft.turned");
        assert(refused.text.empty());
    }

    // ---- An empty stream still renders the titled frame (v0's default
    // bounds), so a blank recipe previews as blank, not as an error. ----
    {
        const AsciiRenderResult empty = renderOpsProjection({}, AsciiProjection::Top);
        assert(empty.ok);
        assert(empty.text.find("TOP PROJECTION") != std::string::npos);
    }

    // ---- Top-projection subset golden. The doric top golden above is 100%
    // overdrawn by the entablature slab (it is a single box rect — zero ▒/▓
    // cells survive), so every circular top-view path is invisible to it.
    // This no-box subset — base moulding, shaft, flutes, in stream order so
    // the smaller shaft disc stays visible over the wider moulding annulus —
    // is the real guard for the cylinder/moulding top circles, the
    // max-radius profile scan, the flute spokes, and circle()'s border band. ----
    {
        const std::vector<RecipeOp> subset{compiled.ops[2], compiled.ops[3], compiled.ops[4]};
        const AsciiRenderResult top = renderOpsProjection(subset, AsciiProjection::Top, 48, 36);
        assert(top.ok);
        const std::string golden = R"GOLD(                        █
 TOP PROJECTION    █████▓█████
                ███▓▓▓▓▓▓▓▓▓▓▓███
               █▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓█
             ██▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓██
            ██▓▓▓▓▓▓▓▓▓▓█▓▓▓▓▓▓▓▓▓▓██
           █▓▓▓▓▓▓▓█████▒█████▓▓▓▓▓▓▓█
          ██▓▓▓▓▓▓██▒▒▒▒▒▒▒▒▒██▓▓▓▓▓▓██
          █▓▓▓▓▓██▒▒▒░▒▒░▒▒░▒▒▒██▓▓▓▓▓█
         █▓▓▓▓▓██▒▒▒▒░▒▒░▒▒░▒▒▒▒██▓▓▓▓▓█
        █▓▓▓▓▓██▒▒░▒▒▒░▒░▒░▒▒▒░▒▒██▓▓▓▓▓█
        █▓▓▓▓▓█▒▒▒▒░▒▒░▒░▒░▒▒░▒▒▒▒█▓▓▓▓▓█
        █▓▓▓▓█▒▒░▒▒▒░▒░▒░▒░▒░▒▒▒░▒▒█▓▓▓▓█
       █▓▓▓▓██▒▒▒░▒▒░▒▒░░░▒▒░▒▒░▒▒▒██▓▓▓▓█
       █▓▓▓▓█▒▒▒▒▒░░▒░▒░░░▒░▒░░▒▒▒▒▒█▓▓▓▓█
       █▓▓▓▓█▒░░▒▒▒▒░▒░▒▒▒░▒░▒▒▒▒░░▒█▓▓▓▓█
       █▓▓▓▓█▒▒▒░░░▒▒░▒▒▒▒▒░▒▒░░░▒▒▒█▓▓▓▓█
       █▓▓▓▓█▒▒▒▒▒▒░░▒▒▒▒▒▒▒░░▒▒▒▒▒▒█▓▓▓▓█
      █▓▓▓▓█▒▒░░░░░░░▒▒▒▒▒▒▒░░░░░░░▒▒█▓▓▓▓█
       █▓▓▓▓█▒▒▒▒▒▒░░▒▒▒▒▒▒▒░░▒▒▒▒▒▒█▓▓▓▓█
       █▓▓▓▓█▒▒▒░░░▒▒░▒▒▒▒▒░▒▒░░░▒▒▒█▓▓▓▓█
       █▓▓▓▓█▒░░▒▒▒▒░▒░▒▒▒░▒░▒▒▒▒░░▒█▓▓▓▓█
       █▓▓▓▓█▒▒▒▒▒░░▒░▒░░░▒░▒░░▒▒▒▒▒█▓▓▓▓█
       █▓▓▓▓██▒▒▒░▒▒░▒▒░░░▒▒░▒▒░▒▒▒██▓▓▓▓█
        █▓▓▓▓█▒▒░▒▒▒░▒░▒░▒░▒░▒▒▒░▒▒█▓▓▓▓█
        █▓▓▓▓▓█▒▒▒▒░▒▒░▒░▒░▒▒░▒▒▒▒█▓▓▓▓▓█
        █▓▓▓▓▓██▒▒░▒▒▒░▒░▒░▒▒▒░▒▒██▓▓▓▓▓█
         █▓▓▓▓▓██▒▒▒▒░▒▒░▒▒░▒▒▒▒██▓▓▓▓▓█
          █▓▓▓▓▓██▒▒▒░▒▒░▒▒░▒▒▒██▓▓▓▓▓█
          ██▓▓▓▓▓▓██▒▒▒▒▒▒▒▒▒██▓▓▓▓▓▓██
           █▓▓▓▓▓▓▓█████▒█████▓▓▓▓▓▓▓█
            ██▓▓▓▓▓▓▓▓▓▓█▓▓▓▓▓▓▓▓▓▓██
             ██▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓██
               █▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓█
                ███▓▓▓▓▓▓▓▓▓▓▓███
                   █████▓█████)GOLD";
        assert(top.text == golden);
    }

    // ---- A lone plain drum: both caps must be visible. The doric goldens
    // never show capTop — the necking moulding starts wider than the shaft's
    // top radius and overdraws its entire cap row — so this standalone
    // render is the only pin on the capTop draw (and on the cap-swap mutant
    // in both directions, since it asserts capBottom too). ----
    {
        AddCylinderOp plain;
        plain.name = "probe.plain_drum";
        plain.radius = 4.0;
        plain.height = 10.0;
        plain.z = 5.0;
        const AsciiRenderResult render =
            renderOpsProjection({RecipeOp{plain}}, AsciiProjection::Front, 24, 16);
        assert(render.ok);
        assert(render.text.find("▀") != std::string::npos); // capTop row visible
        assert(render.text.find("▄") != std::string::npos); // capBottom row visible
    }

    // ---- CutFlutes front-marker edge guards. count=1 pins the
    // max(1, count-1) divide guard BY POSITION: with the guard the lone
    // marker stands at column round(24 * 0.30) = 7; without it the 0/0
    // division goes NaN and the marker leaves its column (presence alone
    // cannot tell — a stray marker still contains the glyph). The doric's
    // count=20 reaches neither guard. ----
    {
        CutFlutesOp lone;
        lone.target = "probe.plain_drum";
        lone.count = 1;
        lone.depth = 0.1;
        const AsciiRenderResult single =
            renderOpsProjection({RecipeOp{lone}}, AsciiProjection::Front, 24, 16);
        assert(single.ok);
        // A marker-only row: exactly seven leading spaces, then the glyph.
        assert(single.text.find("\n       ░") != std::string::npos);

        // The 32 clamp makes count=40 and count=32 byte-identical — but
        // only at a width where their column sets differ unclamped. At the
        // default 96 the unclamped sweeps disagree (count=32 lands gapped
        // columns, count=40 a contiguous band); at 24 every count >= 20
        // paints the same 11 columns and the assert would be tautological.
        CutFlutesOp forty = lone;
        forty.count = 40;
        CutFlutesOp clamped = lone;
        clamped.count = 32;
        const AsciiRenderResult fortyRender =
            renderOpsProjection({RecipeOp{forty}}, AsciiProjection::Front);
        const AsciiRenderResult clampedRender =
            renderOpsProjection({RecipeOp{clamped}}, AsciiProjection::Front);
        assert(fortyRender.ok && clampedRender.ok);
        assert(fortyRender.text == clampedRender.text);
    }

    return 0;
}
