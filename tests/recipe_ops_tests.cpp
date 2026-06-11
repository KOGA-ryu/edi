// The op vocabulary: typed ops + strict TOML store + compile pass +
// validators, ported from the prototype. The doric recipe used as the
// round-trip body below is the prototype's own example
// (examples/doric_column_recipe_v0.json), translated key-for-key.
#include "recipe/RecipeOps.h"
#include "recipe/RecipeOpsStore.h"
#include "recipe/RecipeOpsValidate.h"

#include <cassert>
#include <cmath>
#include <fstream>
#include <iterator>
#include <string>

using namespace edi::recipe;

namespace {

bool near(double a, double b)
{
    return std::abs(a - b) < 1e-9;
}

// The prototype's doric column, op for op. Names, numbers, materials —
// the user's vocabulary verbatim.
RecipeOpStream doricColumnStream()
{
    RecipeOpStream stream;
    stream.id = "doric_column_v0";
    stream.name = "Doric Column";

    AddBoxOp lowerStep;
    lowerStep.name = "plinth.lower_step";
    lowerStep.width = 18;
    lowerStep.depth = 18;
    lowerStep.height = 2.0;
    lowerStep.z = 1.0;
    stream.ops.push_back(lowerStep);

    AddBoxOp upperStep;
    upperStep.name = "plinth.upper_step";
    upperStep.width = 15.5;
    upperStep.depth = 15.5;
    upperStep.height = 2.0;
    upperStep.z = 3.0;
    stream.ops.push_back(upperStep);

    AddProfileMouldingOp baseMoulding;
    baseMoulding.name = "base.torus_scotia_moulding";
    baseMoulding.baseZ = 4.0;
    baseMoulding.sequence = {
        {"fillet", 0.08, 5.55, 5.55, {}, {}},
        {"torus", 0.46, {}, 6.65, {}, {}},
        {"scotia", 0.3, {}, 6.05, {}, {}},
        {"fillet", 0.16, {}, 5.12, {}, {}},
    };
    stream.ops.push_back(baseMoulding);

    AddCylinderOp shaft;
    shaft.name = "shaft.tapered_fluted_core";
    shaft.radius = 5.0;
    shaft.height = 50.4;
    shaft.z = 30.2;
    shaft.vertices = 128;
    shaft.taperTopRadius = 4.3;
    shaft.entasis = true;
    stream.ops.push_back(shaft);

    CutFlutesOp flutes;
    flutes.target = "shaft.tapered_fluted_core";
    flutes.count = 20;
    flutes.depth = 0.45;
    flutes.widthRatio = 0.34;
    flutes.startZ = 6.5;
    flutes.endZ = 53.9;
    stream.ops.push_back(flutes);

    AddProfileMouldingOp necking;
    necking.name = "capital.necking_annuli";
    necking.baseZ = 55.4;
    necking.sequence = {
        {"fillet", 0.25, 4.4, 5.2, {}, {}},
        {"scotia", 0.27, {}, 4.9, {}, {}},
        {"annulet", 0.38, {}, 5.4, {}, {}},
        {"fillet", 0.3, {}, 5.0, {}, {}},
    };
    stream.ops.push_back(necking);

    AddProfileMouldingOp echinusCushion;
    echinusCushion.name = "capital.echinus_cushion";
    echinusCushion.baseZ = 56.6;
    echinusCushion.sequence = {
        {"cavetto", 0.7, 5.0, 6.0, {}, {}},
        {"echinus", 2.35, {}, 7.9, {}, {}},
        {"fillet", 0.95, {}, 6.9, {}, {}},
    };
    stream.ops.push_back(echinusCushion);

    AddBoxOp abacus;
    abacus.name = "capital.abacus_square_slab";
    abacus.width = 16.5;
    abacus.depth = 16.5;
    abacus.height = 3.0;
    abacus.z = 62.1;
    stream.ops.push_back(abacus);

    AddBoxOp entablature;
    entablature.name = "entablature.test_block";
    entablature.width = 22;
    entablature.depth = 18;
    entablature.height = 5;
    entablature.z = 66.1;
    entablature.material = "limestone";
    stream.ops.push_back(entablature);

    return stream;
}

bool sameSegment(const MouldingSegment &a, const MouldingSegment &b)
{
    return a.term == b.term && near(a.height, b.height)
        && a.startRadius == b.startRadius && a.endRadius == b.endRadius
        && a.radiusDelta == b.radiusDelta && a.steps == b.steps;
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
    const RecipeOpStream doric = doricColumnStream();

    // ---- The doric recipe is VALID by the ported checks, and clean under
    // the column-convention lint (it is the convention's own source).
    {
        const OpValidationReport report = validateRecipeOps(doric.ops);
        assert(report.ok);
        for (const OpFinding &finding : report.findings) {
            assert(finding.severity == OpFinding::Severity::Warning);
        }
        assert(lintColumnConventions(doric.ops).empty());
    }

    // ---- TOML round trip: every op, every field, exactly. ----
    {
        const OpStreamTextResult written = recipeOpsToToml(doric);
        assert(written.ok);
        // Pointable keys, the user's op vocabulary verbatim.
        assert(written.text.find("op.3.type = \"AddCylinder\"") != std::string::npos);
        assert(written.text.find("op.3.taper_top_radius = \"4.3\"") != std::string::npos);
        assert(written.text.find("op.4.target = \"shaft.tapered_fluted_core\"") != std::string::npos);
        assert(written.text.find("op.2.seq.1.term = \"torus\"") != std::string::npos);

        // The committed sample IS this construction — drift fails the suite.
        assert(written.text == slurp(EDI_SAMPLES_DIR "/doric_column/doric_column_ops.toml"));

        const OpStreamParseResult reloaded = recipeOpsFromToml(written.text, "round_trip");
        assert(reloaded.ok);
        assert(reloaded.stream.id == doric.id);
        assert(reloaded.stream.ops.size() == doric.ops.size());

        const auto *shaft = std::get_if<AddCylinderOp>(&reloaded.stream.ops[3]);
        assert(shaft != nullptr);
        assert(shaft->name == "shaft.tapered_fluted_core");
        assert(near(shaft->radius, 5.0));
        assert(shaft->taperTopRadius.has_value() && near(*shaft->taperTopRadius, 4.3));
        assert(shaft->entasis);
        assert(near(shaft->entasisRatio, 0.045)); // v0's constant, now data
        assert(shaft->vertices == 128);

        const auto *flutes = std::get_if<CutFlutesOp>(&reloaded.stream.ops[4]);
        assert(flutes != nullptr);
        assert(flutes->count == 20 && near(flutes->depth, 0.45) && near(flutes->widthRatio, 0.34));
        assert(flutes->startZ.has_value() && near(*flutes->startZ, 6.5));

        const auto *moulding = std::get_if<AddProfileMouldingOp>(&reloaded.stream.ops[2]);
        assert(moulding != nullptr);
        assert(moulding->sequence.size() == 4);
        const auto *original = std::get_if<AddProfileMouldingOp>(&doric.ops[2]);
        for (std::size_t i = 0; i < 4; ++i) {
            assert(sameSegment(moulding->sequence[i], original->sequence[i]));
        }

        const auto *entablature = std::get_if<AddBoxOp>(&reloaded.stream.ops[8]);
        assert(entablature != nullptr && entablature->material == "limestone");
    }

    // ---- Compile: profile mouldings lower to explicit points; the chain
    // reproduces the P1 goldens through the op layer. ----
    {
        const RecipeCompileResult compiled = compileRecipeOps(doric.ops);
        assert(compiled.ok);
        assert(compiled.ops.size() == doric.ops.size());
        for (const RecipeOp &op : compiled.ops) {
            assert(std::string(recipeOpTypeName(op)) != "AddProfileMoulding");
        }
        const auto *lowered = std::get_if<AddMouldingOp>(&compiled.ops[2]);
        assert(lowered != nullptr);
        assert(lowered->name == "base.torus_scotia_moulding");
        assert(near(lowered->baseZ, 4.0));
        assert(lowered->profile.size() == 11);
        assert(lowered->profile[2].term == "torus_01");
        assert(near(lowered->profile[2].radius, 5.7219)); // v0's expanded number
        // The compiled stream still validates (lowered mouldings are sane).
        assert(validateRecipeOps(compiled.ops).ok);

        // A failing sequence surfaces the term compiler's own message.
        RecipeOpStream broken = doric;
        auto *badMoulding = std::get_if<AddProfileMouldingOp>(&broken.ops[2]);
        badMoulding->sequence[0].startRadius.reset();
        const RecipeCompileResult refused = compileRecipeOps(broken.ops);
        assert(!refused.ok);
        assert(refused.message == "base.torus_scotia_moulding first segment needs start_radius.");
        assert(refused.ops.empty());
    }

    // ---- Validators: ported findings fire with v0 codes; port divergences
    // fire with their own. ----
    {
        std::vector<RecipeOp> ops;
        AddBoxOp flatBox;
        flatBox.name = "bad.box";
        flatBox.width = 0.0; // non-positive
        flatBox.depth = 1.0;
        flatBox.height = 1.0;
        ops.push_back(flatBox);
        AddBoxOp duplicate = flatBox;
        duplicate.width = 1.0;
        ops.push_back(duplicate); // duplicate name
        AddCylinderOp coarse;
        coarse.name = "coarse.cylinder";
        coarse.radius = 1.0;
        coarse.height = 1.0;
        coarse.vertices = 8;            // warning
        coarse.taperTopRadius = -1.0;   // error
        coarse.material = "plastic";    // port divergence: unknown material
        ops.push_back(coarse);
        CutFlutesOp orphan;
        orphan.target = "no.such.part"; // never declared
        orphan.count = 20;
        orphan.depth = 0.1;
        orphan.startZ = 5.0;
        orphan.endZ = 1.0;              // port addition: reversed range
        ops.push_back(orphan);
        AddRingOp ring;
        ring.name = "odd.ring";
        ring.radius = 1.0;
        ring.tubeHeight = 0.2;
        ring.overhang = 0.05;           // declared in v0, unwired
        ops.push_back(ring);

        const OpValidationReport report = validateRecipeOps(ops);
        assert(!report.ok);
        const auto has = [&report](const char *code) {
            for (const OpFinding &finding : report.findings) {
                if (finding.code == code) {
                    return true;
                }
            }
            return false;
        };
        assert(has("bad_box_dimensions"));
        assert(has("duplicate_name"));
        assert(has("low_cylinder_vertices"));
        assert(has("bad_taper_radius"));
        assert(has("unknown_material"));
        assert(has("flute_missing_target"));
        assert(has("bad_flute_z_range"));
        assert(has("ring_overhang_unwired"));
    }

    // Order matters for flute targets: cutting BEFORE the shaft exists is
    // the error; the same ops in build order are fine.
    {
        CutFlutesOp early;
        early.target = "shaft.core";
        early.count = 20;
        early.depth = 0.1;
        AddCylinderOp shaft;
        shaft.name = "shaft.core";
        shaft.radius = 1.0;
        shaft.height = 4.0;
        const OpValidationReport wrongOrder = validateRecipeOps({early, shaft});
        assert(!wrongOrder.ok);
        const OpValidationReport rightOrder = validateRecipeOps({RecipeOp(shaft), RecipeOp(early)});
        assert(rightOrder.ok);
    }

    // ---- Column lint stays quarantined: a non-column recipe is never
    // judged by column conventions; a column missing its shaft is. ----
    {
        AddBoxOp slab;
        slab.name = "table.top";
        slab.width = 4;
        slab.depth = 2;
        slab.height = 0.1;
        assert(lintColumnConventions({RecipeOp(slab)}).empty());

        AddBoxOp plinthOnly;
        plinthOnly.name = "plinth.lower_step";
        plinthOnly.width = 4;
        plinthOnly.depth = 4;
        plinthOnly.height = 1;
        const std::vector<OpFinding> lint = lintColumnConventions({RecipeOp(plinthOnly)});
        assert(lint.size() == 1);
        assert(lint[0].code == "missing_shaft");
    }

    // ---- Strict store negatives: every offender named. ----
    {
        const OpStreamParseResult unknownType = recipeOpsFromToml(
            "op.0.type = \"AddDodecahedron\"\n", "bad");
        assert(!unknownType.ok);
        assert(unknownType.message.find("AddDodecahedron") != std::string::npos);

        const OpStreamParseResult missingField = recipeOpsFromToml(
            "op.0.type = \"AddBox\"\n"
            "op.0.name = \"a.box\"\n", "bad");
        assert(!missingField.ok);
        assert(missingField.message.find("op.0.width") != std::string::npos);

        const OpStreamParseResult typoField = recipeOpsFromToml(
            "op.0.type = \"AddBox\"\n"
            "op.0.name = \"a.box\"\n"
            "op.0.width = \"1\"\n"
            "op.0.depth = \"1\"\n"
            "op.0.height = \"1\"\n"
            "op.0.z = \"0\"\n"
            "op.0.heigth = \"2\"\n", "bad"); // the classic typo
        assert(!typoField.ok);
        assert(typoField.message.find("heigth") != std::string::npos);

        const OpStreamParseResult badZMode = recipeOpsFromToml(
            "op.0.type = \"AddBox\"\n"
            "op.0.name = \"a.box\"\n"
            "op.0.width = \"1\"\n"
            "op.0.depth = \"1\"\n"
            "op.0.height = \"1\"\n"
            "op.0.z = \"0\"\n"
            "op.0.z_mode = \"top\"\n", "bad");
        assert(!badZMode.ok);
        assert(badZMode.message.find("center or base") != std::string::npos);

        const OpStreamParseResult bothRadius = recipeOpsFromToml(
            "op.0.type = \"AddProfileMoulding\"\n"
            "op.0.name = \"m\"\n"
            "op.0.base_z = \"0\"\n"
            "op.0.seq.0.term = \"torus\"\n"
            "op.0.seq.0.height = \"1\"\n"
            "op.0.seq.0.start_radius = \"2\"\n"
            "op.0.seq.0.end_radius = \"3\"\n"
            "op.0.seq.0.radius_delta = \"1\"\n", "bad");
        assert(!bothRadius.ok);
        assert(bothRadius.message.find("both end_radius and radius_delta") != std::string::npos);

        const OpStreamParseResult gapped = recipeOpsFromToml(
            "op.0.type = \"AddLabel\"\n"
            "op.0.name = \"l\"\n"
            "op.0.text = \"hi\"\n"
            "op.0.x = \"0\"\n"
            "op.0.y = \"0\"\n"
            "op.0.z = \"0\"\n"
            "op.2.type = \"AddLabel\"\n", "bad");
        assert(!gapped.ok);
        assert(gapped.message.find("op.2.type") != std::string::npos);

        const OpStreamParseResult empty = recipeOpsFromToml("", "empty");
        assert(empty.ok);
        assert(empty.stream.ops.empty());
    }

    return 0;
}
