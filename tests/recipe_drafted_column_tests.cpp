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

#include "EdiAssert.h"
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
    EDI_CHECK(in.is_open());
    return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

} // namespace

int main()
{
    const RecipeOpStream source = doricDraftedOpStream();

    // ---- The committed SOURCE document IS this fixture, byte for byte. ----
    {
        const OpStreamTextResult written = recipeOpsToToml(source);
        EDI_CHECK(written.ok);
        EDI_CHECK(written.text
               == slurp(EDI_SAMPLES_DIR "/doric_column/doric_column_drafted_ops.toml"));
    }

    // ---- The source is honest about being unresolved: profile references
    // present, so the gate refuses it for downstream tiers. ----
    EDI_CHECK(!recipeOpsResolved(source));
    EDI_CHECK(!compileRecipeOps(source.ops).ok);

    // ---- Resolve against the REAL drafted document + the default grid —
    // the drafting surface is the measurement authority, live. ----
    std::ifstream in(EDI_SAMPLES_DIR "/doric_column/doric_column_profiles.edidraw",
                     std::ios::binary);
    EDI_CHECK(in.is_open());
    const std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)),
                                          std::istreambuf_iterator<char>());
    const auto decoded = decodeDraftingDocument(bytes);
    EDI_CHECK(decoded.ok);
    const DraftingGridProjection grid = projectDraftingGrid(defaultDraftingGridSettings());

    const OpResolveResult resolved = resolveRecipeOps(source, *decoded.value, grid);
    EDI_CHECK(resolved.ok);
    EDI_CHECK(resolved.findings.empty());
    EDI_CHECK(recipeOpsResolved(resolved.stream));

    // ---- Pipeline A's numbers, by probe (full precision, near()): the
    // lathes lowered to mouldings carrying the DRAFTED radii and heights. ----
    {
        const auto *baseCove = std::get_if<AddMouldingOp>(&resolved.stream.ops[2]);
        EDI_CHECK(baseCove != nullptr);
        EDI_CHECK(baseCove->name == "base.cove");
        EDI_CHECK(baseCove->vertices == 64);
        EDI_CHECK(baseCove->profile.size() == 3);
        EDI_CHECK(baseCove->profile[0].term == "profile_00");
        EDI_CHECK(near(baseCove->profile[0].radius, 1.32));
        EDI_CHECK(near(baseCove->profile[0].z, 0.83999999999999941));
        EDI_CHECK(near(baseCove->profile[2].radius, 1.056));
        EDI_CHECK(near(baseCove->profile[2].z, 1.0079999999999996));

        const auto *shaft = std::get_if<AddMouldingOp>(&resolved.stream.ops[3]);
        EDI_CHECK(shaft != nullptr);
        EDI_CHECK(shaft->profile.size() == 5);
        EDI_CHECK(near(shaft->profile[0].radius, 1.056));
        EDI_CHECK(near(shaft->profile[0].z, 1.0079999999999996));
        EDI_CHECK(near(shaft->profile[2].radius, 0.96)); // the drafted entasis midpoint
        EDI_CHECK(near(shaft->profile[4].radius, 0.79200000000000004));
        EDI_CHECK(near(shaft->profile[4].z, 8.0399999999999991));

        const auto *flutes = std::get_if<CutFlutesOp>(&resolved.stream.ops[4]);
        EDI_CHECK(flutes != nullptr);
        EDI_CHECK(flutes->count == 20);
        EDI_CHECK(flutes->cutterRadius.has_value() && near(*flutes->cutterRadius, 0.16));
        EDI_CHECK(flutes->atRadius.has_value() && near(*flutes->atRadius, 1.056));
        EDI_CHECK(near(flutes->depth, 0.12));

        const auto *echinus = std::get_if<AddMouldingOp>(&resolved.stream.ops[5]);
        EDI_CHECK(echinus != nullptr);
        EDI_CHECK(echinus->profile.size() == 4);
        EDI_CHECK(near(echinus->profile[0].radius, 0.79200000000000004));
        EDI_CHECK(near(echinus->profile[3].radius, 1.32));
        EDI_CHECK(near(echinus->profile[3].z, 8.3999999999999986));
    }

    // ---- The committed RESOLVED document IS this resolution, byte for
    // byte — regenerate by re-running exactly this chain. ----
    {
        const OpStreamTextResult written = recipeOpsToToml(resolved.stream);
        EDI_CHECK(written.ok);
        EDI_CHECK(written.text
               == slurp(EDI_SAMPLES_DIR "/doric_column/doric_column_drafted_resolved.toml"));
    }

    // ---- The resolved column compiles and validates clean (warnings
    // allowed, errors not) — the full downstream is open to it. ----
    {
        const RecipeCompileResult compiled = compileRecipeOps(resolved.stream.ops);
        EDI_CHECK(compiled.ok);
        const OpValidationReport report = validateRecipeOps(compiled.ops);
        EDI_CHECK(report.ok);
    }

    return 0;
}
