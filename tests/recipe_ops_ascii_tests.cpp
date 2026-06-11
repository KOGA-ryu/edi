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

    // ---- An empty stream still renders the titled frame (v0's default
    // bounds), so a blank recipe previews as blank, not as an error. ----
    {
        const AsciiRenderResult empty = renderOpsProjection({}, AsciiProjection::Top);
        assert(empty.ok);
        assert(empty.text.find("TOP PROJECTION") != std::string::npos);
    }

    return 0;
}
