// THE R1 ACCEPTANCE BENCHMARK (B06): the same column pipeline A built,
// from the same drafted profiles, through the ONE surviving pipeline —
// drafted .edidraw -> resolve (references + explicit cutter) -> lowered
// mouldings -> compile -> validate. The resolved numbers are pinned
// against a probe of pipeline A's resolveRecipe taken immediately before
// its retirement (same document, same default 12x12 grid): base_cove
// r 1.32 -> 1.056 over z 0.84 -> 1.008, shaft r 1.056 -> 0.792 over
// z 1.008 -> 8.04, echinus r 0.792 -> 1.32 over z 8.04 -> 8.4, flutes
// cutter 0.16 riding at 1.056, depth 0.12, z 1.2 -> 6, count 20.
//
// This test loads the REAL drafted document — not a fixture restatement —
// so the chain "the canvas is the measurement authority" stays executable:
// edit a profile vertex in the .edidraw and this test tells you exactly
// which committed numbers changed.
#include "recipe/RecipeOpsResolve.h"
#include "recipe/RecipeOpsStore.h"
#include "recipe/RecipeOpsValidate.h"

#include "recipe_drafted_fixture.h"

#include "drafting/DraftingGrid.h"
#include "drafting/DraftingSerialize.h"

#include <cassert>
#include <cmath>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

using namespace edi::recipe;
using namespace edi::drafting;

namespace {

bool near(double a, double b)
{
    return std::abs(a - b) < 1e-9;
}

std::string slurp(const std::string &path)
{
    std::ifstream in(path, std::ios::binary);
    assert(in.is_open());
    return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

} // namespace

int main()
{
    const RecipeOpStream source = doricDraftedOpStream();

    // ---- The committed SOURCE document IS this fixture, byte for byte. ----
    {
        const OpStreamTextResult written = recipeOpsToToml(source);
        assert(written.ok);
        assert(written.text
               == slurp(EDI_SAMPLES_DIR "/doric_column/doric_column_drafted_ops.toml"));
    }

    // ---- The source is honest about being unresolved: profile references
    // present, so the gate refuses it for downstream tiers. ----
    assert(!recipeOpsResolved(source));
    assert(!compileRecipeOps(source.ops).ok);

    // ---- Resolve against the REAL drafted document + the default grid —
    // the drafting surface is the measurement authority, live. ----
    std::ifstream in(EDI_SAMPLES_DIR "/doric_column/doric_column_profiles.edidraw",
                     std::ios::binary);
    assert(in.is_open());
    const std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)),
                                          std::istreambuf_iterator<char>());
    const auto decoded = decodeDraftingDocument(bytes);
    assert(decoded.ok);
    const DraftingGridProjection grid = projectDraftingGrid(defaultDraftingGridSettings());

    const OpResolveResult resolved = resolveRecipeOps(source, *decoded.value, grid);
    assert(resolved.ok);
    assert(resolved.findings.empty());
    assert(recipeOpsResolved(resolved.stream));

    // ---- Pipeline A's numbers, by probe (full precision, near()): the
    // lathes lowered to mouldings carrying the DRAFTED radii and heights. ----
    {
        const auto *baseCove = std::get_if<AddMouldingOp>(&resolved.stream.ops[2]);
        assert(baseCove != nullptr);
        assert(baseCove->name == "base.cove");
        assert(baseCove->vertices == 64);
        assert(baseCove->profile.size() == 3);
        assert(baseCove->profile[0].term == "profile_00");
        assert(near(baseCove->profile[0].radius, 1.32));
        assert(near(baseCove->profile[0].z, 0.83999999999999941));
        assert(near(baseCove->profile[2].radius, 1.056));
        assert(near(baseCove->profile[2].z, 1.0079999999999996));

        const auto *shaft = std::get_if<AddMouldingOp>(&resolved.stream.ops[3]);
        assert(shaft != nullptr);
        assert(shaft->profile.size() == 5);
        assert(near(shaft->profile[0].radius, 1.056));
        assert(near(shaft->profile[0].z, 1.0079999999999996));
        assert(near(shaft->profile[2].radius, 0.96)); // the drafted entasis midpoint
        assert(near(shaft->profile[4].radius, 0.79200000000000004));
        assert(near(shaft->profile[4].z, 8.0399999999999991));

        const auto *flutes = std::get_if<CutFlutesOp>(&resolved.stream.ops[4]);
        assert(flutes != nullptr);
        assert(flutes->count == 20);
        assert(flutes->cutterRadius.has_value() && near(*flutes->cutterRadius, 0.16));
        assert(flutes->atRadius.has_value() && near(*flutes->atRadius, 1.056));
        assert(near(flutes->depth, 0.12));

        const auto *echinus = std::get_if<AddMouldingOp>(&resolved.stream.ops[5]);
        assert(echinus != nullptr);
        assert(echinus->profile.size() == 4);
        assert(near(echinus->profile[0].radius, 0.79200000000000004));
        assert(near(echinus->profile[3].radius, 1.32));
        assert(near(echinus->profile[3].z, 8.3999999999999986));
    }

    // ---- The committed RESOLVED document IS this resolution, byte for
    // byte — regenerate by re-running exactly this chain. ----
    {
        const OpStreamTextResult written = recipeOpsToToml(resolved.stream);
        assert(written.ok);
        assert(written.text
               == slurp(EDI_SAMPLES_DIR "/doric_column/doric_column_drafted_resolved.toml"));
    }

    // ---- The resolved column compiles and validates clean (warnings
    // allowed, errors not) — the full downstream is open to it. ----
    {
        const RecipeCompileResult compiled = compileRecipeOps(resolved.stream.ops);
        assert(compiled.ok);
        const OpValidationReport report = validateRecipeOps(compiled.ops);
        assert(report.ok);
    }

    return 0;
}
