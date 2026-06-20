// The op vocabulary: typed ops + strict TOML store + compile pass +
// validators, ported from the prototype. The doric recipe used as the
// round-trip body below is the prototype's own example
// (examples/doric_column_recipe_v0.json), translated key-for-key.
#include "recipe/RecipeOps.h"
#include "recipe/RecipeOpSchema.h"
#include "recipe/RecipeOpsBind.h"
#include "recipe/RecipeOpsResolve.h"
#include "recipe/RecipeOpsStore.h"
#include "recipe/RecipeOpsValidate.h"

#include "recipe_doric_fixture.h"

#include "EdiAssert.h"
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <limits>
#include <random>
#include <set>
#include <string>

using namespace edi::recipe;

namespace {

bool near(double a, double b)
{
    return std::abs(a - b) < 1e-9;
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
    EDI_CHECK(in.is_open());
    return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

} // namespace

int main()
{
    const RecipeOpStream doric = doricColumnOpStream();

    // ---- The doric recipe is VALID by the ported checks, and clean under
    // the column-convention lint (it is the convention's own source).
    {
        const OpValidationReport report = validateRecipeOps(doric.ops);
        EDI_CHECK(report.ok);
        for (const OpFinding &finding : report.findings) {
            EDI_CHECK(finding.severity == OpFinding::Severity::Warning);
        }
        EDI_CHECK(lintColumnConventions(doric.ops).empty());
    }

    // ---- TOML round trip: every op, every field, exactly. ----
    {
        const OpStreamTextResult written = recipeOpsToToml(doric);
        EDI_CHECK(written.ok);
        // Pointable keys, the user's op vocabulary verbatim.
        EDI_CHECK(written.text.find("op.3.type = \"AddCylinder\"") != std::string::npos);
        EDI_CHECK(written.text.find("op.3.taper_top_radius = \"4.3\"") != std::string::npos);
        EDI_CHECK(written.text.find("op.4.target = \"shaft.tapered_fluted_core\"") != std::string::npos);
        EDI_CHECK(written.text.find("op.2.seq.1.term = \"torus\"") != std::string::npos);

        // The committed sample IS this construction — drift fails the suite.
        EDI_CHECK(written.text == slurp(EDI_SAMPLES_DIR "/doric_column/doric_column_ops.toml"));

        const OpStreamParseResult reloaded = recipeOpsFromToml(written.text, "round_trip");
        EDI_CHECK(reloaded.ok);
        EDI_CHECK(reloaded.stream.id == doric.id);
        EDI_CHECK(reloaded.stream.ops.size() == doric.ops.size());

        const auto *shaft = std::get_if<AddCylinderOp>(&reloaded.stream.ops[3]);
        EDI_CHECK(shaft != nullptr);
        EDI_CHECK(shaft->name == "shaft.tapered_fluted_core");
        EDI_CHECK(near(shaft->radius, 5.0));
        EDI_CHECK(shaft->taperTopRadius.has_value() && near(*shaft->taperTopRadius, 4.3));
        EDI_CHECK(shaft->entasis);
        EDI_CHECK(near(shaft->entasisRatio, 0.045)); // v0's constant, now data
        EDI_CHECK(shaft->vertices == 128);

        const auto *flutes = std::get_if<CutFlutesOp>(&reloaded.stream.ops[4]);
        EDI_CHECK(flutes != nullptr);
        EDI_CHECK(flutes->count == 20 && near(flutes->depth, 0.45) && near(flutes->widthRatio, 0.34));
        EDI_CHECK(flutes->startZ.has_value() && near(*flutes->startZ, 6.5));

        const auto *moulding = std::get_if<AddProfileMouldingOp>(&reloaded.stream.ops[2]);
        EDI_CHECK(moulding != nullptr);
        EDI_CHECK(moulding->sequence.size() == 4);
        const auto *original = std::get_if<AddProfileMouldingOp>(&doric.ops[2]);
        for (std::size_t i = 0; i < 4; ++i) {
            EDI_CHECK(sameSegment(moulding->sequence[i], original->sequence[i]));
        }

        const auto *entablature = std::get_if<AddBoxOp>(&reloaded.stream.ops[8]);
        EDI_CHECK(entablature != nullptr && entablature->material == "limestone");
    }

    // ---- Compile: profile mouldings lower to explicit points; the chain
    // reproduces the P1 goldens through the op layer. ----
    {
        const RecipeCompileResult compiled = compileRecipeOps(doric.ops);
        EDI_CHECK(compiled.ok);
        EDI_CHECK(compiled.ops.size() == doric.ops.size());
        for (const RecipeOp &op : compiled.ops) {
            EDI_CHECK(std::string(recipeOpTypeName(op)) != "AddProfileMoulding");
        }
        const auto *lowered = std::get_if<AddMouldingOp>(&compiled.ops[2]);
        EDI_CHECK(lowered != nullptr);
        EDI_CHECK(lowered->name == "base.torus_scotia_moulding");
        EDI_CHECK(near(lowered->baseZ, 4.0));
        EDI_CHECK(lowered->profile.size() == 11);
        EDI_CHECK(lowered->profile[2].term == "torus_01");
        EDI_CHECK(near(lowered->profile[2].radius, 5.7219)); // v0's expanded number
        // The compiled stream still validates (lowered mouldings are sane).
        EDI_CHECK(validateRecipeOps(compiled.ops).ok);

        // The committed compiled sample — the artifact the Blender driver
        // reads — IS this compile, serialized. Drift at the C++/python
        // seam (or in AddMouldingOp serialization, which appears in no
        // other byte-golden) fails here. This is also the executable
        // recipe for regenerating the file.
        RecipeOpStream compiledStream;
        compiledStream.id = doric.id;
        compiledStream.name = doric.name;
        compiledStream.ops = compiled.ops;
        const OpStreamTextResult compiledWritten = recipeOpsToToml(compiledStream);
        EDI_CHECK(compiledWritten.ok);
        EDI_CHECK(compiledWritten.text == slurp(EDI_SAMPLES_DIR "/doric_column/doric_column_ops_compiled.toml"));

        // A failing sequence surfaces the term compiler's own message.
        RecipeOpStream broken = doric;
        auto *badMoulding = std::get_if<AddProfileMouldingOp>(&broken.ops[2]);
        badMoulding->sequence[0].startRadius.reset();
        const RecipeCompileResult refused = compileRecipeOps(broken.ops);
        EDI_CHECK(!refused.ok);
        EDI_CHECK(refused.message == "base.torus_scotia_moulding first segment needs start_radius.");
        EDI_CHECK(refused.ops.empty());

        // An unresolved lathe reference cannot compile: compile has no
        // drawing to read, and guessing points is the forbidden move.
        AddRevolvedProfileOp unresolved;
        unresolved.name = "shaft.turned";
        unresolved.profile = "shaft";
        const RecipeCompileResult lathe = compileRecipeOps({RecipeOp{unresolved}});
        EDI_CHECK(!lathe.ok);
        EDI_CHECK(lathe.message == "AddRevolvedProfile must be resolved before compiling: shaft.turned");
        EDI_CHECK(lathe.ops.empty());
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
        ring.overhang = 0.05;           // alias semantics: widens, no torus
        ops.push_back(ring);

        const OpValidationReport report = validateRecipeOps(ops);
        EDI_CHECK(!report.ok);
        const auto has = [&report](const char *code) {
            for (const OpFinding &finding : report.findings) {
                if (finding.code == code) {
                    return true;
                }
            }
            return false;
        };
        EDI_CHECK(has("bad_box_dimensions"));
        EDI_CHECK(has("duplicate_name"));
        EDI_CHECK(has("low_cylinder_vertices"));
        EDI_CHECK(has("bad_taper_radius"));
        EDI_CHECK(has("unknown_material"));
        EDI_CHECK(has("flute_missing_target"));
        EDI_CHECK(has("bad_flute_z_range"));
        EDI_CHECK(has("ring_overhang_alias"));
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
        EDI_CHECK(!wrongOrder.ok);
        const OpValidationReport rightOrder = validateRecipeOps({RecipeOp(shaft), RecipeOp(early)});
        EDI_CHECK(rightOrder.ok);
    }

    // ---- Column lint stays quarantined: a non-column recipe is never
    // judged by column conventions; a column missing its shaft is. ----
    {
        AddBoxOp slab;
        slab.name = "table.top";
        slab.width = 4;
        slab.depth = 2;
        slab.height = 0.1;
        EDI_CHECK(lintColumnConventions({RecipeOp(slab)}).empty());

        AddBoxOp plinthOnly;
        plinthOnly.name = "plinth.lower_step";
        plinthOnly.width = 4;
        plinthOnly.depth = 4;
        plinthOnly.height = 1;
        const std::vector<OpFinding> lint = lintColumnConventions({RecipeOp(plinthOnly)});
        EDI_CHECK(lint.size() == 1);
        EDI_CHECK(lint[0].code == "missing_shaft");
    }

    // ---- Strict store negatives: every offender named. ----
    {
        const OpStreamParseResult unknownType = recipeOpsFromToml(
            "op.0.type = \"AddDodecahedron\"\n", "bad");
        EDI_CHECK(!unknownType.ok);
        EDI_CHECK(unknownType.message.find("AddDodecahedron") != std::string::npos);

        const OpStreamParseResult missingField = recipeOpsFromToml(
            "op.0.type = \"AddBox\"\n"
            "op.0.name = \"a.box\"\n", "bad");
        EDI_CHECK(!missingField.ok);
        EDI_CHECK(missingField.message.find("op.0.width") != std::string::npos);

        const OpStreamParseResult typoField = recipeOpsFromToml(
            "op.0.type = \"AddBox\"\n"
            "op.0.name = \"a.box\"\n"
            "op.0.width = \"1\"\n"
            "op.0.depth = \"1\"\n"
            "op.0.height = \"1\"\n"
            "op.0.z = \"0\"\n"
            "op.0.heigth = \"2\"\n", "bad"); // the classic typo
        EDI_CHECK(!typoField.ok);
        EDI_CHECK(typoField.message.find("heigth") != std::string::npos);

        const OpStreamParseResult badZMode = recipeOpsFromToml(
            "op.0.type = \"AddBox\"\n"
            "op.0.name = \"a.box\"\n"
            "op.0.width = \"1\"\n"
            "op.0.depth = \"1\"\n"
            "op.0.height = \"1\"\n"
            "op.0.z = \"0\"\n"
            "op.0.z_mode = \"top\"\n", "bad");
        EDI_CHECK(!badZMode.ok);
        EDI_CHECK(badZMode.message.find("center or base") != std::string::npos);

        const OpStreamParseResult bothRadius = recipeOpsFromToml(
            "op.0.type = \"AddProfileMoulding\"\n"
            "op.0.name = \"m\"\n"
            "op.0.base_z = \"0\"\n"
            "op.0.seq.0.term = \"torus\"\n"
            "op.0.seq.0.height = \"1\"\n"
            "op.0.seq.0.start_radius = \"2\"\n"
            "op.0.seq.0.end_radius = \"3\"\n"
            "op.0.seq.0.radius_delta = \"1\"\n", "bad");
        EDI_CHECK(!bothRadius.ok);
        EDI_CHECK(bothRadius.message.find("both end_radius and radius_delta") != std::string::npos);

        const OpStreamParseResult gapped = recipeOpsFromToml(
            "op.0.type = \"AddLabel\"\n"
            "op.0.name = \"l\"\n"
            "op.0.text = \"hi\"\n"
            "op.0.x = \"0\"\n"
            "op.0.y = \"0\"\n"
            "op.0.z = \"0\"\n"
            "op.2.type = \"AddLabel\"\n", "bad");
        EDI_CHECK(!gapped.ok);
        EDI_CHECK(gapped.message.find("op.2.type") != std::string::npos);

        const OpStreamParseResult empty = recipeOpsFromToml("", "empty");
        EDI_CHECK(empty.ok);
        EDI_CHECK(empty.stream.ops.empty());
    }

    // ---- Every remaining validator code fires at least once: a deleted
    // check must fail a test, not survive silently. ----
    {
        std::vector<RecipeOp> ops;
        AddCylinderOp shaft;
        shaft.name = "probe.shaft";
        shaft.radius = 1.0;
        shaft.height = 4.0;
        ops.push_back(shaft);
        AddSphereOp pebble;
        pebble.name = "probe.pebble";
        pebble.radius = 0.0;  // bad_sphere_radius
        pebble.vertices = 4;  // low_sphere_vertices
        ops.push_back(pebble);
        AddMouldingOp kinked;
        kinked.name = "probe.kinked";
        kinked.baseZ = -1.0;  // negative_moulding_base_z
        kinked.vertices = 8;  // low_moulding_vertices
        // Two descents (1.0 -> 0.5 -> 0.2): per-point reporting must name
        // both kinks; a first-only dedup would collapse them to one.
        kinked.profile = {{"a", 1.0, -0.5}, {"b", 0.5, 2.0}, {"c", 0.2, 2.0}};
        ops.push_back(kinked);
        AddMouldingOp stub;
        stub.name = "probe.stub";
        stub.profile = {{"only", -1.0, 1.0}}; // short profile + negative local z
        ops.push_back(stub);
        AddProfileMouldingOp hollow;
        hollow.name = "probe.hollow";
        hollow.baseZ = -2.0; // negative_profile_moulding_base_z
        hollow.vertices = 8; // low_profile_moulding_vertices
        ops.push_back(hollow); // empty sequence
        AddProfileMouldingOp mangled;
        mangled.name = "probe.mangled";
        MouldingSegment badSegment;
        badSegment.term = "ovolo";   // unknown_profile_term
        badSegment.height = 0.0;     // bad_profile_segment_height
        badSegment.endRadius = -1.0; // bad_profile_segment_radius (end half)
        badSegment.steps = 0;        // bad_profile_segment_steps
        mangled.sequence = {badSegment}; // and: missing_profile_start_radius
        MouldingSegment badStart;
        badStart.term = "torus";
        badStart.height = 1.0;
        badStart.startRadius = -2.0; // bad_profile_segment_radius (start half)
        mangled.sequence.push_back(badStart);
        ops.push_back(mangled);
        CutFlutesOp shallow;
        shallow.target = "probe.shaft";
        shallow.count = 8;        // low_flute_count
        shallow.depth = 0.0;      // bad_flute_depth
        shallow.widthRatio = 0.95; // odd_flute_width_ratio
        ops.push_back(shallow);
        AddCylinderOp beam;
        beam.name = "probe.beam";
        beam.radius = 1.0;
        beam.height = 6.0;
        beam.axis = Axis::X;
        ops.push_back(beam);
        CutFlutesOp sideways;
        sideways.target = "probe.beam"; // flute_target_not_vertical
        sideways.count = 20;
        sideways.depth = 0.1;
        ops.push_back(sideways);
        CutFlutesOp badCutter;        // R1-B04b explicit-cutter positives
        badCutter.target = "probe.shaft";
        badCutter.count = 20;
        badCutter.depth = 0.1;
        badCutter.cutterRadius = -0.5; // bad_cutter_radius
        badCutter.atRadius = -1.0;     // bad_at_radius
        ops.push_back(badCutter);
        CutFlutesOp explicitOk;       // valid explicit pair, odd ratio SUPPRESSED
        explicitOk.target = "probe.shaft";
        explicitOk.count = 20;
        explicitOk.depth = 0.1;
        explicitOk.cutterRadius = 0.16;
        explicitOk.atRadius = 1.0;
        explicitOk.widthRatio = 0.95; // would be odd, but the explicit pair makes it irrelevant
        ops.push_back(explicitOk);
        AddRevolvedProfileOp hollowLathe;
        hollowLathe.name = "probe.lathe";
        hollowLathe.baseZ = -0.5;       // negative_revolved_profile_base_z
        hollowLathe.vertices = 8;       // low_revolved_profile_vertices
        ops.push_back(hollowLathe);     // empty profile: missing_profile_reference

        const OpValidationReport report = validateRecipeOps(ops);
        EDI_CHECK(!report.ok);
        const auto count = [&report](const char *code) {
            int hits = 0;
            for (const OpFinding &finding : report.findings) {
                if (finding.code == code) {
                    ++hits;
                }
            }
            return hits;
        };
        EDI_CHECK(count("bad_sphere_radius") == 1);
        EDI_CHECK(count("low_sphere_vertices") == 1);
        EDI_CHECK(count("negative_moulding_base_z") == 1);
        EDI_CHECK(count("low_moulding_vertices") == 1);
        EDI_CHECK(count("bad_moulding_radius") == 1);
        EDI_CHECK(count("moulding_profile_not_monotonic") == 2); // one PER kink (v0 behavior)
        EDI_CHECK(count("short_moulding_profile") == 1);
        EDI_CHECK(count("bad_moulding_z") == 1);
        EDI_CHECK(count("negative_profile_moulding_base_z") == 1);
        EDI_CHECK(count("low_profile_moulding_vertices") == 1);
        EDI_CHECK(count("empty_profile_moulding_sequence") == 1);
        EDI_CHECK(count("unknown_profile_term") == 1);
        EDI_CHECK(count("bad_profile_segment_height") == 1);
        EDI_CHECK(count("missing_profile_start_radius") == 1);
        EDI_CHECK(count("bad_profile_segment_radius") == 2); // start AND end halves
        EDI_CHECK(count("bad_profile_segment_steps") == 1);
        EDI_CHECK(count("low_flute_count") == 1);
        EDI_CHECK(count("bad_flute_depth") == 1);
        // == 1, not 2: explicitOk's 0.95 ratio must NOT fire — the explicit
        // cutter pair makes the ratio lint irrelevant (R1-B04b decision 5).
        EDI_CHECK(count("odd_flute_width_ratio") == 1);
        EDI_CHECK(count("bad_cutter_radius") == 1);
        EDI_CHECK(count("bad_at_radius") == 1);
        EDI_CHECK(count("missing_profile_reference") == 1);
        EDI_CHECK(count("negative_revolved_profile_base_z") == 1);
        EDI_CHECK(count("low_revolved_profile_vertices") == 1);
        // numberKeyText formatting, not std::to_string's "0.950000".
        bool sawRatioMessage = false;
        for (const OpFinding &finding : report.findings) {
            if (finding.code == "odd_flute_width_ratio") {
                EDI_CHECK(finding.message.find("0.95") != std::string::npos);
                EDI_CHECK(finding.message.find("0.950000") == std::string::npos);
                sawRatioMessage = true;
            }
        }
        EDI_CHECK(sawRatioMessage);
        EDI_CHECK(count("flute_target_not_vertical") == 1);
    }

    // ---- The column lint's warnings, both directions, and max-vs-min. ----
    {
        AddCylinderOp shaft;
        shaft.name = "shaft.tapered_fluted_core";
        shaft.radius = 5.0;
        shaft.height = 40.0;

        AddBoxOp narrowPlinth;
        narrowPlinth.name = "plinth.lower_step";
        narrowPlinth.width = 9.0; // max(9,4) = 9 <= 10: too narrow
        narrowPlinth.depth = 4.0;
        narrowPlinth.height = 1.0;
        const std::vector<OpFinding> narrow = lintColumnConventions({RecipeOp(narrowPlinth), RecipeOp(shaft)});
        EDI_CHECK(narrow.size() == 2);
        EDI_CHECK(narrow[0].code == "base_not_wider_than_shaft");
        EDI_CHECK(narrow[1].code == "missing_capital");

        AddBoxOp wideRect = narrowPlinth;
        wideRect.width = 12.0; // max(12,4) = 12 > 10 — but min would be 4: kills max->min
        const std::vector<OpFinding> wide = lintColumnConventions({RecipeOp(wideRect), RecipeOp(shaft)});
        EDI_CHECK(wide.size() == 1);
        EDI_CHECK(wide[0].code == "missing_capital");

        AddBoxOp boundary = narrowPlinth;
        boundary.width = 10.0;
        boundary.depth = 10.0; // exactly shaft diameter: <= fires
        const std::vector<OpFinding> atBoundary = lintColumnConventions({RecipeOp(boundary), RecipeOp(shaft)});
        EDI_CHECK(atBoundary.size() == 2);
        EDI_CHECK(atBoundary[0].code == "base_not_wider_than_shaft");

        const std::vector<OpFinding> bare = lintColumnConventions({RecipeOp(shaft)});
        EDI_CHECK(bare.size() == 1); // no plinth box: only the capital warning
        EDI_CHECK(bare[0].code == "missing_capital");
    }

    // ---- Round trip for every writer the doric never exercises, with
    // every reloaded field asserted (zMode/axis/entasis/overhang included).
    {
        RecipeOpStream zoo;
        zoo.id = "writer_zoo";
        zoo.name = "Writer Zoo";
        AddSphereOp finial;
        finial.name = "probe.finial";
        finial.radius = 1.5;
        finial.z = 10.0;
        finial.x = 0.5;
        finial.y = -0.25;
        finial.vertices = 16;
        finial.material = "marble";
        zoo.ops.push_back(finial);
        AddRingOp collar;
        collar.name = "probe.collar";
        collar.radius = 2.0;
        collar.tubeHeight = 0.5;
        collar.z = 8.0;
        collar.overhang = 0.25;
        zoo.ops.push_back(collar);
        AddLabelOp tag;
        tag.name = "probe.tag";
        tag.text = "north face";
        tag.x = 1.0;
        tag.y = 2.0;
        tag.z = 3.0;
        zoo.ops.push_back(tag);
        AddMouldingOp band;
        band.name = "probe.band";
        band.baseZ = 4.0;
        band.profile = {{"fillet_start", 0.0, 2.0}, {"fillet_01", 0.5, 2.5}};
        zoo.ops.push_back(band);
        AddCylinderOp beam;
        beam.name = "probe.beam";
        beam.radius = 0.5;
        beam.height = 6.0;
        beam.z = 1.0;
        beam.x = 0.75;
        beam.y = -0.5;
        beam.axis = Axis::X;
        beam.zMode = ZMode::Base;
        beam.entasis = false;
        zoo.ops.push_back(beam);

        const OpStreamTextResult written = recipeOpsToToml(zoo);
        EDI_CHECK(written.ok);
        const OpStreamParseResult reloaded = recipeOpsFromToml(written.text, "zoo");
        EDI_CHECK(reloaded.ok);
        EDI_CHECK(reloaded.stream.ops.size() == 5);
        const auto *sphere = std::get_if<AddSphereOp>(&reloaded.stream.ops[0]);
        EDI_CHECK(sphere != nullptr && sphere->name == "probe.finial" && near(sphere->radius, 1.5)
               && near(sphere->x, 0.5) && near(sphere->y, -0.25)
               && sphere->vertices == 16 && sphere->material == "marble");
        const auto *ring = std::get_if<AddRingOp>(&reloaded.stream.ops[1]);
        EDI_CHECK(ring != nullptr && near(ring->tubeHeight, 0.5) && near(ring->overhang, 0.25));
        const auto *label = std::get_if<AddLabelOp>(&reloaded.stream.ops[2]);
        EDI_CHECK(label != nullptr && label->text == "north face"
               && near(label->x, 1.0) && near(label->y, 2.0) && near(label->z, 3.0));
        const auto *mouldingZoo = std::get_if<AddMouldingOp>(&reloaded.stream.ops[3]);
        EDI_CHECK(mouldingZoo != nullptr && mouldingZoo->profile.size() == 2
               && mouldingZoo->profile[1].term == "fillet_01"
               && near(mouldingZoo->profile[1].z, 0.5) && near(mouldingZoo->profile[1].radius, 2.5));
        const auto *beamBack = std::get_if<AddCylinderOp>(&reloaded.stream.ops[4]);
        EDI_CHECK(beamBack != nullptr && beamBack->axis == Axis::X && beamBack->zMode == ZMode::Base
               && !beamBack->entasis && near(beamBack->x, 0.75) && near(beamBack->y, -0.5));

        // TOML basic-string escaping survives the round trip: a quote, a
        // backslash, a newline, and DEL (0x7F — forbidden unescaped by
        // TOML just like the C0 controls) must come back verbatim.
        RecipeOpStream tricky;
        AddLabelOp gnarly;
        gnarly.name = "probe.gnarly";
        gnarly.text = "say \"hi\" to C:\\paths\nline two\x7F";
        gnarly.x = 0.0;
        gnarly.y = 0.0;
        gnarly.z = 0.0;
        tricky.ops.push_back(gnarly);
        const OpStreamTextResult trickyWritten = recipeOpsToToml(tricky);
        EDI_CHECK(trickyWritten.ok);
        // DEL must appear ESCAPED in the written text: our own reader would
        // happily round-trip a raw 0x7F, but tomllib (the python half of
        // the pipeline) refuses it — the escape is for the OTHER reader.
        EDI_CHECK(trickyWritten.text.find("\\u007F") != std::string::npos);
        const OpStreamParseResult trickyBack = recipeOpsFromToml(trickyWritten.text, "tricky");
        EDI_CHECK(trickyBack.ok);
        const auto *gnarlyBack = std::get_if<AddLabelOp>(&trickyBack.stream.ops[0]);
        EDI_CHECK(gnarlyBack != nullptr && gnarlyBack->text == gnarly.text);
    }

    // ---- A minimal handwritten op exercises every reader DEFAULT (the
    // writer always emits x/y/vertices/material, so only a hand file can
    // reach this path). And an oversized integer is refused by name.
    {
        // The revolved profile's reader defaults (vertices 96 — the
        // documented divergence from A's segments=64 — and material stone)
        // need a hand file too: every constructed test sets them.
        const OpStreamParseResult bareLathe = recipeOpsFromToml(
            "op.0.type = \"AddRevolvedProfile\"\n"
            "op.0.name = \"bare.lathe\"\n"
            "op.0.profile = \"shaft\"\n"
            "op.0.base_z = \"0\"\n", "minimal");
        EDI_CHECK(bareLathe.ok);
        const auto *bareTurned = std::get_if<AddRevolvedProfileOp>(&bareLathe.stream.ops[0]);
        EDI_CHECK(bareTurned != nullptr);
        EDI_CHECK(bareTurned->vertices == 96 && bareTurned->material == "stone"
               && bareTurned->x == 0.0 && bareTurned->y == 0.0);

        const OpStreamParseResult minimal = recipeOpsFromToml(
            "op.0.type = \"AddCylinder\"\n"
            "op.0.name = \"bare.drum\"\n"
            "op.0.radius = \"1\"\n"
            "op.0.height = \"2\"\n"
            "op.0.z = \"0\"\n", "minimal");
        EDI_CHECK(minimal.ok);
        const auto *drum = std::get_if<AddCylinderOp>(&minimal.stream.ops[0]);
        EDI_CHECK(drum != nullptr);
        EDI_CHECK(drum->x == 0.0 && drum->y == 0.0 && drum->vertices == 96
               && drum->material == "stone" && drum->axis == Axis::Z
               && drum->zMode == ZMode::Center && !drum->entasis
               && !drum->taperTopRadius.has_value());

        const OpStreamParseResult oversized = recipeOpsFromToml(
            "op.0.type = \"AddCylinder\"\n"
            "op.0.name = \"bare.drum\"\n"
            "op.0.radius = \"1\"\n"
            "op.0.height = \"2\"\n"
            "op.0.z = \"0\"\n"
            "op.0.vertices = \"99999999999\"\n", "oversized");
        EDI_CHECK(!oversized.ok);
        EDI_CHECK(oversized.message.find("op.0.vertices: not an integer") != std::string::npos);
    }

    // ---- Measurement bindings (R1-B02): pipeline A's .object/.field shape
    // grafted onto the op dialect's bare keys, bindings as a side table
    // (docs/recipe_binding_contract.md). The op dialect's literal is the
    // bare key — A's carried .value — so the has-both wording differs by
    // exactly that suffix.
    {
        // Round trip across kinds: one binding on EVERY op kind, so the
        // reader's acceptance call sites stay provably in step with the
        // registry (a reader gap here breaks the round trip by name).
        RecipeOpStream bound;
        bound.id = "bind.zoo";
        bound.name = "Binding Zoo";
        AddBoxOp plinth;
        plinth.name = "probe.plinth";
        plinth.width = 3.0;
        plinth.depth = 3.0;
        plinth.height = 0.5;
        plinth.z = 0.25;
        bound.ops.push_back(plinth);
        AddCylinderOp shaft;
        shaft.name = "probe.shaft";
        shaft.radius = 1.0;
        shaft.height = 6.0;
        shaft.z = 3.5;
        bound.ops.push_back(shaft);
        CutFlutesOp flutes;
        flutes.target = "probe.shaft";
        flutes.count = 20;
        flutes.depth = 0.1;
        bound.ops.push_back(flutes);
        AddSphereOp finial;
        finial.name = "probe.finial";
        finial.radius = 0.5;
        finial.z = 8.0;
        bound.ops.push_back(finial);
        AddRingOp collar;
        collar.name = "probe.collar";
        collar.radius = 1.2;
        collar.tubeHeight = 0.3;
        collar.z = 7.0;
        bound.ops.push_back(collar);
        AddMouldingOp band;
        band.name = "probe.band";
        band.profile = {{"fillet_start", 0.0, 1.0}, {"fillet_01", 0.2, 1.0}};
        bound.ops.push_back(band);
        AddProfileMouldingOp cove;
        cove.name = "probe.cove";
        MouldingSegment coveSegment;
        coveSegment.term = "cavetto";
        coveSegment.height = 0.3;
        coveSegment.startRadius = 1.0;
        cove.sequence = {coveSegment};
        bound.ops.push_back(cove);
        AddLabelOp tag;
        tag.name = "probe.tag";
        tag.text = "north";
        tag.x = 1.0;
        tag.y = 2.0;
        bound.ops.push_back(tag);
        AddRevolvedProfileOp turned;
        turned.name = "probe.turned";
        turned.profile = "shaft_profile";
        turned.vertices = 64;
        bound.ops.push_back(turned);
        bound.bindings = {
            {0, "width", "plinth_face", "width"},
            {1, "radius", "shaft_top", "radius"},
            {2, "width_ratio", "flute_gauge", "length"},
            {3, "z", "finial_seat", "height"},
            {4, "overhang", "collar_lip", "width"},
            {5, "base_z", "band_seat", "height"},
            {6, "base_z", "cove_seat", "height"},
            {7, "z", "tag_line", "height"},
            {8, "base_z", "lathe_seat", "height"},
        };

        const OpStreamTextResult boundWritten = recipeOpsToToml(bound);
        EDI_CHECK(boundWritten.ok);
        // The binding keys stand in for the literal; the bare key must NOT
        // appear beside them (the reader refuses that file as ambiguous).
        EDI_CHECK(boundWritten.text.find("op.0.width.object = \"plinth_face\"") != std::string::npos);
        EDI_CHECK(boundWritten.text.find("op.0.width.field = \"width\"") != std::string::npos);
        EDI_CHECK(boundWritten.text.find("op.0.width = ") == std::string::npos);
        EDI_CHECK(boundWritten.text.find("op.1.radius.object = \"shaft_top\"") != std::string::npos);
        EDI_CHECK(boundWritten.text.find("op.1.radius = ") == std::string::npos);
        EDI_CHECK(boundWritten.text.find("op.2.width_ratio.object = \"flute_gauge\"") != std::string::npos);
        EDI_CHECK(boundWritten.text.find("op.2.width_ratio = ") == std::string::npos);

        const OpStreamParseResult boundBack = recipeOpsFromToml(boundWritten.text, "bind.zoo");
        EDI_CHECK(boundBack.ok);
        EDI_CHECK(boundBack.stream.bindings.size() == bound.bindings.size());
        for (const RecipeFieldBinding &binding : bound.bindings) {
            bool found = false;
            for (const RecipeFieldBinding &loaded : boundBack.stream.bindings) {
                if (loaded.opIndex == binding.opIndex && loaded.fieldKey == binding.fieldKey
                    && loaded.objectId == binding.objectId && loaded.field == binding.field) {
                    found = true;
                    break;
                }
            }
            EDI_CHECK(found);
        }
        // A bound field carries the STRUCT DEFAULT until resolve (B03): the
        // unresolved number must be the inert default, never file garbage.
        const auto *plinthBack = std::get_if<AddBoxOp>(&boundBack.stream.ops[0]);
        EDI_CHECK(plinthBack != nullptr && plinthBack->width == 0.0);
        EDI_CHECK(plinthBack->depth == 3.0); // unbound literals load normally
        const auto *flutesBack = std::get_if<CutFlutesOp>(&boundBack.stream.ops[2]);
        EDI_CHECK(flutesBack != nullptr && flutesBack->widthRatio == 0.28); // spec default
        // The lathe reference round-trips: the profile id is a plain string
        // field (the OTHER crown jewel — never a binding).
        const auto *turnedBack = std::get_if<AddRevolvedProfileOp>(&boundBack.stream.ops[8]);
        EDI_CHECK(turnedBack != nullptr && turnedBack->profile == "shaft_profile"
               && turnedBack->vertices == 64 && turnedBack->baseZ == 0.0);

        // Refusals, A's loader order: half a binding, then literal clash,
        // then empty names.
        const OpStreamParseResult half = recipeOpsFromToml(
            "op.0.type = \"AddBox\"\n"
            "op.0.name = \"b\"\n"
            "op.0.width.object = \"plank\"\n", "bad");
        EDI_CHECK(!half.ok);
        EDI_CHECK(half.message == "op.0.width: a measurement binding needs both .object and .field");

        const OpStreamParseResult clash = recipeOpsFromToml(
            "op.0.type = \"AddBox\"\n"
            "op.0.name = \"b\"\n"
            "op.0.width = \"1\"\n"
            "op.0.width.object = \"plank\"\n"
            "op.0.width.field = \"width\"\n", "bad");
        EDI_CHECK(!clash.ok);
        EDI_CHECK(clash.message
               == "op.0.width: has both a literal and a measurement binding (.object/.field)");

        const OpStreamParseResult unnamed = recipeOpsFromToml(
            "op.0.type = \"AddBox\"\n"
            "op.0.name = \"b\"\n"
            "op.0.width.object = \"\"\n"
            "op.0.width.field = \"width\"\n", "bad");
        EDI_CHECK(!unnamed.ok);
        EDI_CHECK(unnamed.message == "op.0.width: a binding names an object and a field");

        // A binding on a non-bindable key falls to the consumption audit —
        // ints stay literal-only (divergence from A, see RecipeOpsBind.h).
        const OpStreamParseResult intBinding = recipeOpsFromToml(
            "op.0.type = \"AddCylinder\"\n"
            "op.0.name = \"c\"\n"
            "op.0.radius = \"1\"\n"
            "op.0.height = \"2\"\n"
            "op.0.z = \"0\"\n"
            "op.0.vertices.object = \"gauge\"\n"
            "op.0.vertices.field = \"width\"\n", "bad");
        EDI_CHECK(!intBinding.ok);
        EDI_CHECK(intBinding.message == "unknown recipe key: op.0.vertices.field");

        // The WRITER refuses bogus bindings too — a stream must not
        // serialize a file the reader would bounce. (A refusal message can
        // only be non-empty on the !ok path, so asserting the message IS
        // asserting the refusal.)
        RecipeOpStream bogus = bound;
        bogus.bindings = {{9, "width", "plank", "width"}};
        EDI_CHECK(recipeOpsToToml(bogus).message == "binding for op.9.width: no such op");
        bogus.bindings = {{1, "vertices", "gauge", "width"}};
        EDI_CHECK(recipeOpsToToml(bogus).message == "op.1.vertices: not a bindable field");
        bogus.bindings = {{0, "width", "", "width"}};
        EDI_CHECK(recipeOpsToToml(bogus).message == "op.0.width: a binding names an object and a field");
        bogus.bindings = {{0, "width", "a", "width"}, {0, "width", "b", "width"}};
        EDI_CHECK(recipeOpsToToml(bogus).message == "op.0.width: bound more than once");

        // The registry pinned EXHAUSTIVELY, row by row: deleting any row
        // (or typo'ing a key) fails here by name. The negative keys prove
        // ints, optionals, and strings stayed literal-only.
        struct RegistryPin {
            RecipeOp op;
            std::vector<const char *> bindable;
            std::vector<const char *> notBindable;
        };
        const RegistryPin registryPins[] = {
            {AddBoxOp{}, {"width", "depth", "height", "z", "x", "y"}, {"material", "z_mode"}},
            {AddCylinderOp{}, {"radius", "height", "z", "x", "y", "entasis_ratio"},
             {"vertices", "taper_top_radius", "entasis", "axis"}},
            {AddSphereOp{}, {"radius", "z", "x", "y"}, {"vertices", "height"}},
            {AddRingOp{}, {"radius", "tube_height", "z", "overhang", "x", "y"}, {"vertices"}},
            {AddMouldingOp{}, {"base_z", "x", "y"}, {"vertices", "profile"}},
            {AddProfileMouldingOp{}, {"base_z", "x", "y"}, {"vertices", "seq"}},
            {AddRevolvedProfileOp{}, {"base_z", "x", "y"}, {"vertices", "profile", "material"}},
            {CutFlutesOp{}, {"depth", "width_ratio"},
             {"count", "start_z", "end_z", "target", "cutter_radius", "at_radius"}},
            {AddLabelOp{}, {"x", "y", "z"}, {"text", "name", "width"}},
            // A craftsman's placement binds; its id, name, and (opaque) params
            // do not — "radius" is a param key here, never a bindable double.
            {ScriptOp{}, {"x", "y", "z"}, {"script", "name", "radius"}},
        };
        for (const RegistryPin &pin : registryPins) {
            for (const char *key : pin.bindable) {
                EDI_CHECK(opFieldBindable(pin.op, key));
                RecipeOp writable = pin.op;
                EDI_CHECK(setOpFieldValue(writable, key, 2.5));
            }
            for (const char *key : pin.notBindable) {
                EDI_CHECK(!opFieldBindable(pin.op, key));
                RecipeOp writable = pin.op;
                EDI_CHECK(!setOpFieldValue(writable, key, 2.5));
            }
        }
        // And the pointers land in the right members, not just somewhere.
        RecipeOp probe = AddRingOp{};
        EDI_CHECK(setOpFieldValue(probe, "tube_height", 2.5));
        EDI_CHECK(std::get_if<AddRingOp>(&probe)->tubeHeight == 2.5);
        EDI_CHECK(std::get_if<AddRingOp>(&probe)->radius == 0.0);

        // opFields lists the same fields the registry binds, with their live
        // values — what the human inspector reads to build its spinboxes. The
        // listed keys round-trip through opFieldBindable/setOpFieldValue, so the
        // read, the write, and the predicate can never drift apart.
        RecipeOp box = AddBoxOp{};
        std::get_if<AddBoxOp>(&box)->width = 3.0;
        std::get_if<AddBoxOp>(&box)->height = 5.0;
        const std::vector<RecipeOpField> boxFields = opFields(box);
        EDI_CHECK(boxFields.size() == 6); // width, depth, height, z, x, y
        EDI_CHECK(boxFields[0].key == "width" && boxFields[0].value == 3.0);
        EDI_CHECK(boxFields[2].key == "height" && boxFields[2].value == 5.0);
        for (const RecipeOpField &field : boxFields) {
            EDI_CHECK(opFieldBindable(box, field.key));
            EDI_CHECK(setOpFieldValue(box, field.key, 1.5));
        }
        EDI_CHECK(opFields(RecipeOp{AddSphereOp{}}).size() == 4); // radius, z, x, y
        EDI_CHECK(opFields(RecipeOp{AddCylinderOp{}})[5].key == "entasis_ratio");

        // The step palette: every offered type makes a valid, named, unit-sized
        // op the inspector can immediately tune; an off-palette type makes
        // nothing (mouldings/lathe/flutes need more than a click to be valid).
        const std::vector<std::string> &palette = recipePaletteOpTypes();
        EDI_CHECK(palette.size() == 4); // box, cylinder, sphere, ring
        for (const std::string &type : palette) {
            const std::optional<RecipeOp> made = makeRecipeOp(type, "step_test");
            EDI_CHECK(made.has_value());
            EDI_CHECK(recipeOpTypeName(*made) == type);
        }
        const std::optional<RecipeOp> newBox = makeRecipeOp("AddBox", "b0");
        EDI_CHECK(newBox.has_value());
        EDI_CHECK(std::get_if<AddBoxOp>(&*newBox)->width == 1.0);
        EDI_CHECK(std::get_if<AddBoxOp>(&*newBox)->name == "b0");
        EDI_CHECK(!makeRecipeOp("AddProfileMoulding", "x").has_value());

        // Remove/reorder keep the binding table (bindings are by op INDEX) sane.
        RecipeOpStream s;
        s.ops = {makeRecipeOp("AddBox", "a").value(),
                 makeRecipeOp("AddCylinder", "b").value(),
                 makeRecipeOp("AddSphere", "c").value()};
        s.bindings = {{0, "width", "obj0", "width"}, {2, "radius", "obj2", "radius"}};
        removeRecipeOp(s, 0); // op a gone; the sphere's binding slides 2 -> 1
        EDI_CHECK(s.ops.size() == 2);
        EDI_CHECK(recipeOpTypeName(s.ops[0]) == std::string("AddCylinder"));
        EDI_CHECK(s.bindings.size() == 1 && s.bindings[0].opIndex == 1);
        moveRecipeOp(s, 1, 0); // sphere to the front; its binding follows to 0
        EDI_CHECK(recipeOpTypeName(s.ops[0]) == std::string("AddSphere"));
        EDI_CHECK(s.bindings[0].opIndex == 0);
        removeRecipeOp(s, 99); // out of range: no-op
        EDI_CHECK(s.ops.size() == 2);

        // opEditableScalars covers what opFields cannot — enums, material, ints,
        // bools, strings — and setOpScalar writes each back by typed value.
        RecipeOp cyl = makeRecipeOp("AddCylinder", "drum").value();
        bool sawMaterial = false, sawZMode = false, sawName = false, sawVertices = false;
        for (const RecipeOpScalar &scalar : opEditableScalars(cyl)) {
            if (scalar.key == "material") {
                sawMaterial = scalar.kind == RecipeFieldKind::Choice && !scalar.choices.empty();
            } else if (scalar.key == "z_mode") {
                sawZMode = scalar.kind == RecipeFieldKind::Choice && scalar.choices.size() == 2;
            } else if (scalar.key == "name") {
                sawName = scalar.kind == RecipeFieldKind::Text && scalar.text == "drum";
            } else if (scalar.key == "vertices") {
                sawVertices = scalar.kind == RecipeFieldKind::Integer && scalar.integer == 96;
            }
        }
        EDI_CHECK(sawMaterial && sawZMode && sawName && sawVertices);
        EDI_CHECK(setOpScalar(cyl, "material", std::string("marble")));
        EDI_CHECK(std::get_if<AddCylinderOp>(&cyl)->material == "marble");
        EDI_CHECK(setOpScalar(cyl, "z_mode", std::string("base")));
        EDI_CHECK(std::get_if<AddCylinderOp>(&cyl)->zMode == ZMode::Base);
        EDI_CHECK(setOpScalar(cyl, "vertices", 48));
        EDI_CHECK(std::get_if<AddCylinderOp>(&cyl)->vertices == 48);
        EDI_CHECK(setOpScalar(cyl, "entasis", true));
        EDI_CHECK(std::get_if<AddCylinderOp>(&cyl)->entasis);
        EDI_CHECK(setOpScalar(cyl, "radius", 2.0)); // a double still routes through the registry
        EDI_CHECK(std::get_if<AddCylinderOp>(&cyl)->radius == 2.0);
        EDI_CHECK(!setOpScalar(cyl, "nonsense", 1.0)); // unknown key

        // Binding picker core: add / find / replace / clear a measurement binding.
        RecipeOpStream bstream;
        bstream.ops = {makeRecipeOp("AddBox", "b").value()};
        EDI_CHECK(addRecipeBinding(bstream, 0, "width", "plank_1", "length"));
        const RecipeFieldBinding *found = findRecipeBinding(bstream, 0, "width");
        EDI_CHECK(found != nullptr && found->objectId == "plank_1" && found->field == "length");
        EDI_CHECK(addRecipeBinding(bstream, 0, "width", "plank_2", "width")); // re-bind REPLACES
        EDI_CHECK(bstream.bindings.size() == 1);
        EDI_CHECK(findRecipeBinding(bstream, 0, "width")->objectId == "plank_2");
        EDI_CHECK(!addRecipeBinding(bstream, 0, "name", "x", "width")); // not a bindable double
        clearRecipeBinding(bstream, 0, "width");
        EDI_CHECK(bstream.bindings.empty() && findRecipeBinding(bstream, 0, "width") == nullptr);
    }

    // ---- Explicit cutter geometry (R1-B04b): the optional cutter_radius +
    // at_radius pair, XOR'd with the width_ratio derivation. Round trip emits
    // the pair and suppresses width_ratio; the reader refuses a half pair and
    // a pair beside a width_ratio, with the SAME wordings edi_craft.py pins
    // (the op.N: prefix matches python's op_key). ----
    {
        RecipeOpStream stream;
        stream.id = "cutter.zoo";
        AddCylinderOp shaft;
        shaft.name = "shaft";
        shaft.radius = 1.0;
        shaft.height = 6.0;
        stream.ops.push_back(shaft);
        CutFlutesOp flutes;
        flutes.target = "shaft";
        flutes.count = 20;
        flutes.depth = 0.12;
        flutes.cutterRadius = 0.16;   // the benchmark doric flutes' cutter
        flutes.atRadius = 1.056;
        stream.ops.push_back(flutes);

        const OpStreamTextResult written = recipeOpsToToml(stream);
        EDI_CHECK(written.ok);
        // The pair is emitted; width_ratio is NOT (a file showing both is the
        // lie the reader refuses).
        EDI_CHECK(written.text.find("op.1.cutter_radius = \"0.16\"") != std::string::npos);
        EDI_CHECK(written.text.find("op.1.at_radius = \"1.056\"") != std::string::npos);
        EDI_CHECK(written.text.find("op.1.width_ratio") == std::string::npos);

        const OpStreamParseResult back = recipeOpsFromToml(written.text, "cutter.zoo");
        EDI_CHECK(back.ok);
        const auto *flutesBack = std::get_if<CutFlutesOp>(&back.stream.ops[1]);
        EDI_CHECK(flutesBack != nullptr);
        EDI_CHECK(flutesBack->cutterRadius.has_value() && near(*flutesBack->cutterRadius, 0.16));
        EDI_CHECK(flutesBack->atRadius.has_value() && near(*flutesBack->atRadius, 1.056));
        EDI_CHECK(flutesBack->widthRatio == 0.28); // the inert default; the file carried none

        // Half a pair refuses, by name.
        const OpStreamParseResult halfPair = recipeOpsFromToml(
            "op.0.type = \"CutFlutes\"\n"
            "op.0.target = \"shaft\"\n"
            "op.0.count = \"20\"\n"
            "op.0.depth = \"0.12\"\n"
            "op.0.cutter_radius = \"0.16\"\n", "bad");
        EDI_CHECK(!halfPair.ok);
        EDI_CHECK(halfPair.message == "op.0: a cutter needs both .cutter_radius and .at_radius");

        // The pair beside a width_ratio refuses, by name (both sources).
        const OpStreamParseResult pairAndRatio = recipeOpsFromToml(
            "op.0.type = \"CutFlutes\"\n"
            "op.0.target = \"shaft\"\n"
            "op.0.count = \"20\"\n"
            "op.0.depth = \"0.12\"\n"
            "op.0.cutter_radius = \"0.16\"\n"
            "op.0.at_radius = \"1.056\"\n"
            "op.0.width_ratio = \"0.34\"\n", "bad");
        EDI_CHECK(!pairAndRatio.ok);
        EDI_CHECK(pairAndRatio.message
               == "op.0: has both an explicit cutter (.cutter_radius/.at_radius) and a .width_ratio");

        // The WRITER refuses the half pair too (planner ruling on the B04b
        // builder's flag #4 — the B02 rule: never serialize a file the
        // reader bounces; without this, a half pair in memory would emit
        // width_ratio and silently DROP the lone cutter value).
        RecipeOpStream halfPairStream;
        CutFlutesOp halfCutter;
        halfCutter.target = "shaft";
        halfCutter.count = 20;
        halfCutter.depth = 0.12;
        halfCutter.cutterRadius = 0.16; // atRadius deliberately unset
        halfPairStream.ops.push_back(halfCutter);
        const OpStreamTextResult halfWritten = recipeOpsToToml(halfPairStream);
        EDI_CHECK(!halfWritten.ok);
        EDI_CHECK(halfWritten.message == "op.0: a cutter needs both .cutter_radius and .at_radius");
    }

    // ---- Custom craftsmen: the Script op. The C++ side cannot see the
    // craftsman's MANIFEST (it lives in the Python script), so it carries a
    // free, untyped param bag and round-trips the EXACT shape edi_craft.py's
    // parse_ops reads — type/script/name/x/y/z + flat params.
    {
        // Round-trip a fully-specified craftsman step, and pin the on-disk keys
        // edi_craft consumes/sweeps (a writer drift that broke the python half
        // would fail HERE, by name).
        RecipeOpStream zoo;
        zoo.id = "craft_zoo";
        zoo.name = "Craft Zoo";
        ScriptOp tw;
        tw.scriptId = "twisted_column";
        tw.name = "twist.core";
        tw.x = 0.5;
        tw.y = -0.25;
        tw.z = 4.0;
        tw.params = {{"radius", "1.25"}, {"sides", "6"}, {"material", "marble"}};
        zoo.ops.push_back(tw);

        const OpStreamTextResult w = recipeOpsToToml(zoo);
        EDI_CHECK(w.ok);
        EDI_CHECK(w.text.find("op.0.type = \"Script\"") != std::string::npos);
        EDI_CHECK(w.text.find("op.0.script = \"twisted_column\"") != std::string::npos);
        EDI_CHECK(w.text.find("op.0.name = \"twist.core\"") != std::string::npos);
        EDI_CHECK(w.text.find("op.0.radius = \"1.25\"") != std::string::npos);
        EDI_CHECK(w.text.find("op.0.sides = \"6\"") != std::string::npos);
        EDI_CHECK(w.text.find("op.0.material = \"marble\"") != std::string::npos);

        const OpStreamParseResult rp = recipeOpsFromToml(w.text, "craft.zoo");
        EDI_CHECK(rp.ok && rp.stream.ops.size() == 1);
        const auto *back = std::get_if<ScriptOp>(&rp.stream.ops[0]);
        EDI_CHECK(back != nullptr && back->scriptId == "twisted_column" && back->name == "twist.core");
        EDI_CHECK(near(back->x, 0.5) && near(back->y, -0.25) && near(back->z, 4.0));
        EDI_CHECK(back->params.size() == 3);
        const auto pval = [&](const char *key) -> std::string {
            for (const ScriptParam &p : back->params) {
                if (p.key == key) return p.value;
            }
            return "<none>";
        };
        EDI_CHECK(pval("radius") == "1.25" && pval("sides") == "6" && pval("material") == "marble");
        // The canonical writer is idempotent (the lab re-serializes on every edit).
        EDI_CHECK(recipeOpsToToml(rp.stream).text == w.text);

        // Minimal hand file: only type + script. Name falls back to the id (the
        // python default), x/y/z default to 0, the bag is empty.
        const OpStreamParseResult mini = recipeOpsFromToml(
            "op.0.type = \"Script\"\n"
            "op.0.script = \"twisted_column\"\n", "mini");
        EDI_CHECK(mini.ok);
        const auto *m = std::get_if<ScriptOp>(&mini.stream.ops[0]);
        EDI_CHECK(m != nullptr && m->scriptId == "twisted_column" && m->name == "twisted_column");
        EDI_CHECK(m->x == 0.0 && m->y == 0.0 && m->z == 0.0 && m->params.empty());

        // A missing craftsman id is refused at READ, by name (requireText).
        const OpStreamParseResult noScript = recipeOpsFromToml(
            "op.0.type = \"Script\"\n"
            "op.0.name = \"x\"\n", "bad");
        EDI_CHECK(!noScript.ok);
        EDI_CHECK(noScript.message == "missing required key: op.0.script");

        // The bag accepts ANY key — there is no schema to audit it against here
        // (the craftsman's MANIFEST owns that), so the global unknown-key audit
        // does not fire on a Script op's params.
        const OpStreamParseResult wild = recipeOpsFromToml(
            "op.0.type = \"Script\"\n"
            "op.0.script = \"twisted_column\"\n"
            "op.0.name = \"w\"\n"
            "op.0.whatever = \"42\"\n"
            "op.0.another_param = \"hi\"\n", "wild");
        EDI_CHECK(wild.ok);
        const auto *wl = std::get_if<ScriptOp>(&wild.stream.ops[0]);
        EDI_CHECK(wl != nullptr && wl->params.size() == 2);

        // The param-key contract is ENFORCED at write, read, and validate (not
        // just asserted in a comment): a key that collides with a built-in
        // would overwrite it in the flat map; a dotted/spaced key would emit an
        // unreadable line or nest under the python half's tomllib. All three
        // gates refuse the same keys, by name.
        {
            // recipeScriptParamKeyProblem: the shared predicate.
            EDI_CHECK(recipeScriptParamKeyProblem("sides").empty());
            EDI_CHECK(recipeScriptParamKeyProblem("my-param_2").empty());
            EDI_CHECK(!recipeScriptParamKeyProblem("name").empty());  // built-in
            EDI_CHECK(!recipeScriptParamKeyProblem("a.b").empty());   // nests under tomllib
            EDI_CHECK(!recipeScriptParamKeyProblem("has space").empty());
            EDI_CHECK(!recipeScriptParamKeyProblem("").empty());

            // WRITE (B02): a colliding key refuses by name.
            RecipeOpStream collide;
            ScriptOp bad;
            bad.scriptId = "c";
            bad.name = "bad";
            bad.params = {{"name", "oops"}};
            collide.ops.push_back(bad);
            const OpStreamTextResult cw = recipeOpsToToml(collide);
            EDI_CHECK(!cw.ok);
            EDI_CHECK(cw.message == "op.0: param key 'name' collides with the built-in field 'name'");

            // WRITE: a dotted key refuses by name.
            RecipeOpStream dotted;
            ScriptOp dot;
            dot.scriptId = "c";
            dot.name = "d";
            dot.params = {{"a.b", "v"}};
            dotted.ops.push_back(dot);
            const OpStreamTextResult dw = recipeOpsToToml(dotted);
            EDI_CHECK(!dw.ok);
            EDI_CHECK(dw.message == "op.0: param key 'a.b' must be letters, digits, '_' or '-'");

            // READ: the strict reader refuses a dotted param key (it would round
            // -trip apart under tomllib). Reserved keys can't reach here — the
            // position readers consume them first — so the dotted case is the
            // reachable one.
            const OpStreamParseResult dottedRead = recipeOpsFromToml(
                "op.0.type = \"Script\"\n"
                "op.0.script = \"c\"\n"
                "op.0.a.b = \"v\"\n", "dotted");
            EDI_CHECK(!dottedRead.ok);
            EDI_CHECK(dottedRead.message == "op.0: param key 'a.b' must be letters, digits, '_' or '-'");

            // VALIDATE: a bad key surfaces as a named finding.
            ScriptOp badVal;
            badVal.scriptId = "c";
            badVal.name = "v";
            badVal.params = {{"bad key", "1"}};
            const OpValidationReport vr = validateRecipeOps({RecipeOp{badVal}});
            EDI_CHECK(!vr.ok);
            bool sawBadKey = false;
            for (const OpFinding &f : vr.findings) {
                sawBadKey = sawBadKey || f.code == "bad_param_key";
            }
            EDI_CHECK(sawBadKey);

            // A hyphenated key is legal and round-trips.
            RecipeOpStream okStream;
            ScriptOp okScript;
            okScript.scriptId = "c";
            okScript.name = "ok";
            okScript.params = {{"my-param", "7"}};
            okStream.ops.push_back(okScript);
            const OpStreamTextResult okw = recipeOpsToToml(okStream);
            EDI_CHECK(okw.ok);
            const OpStreamParseResult okr = recipeOpsFromToml(okw.text, "ok");
            EDI_CHECK(okr.ok);
            const auto *okBack = std::get_if<ScriptOp>(&okr.stream.ops[0]);
            EDI_CHECK(okBack != nullptr && okBack->params.size() == 1
                   && okBack->params[0].key == "my-param" && okBack->params[0].value == "7");
        }

        // Placement BINDS like every other op: write omits the literal x and
        // emits .object/.field; read restores the binding into the table.
        RecipeOpStream bound;
        ScriptOp anchored;
        anchored.scriptId = "twisted_column";
        anchored.name = "twist";
        anchored.params = {{"sides", "6"}};
        bound.ops.push_back(anchored);
        bound.bindings = {{0, "x", "anchor", "width"}};
        const OpStreamTextResult bw = recipeOpsToToml(bound);
        EDI_CHECK(bw.ok);
        EDI_CHECK(bw.text.find("op.0.x.object = \"anchor\"") != std::string::npos);
        EDI_CHECK(bw.text.find("op.0.x.field = \"width\"") != std::string::npos);
        EDI_CHECK(bw.text.find("op.0.x = ") == std::string::npos); // no literal beside the binding
        const OpStreamParseResult br = recipeOpsFromToml(bw.text, "bound.script");
        EDI_CHECK(br.ok && br.stream.bindings.size() == 1);
        EDI_CHECK(br.stream.bindings[0].opIndex == 0 && br.stream.bindings[0].fieldKey == "x"
               && br.stream.bindings[0].objectId == "anchor" && br.stream.bindings[0].field == "width");
        const auto *brBack = std::get_if<ScriptOp>(&br.stream.ops[0]);
        EDI_CHECK(brBack != nullptr && brBack->params.size() == 1
               && brBack->params[0].key == "sides" && brBack->params[0].value == "6");

        // compile leaves a Script op untouched (no lowering needed) — it reaches
        // the craftsmen library as-is.
        const RecipeCompileResult comp = compileRecipeOps(zoo.ops);
        EDI_CHECK(comp.ok && comp.ops.size() == 1
               && recipeOpTypeName(comp.ops[0]) == std::string("Script"));

        // Validate: the one C++-side invariant is a craftsman to dispatch to.
        ScriptOp ghost;
        ghost.name = "ghost"; // scriptId left empty
        const OpValidationReport noId = validateRecipeOps({RecipeOp{ghost}});
        EDI_CHECK(!noId.ok);
        bool sawMissing = false;
        for (const OpFinding &f : noId.findings) {
            sawMissing = sawMissing || f.code == "missing_script_reference";
        }
        EDI_CHECK(sawMissing);
        const OpValidationReport good = validateRecipeOps({RecipeOp{tw}});
        EDI_CHECK(good.ok);
        // A Script name participates in duplicate detection (opName sees it).
        AddBoxOp clash;
        clash.name = "twist.core";
        clash.width = clash.depth = clash.height = 1.0;
        const OpValidationReport dup = validateRecipeOps({RecipeOp{tw}, RecipeOp{clash}});
        bool sawDup = false;
        for (const OpFinding &f : dup.findings) {
            sawDup = sawDup || f.code == "duplicate_name";
        }
        EDI_CHECK(sawDup);

        // The inspector schema: x/y/z are Numbers (the registry), name is
        // editable text, the craftsman id is READ-ONLY, each param is editable
        // text; setOpScalar writes each back by typed value.
        RecipeOp scOp = RecipeOp{tw};
        bool sawX = false, sawName = false, sawScriptRO = false, sawSides = false;
        for (const RecipeOpScalar &s : opEditableScalars(scOp)) {
            if (s.key == "x") sawX = s.kind == RecipeFieldKind::Number && near(s.number, 0.5);
            else if (s.key == "name") sawName = s.kind == RecipeFieldKind::Text && s.text == "twist.core" && s.editable;
            else if (s.key == "script") sawScriptRO = !s.editable && s.text == "twisted_column";
            else if (s.key == "sides") sawSides = s.kind == RecipeFieldKind::Text && s.text == "6";
        }
        EDI_CHECK(sawX && sawName && sawScriptRO && sawSides);
        EDI_CHECK(setOpScalar(scOp, "name", std::string("renamed")));
        EDI_CHECK(std::get_if<ScriptOp>(&scOp)->name == "renamed");
        EDI_CHECK(setOpScalar(scOp, "sides", std::string("8"))); // a param
        EDI_CHECK(std::get_if<ScriptOp>(&scOp)->params[1].value == "8"); // {radius, sides, material}
        EDI_CHECK(setOpScalar(scOp, "x", 5.0)); // a double still routes through the registry
        EDI_CHECK(near(std::get_if<ScriptOp>(&scOp)->x, 5.0));
        EDI_CHECK(!setOpScalar(scOp, "script", std::string("other"))); // read-only id
        EDI_CHECK(std::get_if<ScriptOp>(&scOp)->scriptId == "twisted_column");
        EDI_CHECK(!setOpScalar(scOp, "nope", std::string("x"))); // unknown key
    }

    // ---- AddExtrudedProfile (BL-01): a profile-reference op like the lathe —
    // authored, refused-before-build, lowered later (BL-03). Round trip plus
    // refusal, the two proofs the brief asks for. ----
    {
        // Round trip: every field survives writer -> reader, key-for-key, and
        // the canonical writer is idempotent. base_z is bindable (true) so the
        // hand-omitted x/y exercise the reader defaults like the lathe.
        RecipeOpStream zoo;
        zoo.id = "extrude.zoo";
        zoo.name = "Extrude Zoo";
        AddExtrudedProfileOp prism;
        prism.name = "wall.panel";
        prism.profile = "panel_outline"; // a drafted object id, the reference
        prism.height = 2.5;
        prism.baseZ = 1.0;
        prism.x = 0.5;
        prism.y = -0.25;
        prism.material = "marble";
        zoo.ops.push_back(prism);

        const OpStreamTextResult written = recipeOpsToToml(zoo);
        EDI_CHECK(written.ok);
        // Writer key shape — a drift that broke BL-04's parse_ops would fail here.
        EDI_CHECK(written.text.find("op.0.type = \"AddExtrudedProfile\"") != std::string::npos);
        EDI_CHECK(written.text.find("op.0.profile = \"panel_outline\"") != std::string::npos);
        EDI_CHECK(written.text.find("op.0.height = \"2.5\"") != std::string::npos);
        EDI_CHECK(written.text.find("op.0.base_z = \"1\"") != std::string::npos);
        EDI_CHECK(written.text.find("op.0.material = \"marble\"") != std::string::npos);

        const OpStreamParseResult back = recipeOpsFromToml(written.text, "extrude.zoo");
        EDI_CHECK(back.ok && back.stream.ops.size() == 1);
        const auto *prismBack = std::get_if<AddExtrudedProfileOp>(&back.stream.ops[0]);
        EDI_CHECK(prismBack != nullptr && prismBack->name == "wall.panel"
               && prismBack->profile == "panel_outline" && near(prismBack->height, 2.5)
               && near(prismBack->baseZ, 1.0) && near(prismBack->x, 0.5)
               && near(prismBack->y, -0.25) && prismBack->material == "marble");
        EDI_CHECK(recipeOpsToToml(back.stream).text == written.text); // idempotent

        // Reader defaults from a minimal hand file: x/y -> 0, material -> stone.
        const OpStreamParseResult bare = recipeOpsFromToml(
            "op.0.type = \"AddExtrudedProfile\"\n"
            "op.0.name = \"bare.prism\"\n"
            "op.0.profile = \"outline\"\n"
            "op.0.height = \"3\"\n"
            "op.0.base_z = \"0\"\n", "minimal");
        EDI_CHECK(bare.ok);
        const auto *bareBack = std::get_if<AddExtrudedProfileOp>(&bare.stream.ops[0]);
        EDI_CHECK(bareBack != nullptr && bareBack->x == 0.0 && bareBack->y == 0.0
               && bareBack->material == "stone");

        // Refusal #1: an unlowered extrude cannot compile, refused BY NAME like
        // the lathe; recipeOpsResolved agrees it is not safe downstream.
        AddExtrudedProfileOp unlowered;
        unlowered.name = "shaft.extruded";
        unlowered.profile = "outline";
        unlowered.height = 4.0;
        const RecipeCompileResult refused = compileRecipeOps({RecipeOp{unlowered}});
        EDI_CHECK(!refused.ok);
        EDI_CHECK(refused.message
               == "AddExtrudedProfile must be resolved before compiling: shaft.extruded");
        EDI_CHECK(refused.ops.empty());
        RecipeOpStream survives;
        survives.ops = {RecipeOp{unlowered}};
        EDI_CHECK(!recipeOpsResolved(survives));

        // Refusal #2: a zero height fails validate by its finding name; a valid
        // op passes; a NEGATIVE height is deliberately allowed (BL-05 push/pull).
        AddExtrudedProfileOp flat = unlowered;
        flat.height = 0.0;
        const OpValidationReport flatReport = validateRecipeOps({RecipeOp{flat}});
        EDI_CHECK(!flatReport.ok);
        bool sawZeroHeight = false;
        for (const OpFinding &f : flatReport.findings) {
            sawZeroHeight = sawZeroHeight || f.code == "extruded_profile_zero_height";
        }
        EDI_CHECK(sawZeroHeight);
        EDI_CHECK(validateRecipeOps({RecipeOp{unlowered}}).ok); // height 4.0, valid
        AddExtrudedProfileOp pushPull = unlowered;
        pushPull.height = -1.5; // allowed: not refused here
        const OpValidationReport negReport = validateRecipeOps({RecipeOp{pushPull}});
        for (const OpFinding &f : negReport.findings) {
            EDI_CHECK(f.code != "extruded_profile_zero_height");
        }

        // Bind registry + schema, pinned: height/base_z/x/y bind; profile and
        // material stay literal-only; the inspector exposes name + read-only
        // profile + material.
        RecipeOp bindProbe = RecipeOp{prism};
        for (const char *key : {"height", "base_z", "x", "y"}) {
            EDI_CHECK(opFieldBindable(bindProbe, key));
        }
        for (const char *key : {"profile", "material", "name"}) {
            EDI_CHECK(!opFieldBindable(bindProbe, key));
        }
        bool sawProfileRO = false, sawMaterial = false, sawHeight = false;
        for (const RecipeOpScalar &s : opEditableScalars(bindProbe)) {
            if (s.key == "profile") sawProfileRO = !s.editable && s.text == "panel_outline";
            else if (s.key == "material") sawMaterial = s.kind == RecipeFieldKind::Choice;
            else if (s.key == "height") sawHeight = s.kind == RecipeFieldKind::Number && near(s.number, 2.5);
        }
        EDI_CHECK(sawProfileRO && sawMaterial && sawHeight);
    }

    // ---- BL-05: pin the push/pull AUTHORING SURFACE on AddExtrudedProfile —
    // `height` is a depth verb that needs NO new mechanism: it is a bindable
    // Number opField (the right-click bind affordance's target) AND an editable
    // Number scalar (the field editor renders it), and a write actually REACHES
    // the height member (so the bind/measure path lands on the right double).
    {
        RecipeOp op = RecipeOp{AddExtrudedProfileOp{}};
        // (a) the bind affordance target: height is bindable.
        EDI_CHECK(opFieldBindable(op, "height"));
        // (b) a write reaches the height MEMBER (the gap-check: the binding/
        //     measure path must land on height, not silently miss it).
        EDI_CHECK(setOpFieldValue(op, "height", 7.5));
        EDI_CHECK(near(std::get_if<AddExtrudedProfileOp>(&op)->height, 7.5));
        // (c) the field editor sees an editable Number for height.
        bool heightIsEditableNumber = false;
        for (const RecipeOpScalar &s : opEditableScalars(op)) {
            if (s.key == "height") {
                heightIsEditableNumber = s.kind == RecipeFieldKind::Number && near(s.number, 7.5);
            }
        }
        EDI_CHECK(heightIsEditableNumber);
    }

    // ---- AddPrism (BL-03): the BUILDABLE lowered carrier. Round trip the
    // footprint.i.{x,y} run key-for-key, and pin its validate findings. Unlike
    // the profile-reference ops it passes compile and is NOT refused. ----
    {
        RecipeOpStream zoo;
        zoo.id = "prism.zoo";
        zoo.name = "Prism Zoo";
        AddPrismOp prism;
        prism.name = "wall.block";
        prism.footprint = {{1.2, 0.8}, {3.6, 0.8}, {3.6, 2.4}, {1.2, 2.4}};
        prism.height = 2.0;
        prism.baseZ = 1.0;
        prism.x = 0.5;
        prism.y = -0.25;
        prism.material = "marble";
        zoo.ops.push_back(prism);

        const OpStreamTextResult written = recipeOpsToToml(zoo);
        EDI_CHECK(written.ok);
        EDI_CHECK(written.text.find("op.0.type = \"AddPrism\"") != std::string::npos);
        EDI_CHECK(written.text.find("op.0.height = \"2\"") != std::string::npos);
        EDI_CHECK(written.text.find("op.0.base_z = \"1\"") != std::string::npos);
        EDI_CHECK(written.text.find("op.0.footprint.0.x = \"1.2\"") != std::string::npos);
        EDI_CHECK(written.text.find("op.0.footprint.2.y = \"2.4\"") != std::string::npos);

        const OpStreamParseResult back = recipeOpsFromToml(written.text, "prism.zoo");
        EDI_CHECK(back.ok && back.stream.ops.size() == 1);
        const auto *prismBack = std::get_if<AddPrismOp>(&back.stream.ops[0]);
        EDI_CHECK(prismBack != nullptr && prismBack->name == "wall.block"
               && near(prismBack->height, 2.0) && near(prismBack->baseZ, 1.0)
               && near(prismBack->x, 0.5) && near(prismBack->y, -0.25)
               && prismBack->material == "marble");
        EDI_CHECK(prismBack->footprint.size() == 4);
        EDI_CHECK(near(prismBack->footprint[0].x, 1.2) && near(prismBack->footprint[0].y, 0.8));
        EDI_CHECK(near(prismBack->footprint[3].x, 1.2) && near(prismBack->footprint[3].y, 2.4));
        EDI_CHECK(recipeOpsToToml(back.stream).text == written.text); // idempotent

        // BUILDABLE carrier: compile passes it through untouched (like
        // AddMoulding), and it does NOT make the stream unresolved.
        const RecipeCompileResult compiled = compileRecipeOps({RecipeOp{prism}});
        EDI_CHECK(compiled.ok && compiled.ops.size() == 1
               && recipeOpTypeName(compiled.ops[0]) == std::string("AddPrism"));
        RecipeOpStream resolvedStream;
        resolvedStream.ops = {RecipeOp{prism}};
        EDI_CHECK(recipeOpsResolved(resolvedStream));
        EDI_CHECK(validateRecipeOps({RecipeOp{prism}}).ok);

        // Validate refusals by name: a degenerate footprint (< 3 points) and a
        // zero/non-finite height. A NEGATIVE height is ALLOWED (BL-05).
        AddPrismOp sliver = prism;
        sliver.footprint = {{0.0, 0.0}, {1.0, 0.0}}; // a line, not an area
        const OpValidationReport sliverReport = validateRecipeOps({RecipeOp{sliver}});
        EDI_CHECK(!sliverReport.ok);
        bool sawDegenerate = false;
        for (const OpFinding &f : sliverReport.findings) {
            sawDegenerate = sawDegenerate || f.code == "prism_degenerate_footprint";
        }
        EDI_CHECK(sawDegenerate);

        AddPrismOp flat = prism;
        flat.height = 0.0;
        const OpValidationReport flatReport = validateRecipeOps({RecipeOp{flat}});
        EDI_CHECK(!flatReport.ok);
        bool sawZeroHeight = false;
        for (const OpFinding &f : flatReport.findings) {
            sawZeroHeight = sawZeroHeight || f.code == "prism_zero_height";
        }
        EDI_CHECK(sawZeroHeight);

        AddPrismOp pushPull = prism;
        pushPull.height = -1.5; // allowed
        const OpValidationReport negReport = validateRecipeOps({RecipeOp{pushPull}});
        for (const OpFinding &f : negReport.findings) {
            EDI_CHECK(f.code != "prism_zero_height");
        }
        EDI_CHECK(negReport.ok);
    }

    // ---- P4: taperCurve — non-linear taper exponent on BOTH AddPrismOp and
    // AddSweepProfileOp. Default 1.0 (linear = behavior-preserving). The exponent
    // t^taperCurve remaps the path-fraction before the taper lerp; 1.0 is the
    // identity (t^1=t). Must be positive and finite (0 erases all taper silently).
    // Round-trip, validate refusals (<=0 / non-finite / NaN), default 1.0. ----
    {
        // Default struct value is 1.0 on both ops.
        AddPrismOp defPrism;
        defPrism.name = "p"; defPrism.height = 1.0; defPrism.baseZ = 0.0;
        defPrism.footprint = {{0,0},{1,0},{1,1}};
        EDI_CHECK(near(defPrism.taperCurve, 1.0));

        AddSweepProfileOp defSweep;
        defSweep.name = "s"; defSweep.profile = "prof"; defSweep.path = "pth";
        EDI_CHECK(near(defSweep.taperCurve, 1.0));

        // A non-default taperCurve round-trips on AddPrismOp.
        RecipeOpStream ps;
        ps.id = "p4.zoo"; ps.name = "P4 Zoo";
        AddPrismOp curved = defPrism;
        curved.taperCurve = 2.5;
        curved.taperEnd = 0.4;
        ps.ops.push_back(curved);
        const OpStreamTextResult pw = recipeOpsToToml(ps);
        EDI_CHECK(pw.ok);
        EDI_CHECK(pw.text.find("op.0.taper_curve = \"2.5\"") != std::string::npos);
        const OpStreamParseResult pback = recipeOpsFromToml(pw.text, "p4.zoo");
        EDI_CHECK(pback.ok && pback.stream.ops.size() == 1);
        const auto *prismBack = std::get_if<AddPrismOp>(&pback.stream.ops[0]);
        EDI_CHECK(prismBack != nullptr && near(prismBack->taperCurve, 2.5));

        // Default 1.0 round-trips: the TOML has taper_curve = "1" and reads
        // back as 1.0 — a pre-P4 stream that omits the key also parses to 1.0.
        RecipeOpStream defaults;
        defaults.id = "p4.def"; defaults.name = "P4 Defaults";
        defaults.ops.push_back(defPrism);
        const OpStreamTextResult dw = recipeOpsToToml(defaults);
        EDI_CHECK(dw.ok);
        EDI_CHECK(dw.text.find("op.0.taper_curve = \"1\"") != std::string::npos);
        const auto *dback = std::get_if<AddPrismOp>(&recipeOpsFromToml(dw.text, "p4.def").stream.ops[0]);
        EDI_CHECK(dback != nullptr && near(dback->taperCurve, 1.0));

        // Validate refuses taperCurve <= 0 or non-finite.
        auto checkBadCurve = [](AddPrismOp op, double curve) {
            op.taperCurve = curve;
            const OpValidationReport r = validateRecipeOps({RecipeOp{op}});
            bool saw = false;
            for (const OpFinding &f : r.findings) {
                saw = saw || f.code == "bad_taper_curve";
            }
            EDI_CHECK(!r.ok && saw);
        };
        checkBadCurve(defPrism, 0.0);
        checkBadCurve(defPrism, -1.0);
        checkBadCurve(defPrism, std::numeric_limits<double>::infinity());
        checkBadCurve(defPrism, std::numeric_limits<double>::quiet_NaN());

        // Validate ACCEPTS taperCurve > 0 (e.g. 0.5 front-loads the narrowing).
        AddPrismOp halfCurve = defPrism;
        halfCurve.taperCurve = 0.5;
        EDI_CHECK(validateRecipeOps({RecipeOp{halfCurve}}).ok);
    }

    // ---- P4b: taperEndY — per-axis Y taper with a 0-sentinel (default 0 =
    // follow taperEnd = uniform). 0 is ALLOWED (it's the sentinel, not a
    // degenerate scale); only < 0 and non-finite are refused. ----
    {
        AddPrismOp defPrism;
        defPrism.name = "q"; defPrism.height = 1.0; defPrism.baseZ = 0.0;
        defPrism.footprint = {{0,0},{1,0},{1,1}};
        EDI_CHECK(near(defPrism.taperEndY, 0.0)); // default is the sentinel

        // Non-default taperEndY round-trips.
        RecipeOpStream ps;
        ps.id = "p4b.zoo"; ps.name = "P4b Zoo";
        AddPrismOp asym = defPrism;
        asym.taperEndY = 0.3; // Y narrows more aggressively than X
        ps.ops.push_back(asym);
        const OpStreamTextResult pw = recipeOpsToToml(ps);
        EDI_CHECK(pw.ok);
        EDI_CHECK(pw.text.find("op.0.taper_end_y = \"0.3\"") != std::string::npos);
        const OpStreamParseResult pback = recipeOpsFromToml(pw.text, "p4b.zoo");
        EDI_CHECK(pback.ok && pback.stream.ops.size() == 1);
        const auto *back = std::get_if<AddPrismOp>(&pback.stream.ops[0]);
        EDI_CHECK(back != nullptr && near(back->taperEndY, 0.3));

        // Default 0 round-trips as "0" in TOML, reads back as 0.0.
        RecipeOpStream ds;
        ds.id = "p4b.def"; ds.name = "P4b Defaults";
        ds.ops.push_back(defPrism);
        const OpStreamTextResult dw = recipeOpsToToml(ds);
        EDI_CHECK(dw.ok);
        EDI_CHECK(dw.text.find("op.0.taper_end_y = \"0\"") != std::string::npos);
        const auto *dback = std::get_if<AddPrismOp>(&recipeOpsFromToml(dw.text, "p4b.def").stream.ops[0]);
        EDI_CHECK(dback != nullptr && near(dback->taperEndY, 0.0));

        // Validate: 0 (sentinel) and positive values are ACCEPTED.
        EDI_CHECK(validateRecipeOps({RecipeOp{defPrism}}).ok);     // 0 accepted
        AddPrismOp posY = defPrism; posY.taperEndY = 0.5;
        EDI_CHECK(validateRecipeOps({RecipeOp{posY}}).ok);          // 0.5 accepted

        // Validate: negative and non-finite are REFUSED with bad_taper_end_y.
        auto checkBadY = [](AddPrismOp op, double y) {
            op.taperEndY = y;
            const OpValidationReport r = validateRecipeOps({RecipeOp{op}});
            bool saw = false;
            for (const OpFinding &f : r.findings) {
                saw = saw || f.code == "bad_taper_end_y";
            }
            EDI_CHECK(!r.ok && saw);
        };
        checkBadY(defPrism, -0.1);
        checkBadY(defPrism, -1.0);
        checkBadY(defPrism, std::numeric_limits<double>::infinity());
        checkBadY(defPrism, std::numeric_limits<double>::quiet_NaN());
    }

    // ---- BL-06: partial-angle revolve sweepDegrees — a numeric field on BOTH
    // the lathe and the moulding it lowers to (it must survive lowering), with
    // 360 as the behavior-preserving default. Round-trip, default, validate
    // bounds (0, 360], and the bind affordance, on both ops. ----
    {
        // Default 360 round-trips and is the parsed default when the key is
        // absent (a pre-BL-06 file stays a full revolve).
        AddMouldingOp m;
        m.name = "full.band";
        m.profile = {{"a", 0.0, 1.0}, {"b", 0.5, 1.0}};
        EDI_CHECK(near(m.sweepDegrees, 360.0)); // struct default
        RecipeOpStream ms;
        ms.ops.push_back(m);
        const OpStreamTextResult mw = recipeOpsToToml(ms);
        EDI_CHECK(mw.ok);
        EDI_CHECK(mw.text.find("op.0.sweep_degrees = \"360\"") != std::string::npos);
        const auto *mBack = std::get_if<AddMouldingOp>(&recipeOpsFromToml(mw.text, "m").stream.ops[0]);
        EDI_CHECK(mBack != nullptr && near(mBack->sweepDegrees, 360.0));

        // A non-default sweep round-trips on the lathe.
        AddRevolvedProfileOp lathe;
        lathe.name = "niche";
        lathe.profile = "arch_profile";
        lathe.sweepDegrees = 180.0;
        RecipeOpStream ls;
        ls.ops.push_back(lathe);
        const OpStreamTextResult lw = recipeOpsToToml(ls);
        EDI_CHECK(lw.ok && lw.text.find("op.0.sweep_degrees = \"180\"") != std::string::npos);
        const auto *lBack = std::get_if<AddRevolvedProfileOp>(&recipeOpsFromToml(lw.text, "l").stream.ops[0]);
        EDI_CHECK(lBack != nullptr && near(lBack->sweepDegrees, 180.0));

        // An absent key parses to the 360 default (the byte-preserving guarantee
        // for any pre-BL-06 hand file).
        const OpStreamParseResult bare = recipeOpsFromToml(
            "op.0.type = \"AddRevolvedProfile\"\n"
            "op.0.name = \"bare\"\n"
            "op.0.profile = \"p\"\n"
            "op.0.base_z = \"0\"\n", "bare");
        EDI_CHECK(bare.ok);
        EDI_CHECK(near(std::get_if<AddRevolvedProfileOp>(&bare.stream.ops[0])->sweepDegrees, 360.0));

        // Validate refuses 0 / negative / > 360 by name, on BOTH ops.
        const auto sawBadSweep = [](const OpValidationReport &r) {
            for (const OpFinding &f : r.findings) {
                if (f.code == "bad_sweep_degrees") return true;
            }
            return false;
        };
        AddMouldingOp zero = m; zero.sweepDegrees = 0.0;
        EDI_CHECK(sawBadSweep(validateRecipeOps({RecipeOp{zero}})));
        AddMouldingOp over = m; over.sweepDegrees = 361.0;
        EDI_CHECK(sawBadSweep(validateRecipeOps({RecipeOp{over}})));
        AddRevolvedProfileOp neg; neg.name = "n"; neg.profile = "p"; neg.sweepDegrees = -90.0;
        EDI_CHECK(sawBadSweep(validateRecipeOps({RecipeOp{neg}})));
        // The boundary 360 is allowed (full revolve); 180 is allowed.
        AddMouldingOp ok360 = m; ok360.sweepDegrees = 360.0;
        EDI_CHECK(!sawBadSweep(validateRecipeOps({RecipeOp{ok360}})));

        // The bind affordance: sweep_degrees is a bindable Number on both ops
        // (a drafted angle could drive it) — consistent with how the reader
        // reads it (bindableNumber) and the inspector surfaces it.
        EDI_CHECK(opFieldBindable(RecipeOp{AddMouldingOp{}}, "sweep_degrees"));
        EDI_CHECK(opFieldBindable(RecipeOp{AddRevolvedProfileOp{}}, "sweep_degrees"));
        RecipeOp probe = RecipeOp{AddMouldingOp{}};
        EDI_CHECK(setOpFieldValue(probe, "sweep_degrees", 90.0));
        EDI_CHECK(near(std::get_if<AddMouldingOp>(&probe)->sweepDegrees, 90.0));
    }

    // ---- BL-07: screw/helix params (screw_rise, screw_turns) — numeric fields
    // on BOTH lathe and moulding, default 0/1 = no helix (behavior-preserving).
    // Round-trip, default, validate bounds, and the bind affordance. ----
    {
        // Defaults: rise 0, turns 1, and they round-trip; an absent key parses
        // to the default (a pre-BL-07 file stays non-helical).
        AddMouldingOp m;
        m.name = "thread.band";
        m.profile = {{"a", 0.0, 1.0}, {"b", 0.5, 1.0}};
        EDI_CHECK(near(m.screwRise, 0.0) && near(m.screwTurns, 1.0)); // struct defaults
        m.screwRise = 2.0;
        m.screwTurns = 4.0;
        RecipeOpStream ms;
        ms.ops.push_back(m);
        const OpStreamTextResult mw = recipeOpsToToml(ms);
        EDI_CHECK(mw.ok);
        EDI_CHECK(mw.text.find("op.0.screw_rise = \"2\"") != std::string::npos);
        EDI_CHECK(mw.text.find("op.0.screw_turns = \"4\"") != std::string::npos);
        const auto *mBack = std::get_if<AddMouldingOp>(&recipeOpsFromToml(mw.text, "m").stream.ops[0]);
        EDI_CHECK(mBack != nullptr && near(mBack->screwRise, 2.0) && near(mBack->screwTurns, 4.0));

        // Absent keys default to 0 / 1 on the lathe (the pre-BL-07 guarantee).
        const OpStreamParseResult bare = recipeOpsFromToml(
            "op.0.type = \"AddRevolvedProfile\"\n"
            "op.0.name = \"bare\"\n"
            "op.0.profile = \"p\"\n"
            "op.0.base_z = \"0\"\n", "bare");
        EDI_CHECK(bare.ok);
        const auto *bareL = std::get_if<AddRevolvedProfileOp>(&bare.stream.ops[0]);
        EDI_CHECK(near(bareL->screwRise, 0.0) && near(bareL->screwTurns, 1.0));

        // Validate: screw_turns <= 0 refused by name on BOTH ops; a NEGATIVE
        // screw_rise (left-hand spiral) is allowed.
        const auto sawBadTurns = [](const OpValidationReport &r) {
            for (const OpFinding &f : r.findings) {
                if (f.code == "bad_screw_turns") return true;
            }
            return false;
        };
        AddMouldingOp zeroTurns = m; zeroTurns.screwTurns = 0.0;
        EDI_CHECK(sawBadTurns(validateRecipeOps({RecipeOp{zeroTurns}})));
        AddRevolvedProfileOp negTurns; negTurns.name = "n"; negTurns.profile = "p";
        negTurns.screwTurns = -1.0;
        EDI_CHECK(sawBadTurns(validateRecipeOps({RecipeOp{negTurns}})));
        AddMouldingOp leftHand = m; leftHand.screwRise = -3.0; // left-hand spiral, allowed
        const OpValidationReport leftReport = validateRecipeOps({RecipeOp{leftHand}});
        EDI_CHECK(leftReport.ok);

        // BATCH-2 P1: a partial sweep_degrees beside a helix is a non-fatal
        // WARNING (the v1 helix ignores it — full turns). Fires only when BOTH a
        // partial sweep AND a non-zero rise are set, on BOTH ops.
        const auto sawHelixWarn = [](const OpValidationReport &r) {
            for (const OpFinding &f : r.findings) {
                if (f.code == "helix_ignores_partial_sweep") return true;
            }
            return false;
        };
        AddMouldingOp clash = m; clash.sweepDegrees = 90.0; clash.screwRise = 1.0;
        const OpValidationReport clashReport = validateRecipeOps({RecipeOp{clash}});
        EDI_CHECK(clashReport.ok);                 // a warning is non-fatal
        EDI_CHECK(sawHelixWarn(clashReport));
        AddMouldingOp partialOnly = m; partialOnly.sweepDegrees = 90.0; partialOnly.screwRise = 0.0;
        EDI_CHECK(!sawHelixWarn(validateRecipeOps({RecipeOp{partialOnly}})));
        AddMouldingOp fullHelix = m; fullHelix.sweepDegrees = 360.0; fullHelix.screwRise = 1.0;
        EDI_CHECK(!sawHelixWarn(validateRecipeOps({RecipeOp{fullHelix}})));
        // Also fires on the lathe (the other op carrying the fields).
        AddRevolvedProfileOp latheClash;
        latheClash.name = "l"; latheClash.profile = "p";
        latheClash.sweepDegrees = 90.0; latheClash.screwRise = 1.0;
        EDI_CHECK(sawHelixWarn(validateRecipeOps({RecipeOp{latheClash}})));

        // The bind affordance: both screw params are bindable Numbers on both ops.
        for (const char *key : {"screw_rise", "screw_turns"}) {
            EDI_CHECK(opFieldBindable(RecipeOp{AddMouldingOp{}}, key));
            EDI_CHECK(opFieldBindable(RecipeOp{AddRevolvedProfileOp{}}, key));
        }
        RecipeOp probe = RecipeOp{AddRevolvedProfileOp{}};
        EDI_CHECK(setOpFieldValue(probe, "screw_rise", 5.0));
        EDI_CHECK(near(std::get_if<AddRevolvedProfileOp>(&probe)->screwRise, 5.0));
    }

    // ---- BL-14: the named-recipe LIBRARY — save/list/load round-trip through a
    // temp dir, reusing the stream (de)serializers. ----
    {
        namespace fs = std::filesystem;
        const fs::path dir = fs::temp_directory_path()
            / ("edi_recipe_lib_" + std::to_string(std::random_device{}()));
        fs::remove_all(dir);

        RecipeOpStream lib;
        lib.id = "lib_pedestal";
        lib.name = "pedestal";
        AddBoxOp slab;
        slab.name = "slab";
        slab.width = slab.depth = 3.0;
        slab.height = 0.5;
        lib.ops.push_back(slab);
        AddCylinderOp drum;
        drum.name = "drum";
        drum.radius = 1.0;
        drum.height = 4.0;
        lib.ops.push_back(drum);
        lib.bindings = {{1, "radius", "gauge", "radius"}};

        const OpStreamTextResult saved = saveLibraryRecipe(dir.string(), lib);
        EDI_CHECK(saved.ok);
        const std::vector<std::string> names = listLibraryRecipes(dir.string());
        EDI_CHECK(names.size() == 1 && names[0] == "pedestal"); // filename = sanitized name
        const OpStreamParseResult loaded = loadLibraryRecipe(dir.string(), "pedestal");
        EDI_CHECK(loaded.ok);
        EDI_CHECK(loaded.stream.ops.size() == 2);
        EDI_CHECK(loaded.stream.id == "lib_pedestal" && loaded.stream.name == "pedestal");
        EDI_CHECK(std::get_if<AddBoxOp>(&loaded.stream.ops[0])->name == "slab");
        EDI_CHECK(loaded.stream.bindings.size() == 1
               && loaded.stream.bindings[0].opIndex == 1
               && loaded.stream.bindings[0].objectId == "gauge");
        // A missing recipe fails by name, not a crash.
        const OpStreamParseResult missing = loadLibraryRecipe(dir.string(), "nope");
        EDI_CHECK(!missing.ok && missing.message.find("recipe not found") != std::string::npos);
        // A missing directory lists empty (not an error).
        EDI_CHECK(listLibraryRecipes((dir / "no_such").string()).empty());
        fs::remove_all(dir);
    }

    // ---- BL-14: appendRecipe — binding index re-offset. A source binding on
    // op j lands at j + target.ops.size(); the target binding is untouched. ----
    {
        RecipeOpStream target;
        AddBoxOp a; a.name = "t.a"; a.width = a.depth = a.height = 1.0;
        AddBoxOp b; b.name = "t.b"; b.width = b.depth = b.height = 1.0;
        target.ops = {RecipeOp{a}, RecipeOp{b}};       // M = 2 ops
        target.bindings = {{0, "width", "t_obj", "width"}};

        RecipeOpStream source;
        source.name = "src";
        AddCylinderOp c; c.name = "s.c"; c.radius = 1.0; c.height = 2.0;
        source.ops = {RecipeOp{c}};
        source.bindings = {{0, "radius", "s_obj", "radius"}}; // j = 0

        appendRecipe(target, source);
        EDI_CHECK(target.ops.size() == 3);
        EDI_CHECK(target.bindings.size() == 2);
        // The target binding is untouched.
        EDI_CHECK(target.bindings[0].opIndex == 0 && target.bindings[0].objectId == "t_obj");
        // The source binding re-offset by M=2: j(0) -> 2.
        EDI_CHECK(target.bindings[1].opIndex == 2 && target.bindings[1].objectId == "s_obj");
    }

    // ---- BL-14 KEY TEST: name namespacing prevents cross-reference. Target has
    // an op named "shaft"; source has its OWN "shaft" plus a CutFlutes targeting
    // "shaft". After append the source names are namespaced and the flute points
    // at the SOURCE shaft (not the target's), and the merged stream validates. ----
    {
        RecipeOpStream target;
        AddCylinderOp targetShaft;
        targetShaft.name = "shaft";  // a name the source ALSO uses
        targetShaft.radius = 2.0;
        targetShaft.height = 10.0;
        target.ops = {RecipeOp{targetShaft}};

        RecipeOpStream source;
        source.name = "capital";
        AddCylinderOp srcShaft;
        srcShaft.name = "shaft";     // same bare name as the target's
        srcShaft.radius = 1.0;
        srcShaft.height = 4.0;
        CutFlutesOp flutes;
        flutes.target = "shaft";     // must resolve to the SOURCE shaft
        flutes.count = 20;
        flutes.depth = 0.1;
        source.ops = {RecipeOp{srcShaft}, RecipeOp{flutes}};

        appendRecipe(target, source);
        EDI_CHECK(target.ops.size() == 3);
        // The target shaft keeps its bare name (never namespaced).
        EDI_CHECK(std::get_if<AddCylinderOp>(&target.ops[0])->name == "shaft");
        // The source ops are namespaced under "capital::".
        EDI_CHECK(std::get_if<AddCylinderOp>(&target.ops[1])->name == "capital::shaft");
        // The source flute's target was rewritten to the NAMESPACED source shaft,
        // not left pointing at the target's bare "shaft".
        EDI_CHECK(std::get_if<CutFlutesOp>(&target.ops[2])->target == "capital::shaft");
        // The merged stream validates: the flute finds its target within the
        // spliced block (validation matches names in order).
        EDI_CHECK(validateRecipeOps(target.ops).ok);
    }

    // ---- BL-08: AddSweepProfile (the Follow-Me ref-op) round-trip + refusals,
    // and AddPrism's optional `path` (empty AND non-empty), empty byte-identical. ----
    {
        // AddSweepProfile round-trips key-for-key (two drafted refs).
        RecipeOpStream ss;
        AddSweepProfileOp sweep;
        sweep.name = "cornice.run";
        sweep.profile = "section";
        sweep.path = "run_path";
        sweep.baseZ = 1.0;
        sweep.material = "marble";
        ss.ops.push_back(sweep);
        const OpStreamTextResult sw = recipeOpsToToml(ss);
        EDI_CHECK(sw.ok);
        EDI_CHECK(sw.text.find("op.0.type = \"AddSweepProfile\"") != std::string::npos);
        EDI_CHECK(sw.text.find("op.0.profile = \"section\"") != std::string::npos);
        EDI_CHECK(sw.text.find("op.0.path = \"run_path\"") != std::string::npos);
        const auto *swBack = std::get_if<AddSweepProfileOp>(&recipeOpsFromToml(sw.text, "s").stream.ops[0]);
        EDI_CHECK(swBack != nullptr && swBack->profile == "section" && swBack->path == "run_path"
               && near(swBack->baseZ, 1.0) && swBack->material == "marble");

        // Refused-before-build by name; recipeOpsResolved false while it survives.
        const RecipeCompileResult refused = compileRecipeOps({RecipeOp{sweep}});
        EDI_CHECK(!refused.ok);
        EDI_CHECK(refused.message == "AddSweepProfile must be resolved before compiling: cornice.run");
        RecipeOpStream survives;
        survives.ops = {RecipeOp{sweep}};
        EDI_CHECK(!recipeOpsResolved(survives));
        // Validate refuses an empty profile/path by name.
        AddSweepProfileOp noPath;
        noPath.name = "n";
        noPath.profile = "section"; // path empty
        const OpValidationReport npr = validateRecipeOps({RecipeOp{noPath}});
        bool sawMissingPath = false;
        for (const OpFinding &f : npr.findings) {
            sawMissingPath = sawMissingPath || f.code == "missing_path_reference";
        }
        EDI_CHECK(sawMissingPath);

        // AddPrism with a non-empty path round-trips the path.i.{x,y} run.
        RecipeOpStream ps;
        AddPrismOp prism;
        prism.name = "swept";
        prism.footprint = {{-0.5, 0.0}, {0.5, 0.0}, {0.5, 1.0}, {-0.5, 1.0}};
        prism.path = {{0.0, 0.0}, {4.0, 0.0}, {4.0, 3.0}};
        ps.ops.push_back(prism);
        const OpStreamTextResult pw = recipeOpsToToml(ps);
        EDI_CHECK(pw.ok);
        EDI_CHECK(pw.text.find("op.0.path.0.x = \"0\"") != std::string::npos);
        EDI_CHECK(pw.text.find("op.0.path.2.y = \"3\"") != std::string::npos);
        const auto *pBack = std::get_if<AddPrismOp>(&recipeOpsFromToml(pw.text, "p").stream.ops[0]);
        EDI_CHECK(pBack != nullptr && pBack->path.size() == 3
               && near(pBack->path[2].x, 4.0) && near(pBack->path[2].y, 3.0));
        // A swept prism (path present) validates with NO zero-height finding
        // (height is irrelevant when a path drives the solid).
        const OpValidationReport pvr = validateRecipeOps({RecipeOp{prism}});
        EDI_CHECK(pvr.ok);
        for (const OpFinding &f : pvr.findings) {
            EDI_CHECK(f.code != "prism_zero_height");
        }

        // The empty-path prism emits NO path.* keys and round-trips exactly as a
        // pre-BL-08 straight extrude — path stays empty.
        RecipeOpStream es;
        AddPrismOp straight;
        straight.name = "straight";
        straight.footprint = {{0.0, 0.0}, {1.0, 0.0}, {1.0, 1.0}};
        straight.height = 2.0; // empty path -> a straight extrude needs a height
        es.ops.push_back(straight);
        const OpStreamTextResult ew = recipeOpsToToml(es);
        EDI_CHECK(ew.ok);
        EDI_CHECK(ew.text.find(".path.") == std::string::npos); // no path keys at all
        const auto *eBack = std::get_if<AddPrismOp>(&recipeOpsFromToml(ew.text, "e").stream.ops[0]);
        EDI_CHECK(eBack != nullptr && eBack->path.empty() && near(eBack->height, 2.0));
    }

    // ---- BL-09: taper_end on the sweep + the prism it lowers to. Default 1.0,
    // round-trip on both ops, validate refuses <= 0 / non-finite by name. ----
    {
        // Default 1.0 round-trips on both ops.
        AddSweepProfileOp sweep;
        sweep.name = "spire";
        sweep.profile = "section";
        sweep.path = "rib";
        EDI_CHECK(near(sweep.taperEnd, 1.0)); // struct default
        sweep.taperEnd = 0.4;
        RecipeOpStream ss;
        ss.ops.push_back(sweep);
        const OpStreamTextResult sw = recipeOpsToToml(ss);
        EDI_CHECK(sw.ok && sw.text.find("op.0.taper_end = \"0.4\"") != std::string::npos);
        const auto *swBack = std::get_if<AddSweepProfileOp>(&recipeOpsFromToml(sw.text, "s").stream.ops[0]);
        EDI_CHECK(swBack != nullptr && near(swBack->taperEnd, 0.4));

        AddPrismOp prism;
        prism.name = "p";
        prism.footprint = {{0.0, 0.0}, {1.0, 0.0}, {1.0, 1.0}};
        prism.path = {{0.0, 0.0}, {2.0, 0.0}};
        prism.taperEnd = 0.5;
        RecipeOpStream ps;
        ps.ops.push_back(prism);
        const OpStreamTextResult pw = recipeOpsToToml(ps);
        EDI_CHECK(pw.ok && pw.text.find("op.0.taper_end = \"0.5\"") != std::string::npos);
        const auto *pBack = std::get_if<AddPrismOp>(&recipeOpsFromToml(pw.text, "p").stream.ops[0]);
        EDI_CHECK(pBack != nullptr && near(pBack->taperEnd, 0.5));

        // An absent taper_end defaults to 1.0 (a pre-BL-09 file is untapered).
        const OpStreamParseResult bare = recipeOpsFromToml(
            "op.0.type = \"AddSweepProfile\"\n"
            "op.0.name = \"bare\"\n"
            "op.0.profile = \"s\"\n"
            "op.0.path = \"p\"\n"
            "op.0.base_z = \"0\"\n", "bare");
        EDI_CHECK(bare.ok);
        EDI_CHECK(near(std::get_if<AddSweepProfileOp>(&bare.stream.ops[0])->taperEnd, 1.0));

        // Validate refuses taper_end <= 0 / non-finite by name on BOTH ops.
        const auto sawBadTaper = [](const OpValidationReport &r) {
            for (const OpFinding &f : r.findings) {
                if (f.code == "bad_taper_end") return true;
            }
            return false;
        };
        AddSweepProfileOp zeroTaper = sweep; zeroTaper.taperEnd = 0.0;
        EDI_CHECK(sawBadTaper(validateRecipeOps({RecipeOp{zeroTaper}})));
        AddPrismOp negTaper = prism; negTaper.taperEnd = -1.0;
        EDI_CHECK(sawBadTaper(validateRecipeOps({RecipeOp{negTaper}})));

        // The bind affordance: taper_end is a bindable Number on both ops.
        EDI_CHECK(opFieldBindable(RecipeOp{AddSweepProfileOp{}}, "taper_end"));
        EDI_CHECK(opFieldBindable(RecipeOp{AddPrismOp{}}, "taper_end"));
    }

    // ---- BL-10: inset + normal_offset depth params on AddPrism. Round-trip,
    // defaults 0, validate refuses an oversized inset / non-finite by name. ----
    {
        AddPrismOp prism;
        prism.name = "lipped";
        prism.footprint = {{0.0, 0.0}, {4.0, 0.0}, {4.0, 4.0}, {0.0, 4.0}};
        prism.height = 2.0;
        EDI_CHECK(near(prism.inset, 0.0) && near(prism.normalOffset, 0.0)); // struct defaults
        prism.inset = 0.3;
        prism.normalOffset = -0.1;
        RecipeOpStream ps;
        ps.ops.push_back(prism);
        const OpStreamTextResult pw = recipeOpsToToml(ps);
        EDI_CHECK(pw.ok);
        EDI_CHECK(pw.text.find("op.0.inset = \"0.3\"") != std::string::npos);
        EDI_CHECK(pw.text.find("op.0.normal_offset = \"-0.1\"") != std::string::npos);
        const auto *pBack = std::get_if<AddPrismOp>(&recipeOpsFromToml(pw.text, "p").stream.ops[0]);
        EDI_CHECK(pBack != nullptr && near(pBack->inset, 0.3) && near(pBack->normalOffset, -0.1));

        // Absent keys default to 0 (a pre-BL-10 prism is unchanged).
        const OpStreamParseResult bare = recipeOpsFromToml(
            "op.0.type = \"AddPrism\"\n"
            "op.0.name = \"bare\"\n"
            "op.0.height = \"2\"\n"
            "op.0.base_z = \"0\"\n"
            "op.0.footprint.0.x = \"0\"\nop.0.footprint.0.y = \"0\"\n"
            "op.0.footprint.1.x = \"1\"\nop.0.footprint.1.y = \"0\"\n"
            "op.0.footprint.2.x = \"1\"\nop.0.footprint.2.y = \"1\"\n", "bare");
        EDI_CHECK(bare.ok);
        const auto *bareP = std::get_if<AddPrismOp>(&bare.stream.ops[0]);
        EDI_CHECK(near(bareP->inset, 0.0) && near(bareP->normalOffset, 0.0));

        const auto sawCode = [](const OpValidationReport &r, const char *code) {
            for (const OpFinding &f : r.findings) {
                if (f.code == code) return true;
            }
            return false;
        };
        // A 4x4 footprint: smaller extent 4, so inset >= 2.0 would collapse it.
        AddPrismOp big = prism; big.inset = 2.5;
        EDI_CHECK(sawCode(validateRecipeOps({RecipeOp{big}}), "prism_inset_too_large"));
        AddPrismOp okInset = prism; okInset.inset = 0.5; // well under half the extent
        EDI_CHECK(!sawCode(validateRecipeOps({RecipeOp{okInset}}), "prism_inset_too_large"));
        // Non-finite is refused by name on each.
        AddPrismOp nanOffset = prism;
        nanOffset.normalOffset = std::numeric_limits<double>::infinity();
        EDI_CHECK(sawCode(validateRecipeOps({RecipeOp{nanOffset}}), "bad_normal_offset"));

        // Both bindable Numbers.
        EDI_CHECK(opFieldBindable(RecipeOp{AddPrismOp{}}, "inset"));
        EDI_CHECK(opFieldBindable(RecipeOp{AddPrismOp{}}, "normal_offset"));
    }

    // ---- P6: prism_inset_reflex_pinch guard — a second, tighter validate check
    // that fires specifically at reflex corners of NON-CONVEX footprints. The
    // bbox guard (prism_inset_too_large) is too coarse: it passes insets that
    // are safe for a CONVEX footprint at the same bbox size but will pinch a
    // reflex vertex. The per-vertex pinch bound is O(n) and geometry-aware. ----
    {
        // L-shaped footprint (6 vertices, one reflex vertex at the inner corner):
        //   (0,4)─(0,0)─(4,0)─(4,2)
        //   │              │
        //   (2,4)──────(2,2)
        // CCW winding (positive shoelace area). Reflex vertex at (2,2):
        //   edge in from (4,2): length 2 | edge out to (2,4): length 2
        //   pinch limit = 0.5 * min(2, 2) = 1.0.
        const std::vector<PrismPoint> lShape = {
            {0.0, 0.0}, {4.0, 0.0}, {4.0, 2.0}, {2.0, 2.0}, {2.0, 4.0}, {0.0, 4.0}};
        const auto sawCode = [](const OpValidationReport &r, const char *code) {
            for (const OpFinding &f : r.findings) { if (f.code == code) return true; }
            return false;
        };
        AddPrismOp reflexPrism;
        reflexPrism.name = "l_shape";
        reflexPrism.footprint = lShape;
        reflexPrism.height = 2.0;

        // inset = 0.5 → well under the 1.0 pinch limit AND the 2.0 bbox limit.
        // Neither guard fires — this is a safe inset for the L-shape.
        reflexPrism.inset = 0.5;
        EDI_CHECK(!sawCode(validateRecipeOps({RecipeOp{reflexPrism}}), "prism_inset_reflex_pinch"));
        EDI_CHECK(!sawCode(validateRecipeOps({RecipeOp{reflexPrism}}), "prism_inset_too_large"));

        // inset = 1.1 → exceeds the 1.0 pinch limit for the reflex vertex, but
        // is BELOW the 2.0 bbox limit. prism_inset_too_large does NOT fire; the
        // new per-reflex guard DOES. This is the specific case the bbox guard
        // misses but the per-vertex geometry check catches.
        reflexPrism.inset = 1.1;
        EDI_CHECK(sawCode(validateRecipeOps({RecipeOp{reflexPrism}}), "prism_inset_reflex_pinch"));
        EDI_CHECK(!sawCode(validateRecipeOps({RecipeOp{reflexPrism}}), "prism_inset_too_large"));

        // Convex footprint (the 4×4 square from BL-10): has no reflex vertices,
        // so prism_inset_reflex_pinch must NOT fire, even for large insets.
        // (prism_inset_too_large fires instead at inset >= 2.0.)
        AddPrismOp convexPrism;
        convexPrism.name = "square";
        convexPrism.footprint = {{0.0, 0.0}, {4.0, 0.0}, {4.0, 4.0}, {0.0, 4.0}};
        convexPrism.height = 2.0;
        convexPrism.inset = 2.5; // fires bbox guard, NOT reflex guard
        EDI_CHECK(!sawCode(validateRecipeOps({RecipeOp{convexPrism}}), "prism_inset_reflex_pinch"));
        EDI_CHECK(sawCode(validateRecipeOps({RecipeOp{convexPrism}}), "prism_inset_too_large"));
    }

    // ---- BL-11: AddBoolean round-trip (each kind), validate operand ordering,
    // and the remap-hardening chaining (Part 2). ----
    {
        // Round-trip every kind value through the kind TOML key.
        struct KindCase { BooleanKind kind; const char *text; };
        const KindCase cases[] = {
            {BooleanKind::Union, "union"},
            {BooleanKind::Subtract, "subtract"},
            {BooleanKind::Intersect, "intersect"},
        };
        for (const KindCase &c : cases) {
            RecipeOpStream s;
            AddBooleanOp op;
            op.name = "combo";
            op.a = "lhs";
            op.b = "rhs";
            op.kind = c.kind;
            s.ops.push_back(op);
            const OpStreamTextResult w = recipeOpsToToml(s);
            EDI_CHECK(w.ok);
            EDI_CHECK(w.text.find(std::string("op.0.kind = \"") + c.text + "\"") != std::string::npos);
            EDI_CHECK(w.text.find("op.0.a = \"lhs\"") != std::string::npos);
            EDI_CHECK(w.text.find("op.0.b = \"rhs\"") != std::string::npos);
            const auto *back = std::get_if<AddBooleanOp>(&recipeOpsFromToml(w.text, "b").stream.ops[0]);
            EDI_CHECK(back != nullptr && back->a == "lhs" && back->b == "rhs" && back->kind == c.kind);
        }
        // A bad kind value is refused by name.
        const OpStreamParseResult badKind = recipeOpsFromToml(
            "op.0.type = \"AddBoolean\"\n"
            "op.0.name = \"c\"\nop.0.a = \"x\"\nop.0.b = \"y\"\n"
            "op.0.kind = \"merge\"\n", "bad");
        EDI_CHECK(!badKind.ok && badKind.message.find("union, subtract, or intersect") != std::string::npos);

        // Validate: a/b must name an EARLIER op; a later or absent operand is
        // refused by name. A valid earlier-operand boolean validates clean.
        AddBoxOp a; a.name = "solid.a"; a.width = a.depth = a.height = 1.0;
        AddBoxOp b; b.name = "solid.b"; b.width = b.depth = b.height = 1.0;
        AddBooleanOp good;
        good.name = "good.union"; good.a = "solid.a"; good.b = "solid.b";
        EDI_CHECK(validateRecipeOps({RecipeOp{a}, RecipeOp{b}, RecipeOp{good}}).ok);
        const auto sawCode = [](const OpValidationReport &r, const char *code) {
            for (const OpFinding &f : r.findings) {
                if (f.code == code) return true;
            }
            return false;
        };
        // Operand b is absent.
        AddBooleanOp missing;
        missing.name = "bad"; missing.a = "solid.a"; missing.b = "ghost";
        EDI_CHECK(sawCode(validateRecipeOps({RecipeOp{a}, RecipeOp{missing}}), "boolean_missing_operand"));
        // Operand named LATER (the boolean comes before its operand).
        AddBooleanOp early;
        early.name = "early"; early.a = "solid.a"; early.b = "solid.a";
        const OpValidationReport beforeOrder = validateRecipeOps({RecipeOp{early}, RecipeOp{a}});
        EDI_CHECK(sawCode(beforeOrder, "boolean_missing_operand"));
        // a == b is degenerate, refused by name.
        EDI_CHECK(sawCode(validateRecipeOps({RecipeOp{a}, RecipeOp{early}}), "boolean_self_operand"));

        // Part 2: the remap-hardening chaining — a spliced AddBoolean's a/b are
        // namespaced to the SOURCE operands, never the target's same-named ops.
        RecipeOpStream target;
        AddBoxOp tShaft; tShaft.name = "shaft"; tShaft.width = tShaft.depth = tShaft.height = 1.0;
        target.ops = {RecipeOp{tShaft}};
        RecipeOpStream source;
        source.name = "cap";
        AddBoxOp sShaft; sShaft.name = "shaft"; sShaft.width = sShaft.depth = sShaft.height = 1.0;
        AddCylinderOp sBore; sBore.name = "bore"; sBore.radius = 0.5; sBore.height = 2.0;
        AddBooleanOp sBool;
        sBool.name = "cut"; sBool.a = "shaft"; sBool.b = "bore"; sBool.kind = BooleanKind::Subtract;
        source.ops = {RecipeOp{sShaft}, RecipeOp{sBore}, RecipeOp{sBool}};

        appendRecipe(target, source);
        EDI_CHECK(target.ops.size() == 4);
        const auto *mergedBool = std::get_if<AddBooleanOp>(&target.ops[3]);
        EDI_CHECK(mergedBool != nullptr);
        // Both operands rewritten to the namespaced SOURCE names.
        EDI_CHECK(mergedBool->a == "cap::shaft"); // not the target's bare "shaft"
        EDI_CHECK(mergedBool->b == "cap::bore");
        EDI_CHECK(validateRecipeOps(target.ops).ok); // the boolean finds its namespaced operands
    }

    // ---- BL-15: TOON handoff of a RESOLVED stream. Stable output, key parity
    // with the TOML truth, refusal of an unresolved stream, and no JSON. ----
    {
        // Helper: the op.N / recipe. keys of a "key<sep>value" text.
        const auto keysOf = [](const std::string &text, const std::string &sep) {
            std::set<std::string> keys;
            std::size_t pos = 0;
            while (pos < text.size()) {
                const std::size_t eol = text.find('\n', pos);
                const std::string line = text.substr(pos, eol - pos);
                const std::size_t s = line.find(sep);
                if (s != std::string::npos) {
                    const std::string key = line.substr(0, s);
                    if (key.rfind("op.", 0) == 0 || key.rfind("recipe.", 0) == 0) {
                        keys.insert(key);
                    }
                }
                if (eol == std::string::npos) break;
                pos = eol + 1;
            }
            return keys;
        };

        RecipeOpStream s;
        s.id = "toon_demo";
        s.name = "Toon Demo";
        AddBoxOp box;
        box.name = "block";
        box.width = box.depth = box.height = 2.0;
        box.z = 1.0;
        s.ops.push_back(box);
        AddPrismOp prism;
        prism.name = "lid";
        prism.footprint = {{0.0, 0.0}, {1.0, 0.0}, {1.0, 1.0}};
        prism.height = 0.5;
        prism.baseZ = 2.0;
        s.ops.push_back(prism);

        const auto toon = exportRecipeStreamToToon(s);
        EDI_CHECK(toon.ok && toon.value.has_value());
        const std::string &text = *toon.value;
        // TOON shape: kind/title meta, then flat key: value lines.
        EDI_CHECK(text.find("kind: recipe\n") != std::string::npos);
        EDI_CHECK(text.find("title: Toon Demo\n") != std::string::npos);
        EDI_CHECK(text.find("op.0.type: AddBox\n") != std::string::npos);
        EDI_CHECK(text.find("op.1.type: AddPrism\n") != std::string::npos);
        EDI_CHECK(text.find("op.1.height: 0.5\n") != std::string::npos);
        // No JSON object syntax — TOON is line-oriented, never JSON.
        EDI_CHECK(text.find('{') == std::string::npos && text.find('}') == std::string::npos);

        // No-drift guard: the TOON op/recipe keys are EXACTLY the TOML keys.
        const OpStreamTextResult toml = recipeOpsToToml(s);
        EDI_CHECK(toml.ok);
        EDI_CHECK(keysOf(text, ": ") == keysOf(toml.text, " = "));

        // Refusal: an unresolved stream (a surviving binding) is refused by name.
        RecipeOpStream bound = s;
        bound.bindings = {{0, "width", "gauge", "width"}};
        const auto refusedBind = exportRecipeStreamToToon(bound);
        EDI_CHECK(!refusedBind.ok);
        EDI_CHECK(refusedBind.message.find("must be resolved before TOON handoff") != std::string::npos);

        // Refusal: a surviving refused-before-build ref-op (lathe/extrude/sweep).
        RecipeOpStream withLathe = s;
        AddRevolvedProfileOp lathe;
        lathe.name = "turned"; lathe.profile = "shaft";
        withLathe.ops.push_back(lathe);
        EDI_CHECK(!exportRecipeStreamToToon(withLathe).ok);
        AddSweepProfileOp sweep;
        sweep.name = "run"; sweep.profile = "p"; sweep.path = "rib";
        RecipeOpStream withSweep = s;
        withSweep.ops.push_back(sweep);
        EDI_CHECK(!exportRecipeStreamToToon(withSweep).ok);
    }

    // ---- RD2: exportRecipeStreamDiffToToon — semantic diff of two resolved
    // streams emitted as TOON. The diff uses the SAME op.N.<field> key
    // vocabulary as the TOML / BL-15 TOON, so the AI can point at a diff key
    // and know exactly which field to change in the recipe. ----
    {
        // ---- Case 1: a single field change (height 2 → 3). ----
        // Only the changed key appears in the diff; unchanged keys are OMITTED.
        RecipeOpStream before;
        before.id = "diff_demo";
        before.name = "Diff Demo";
        AddBoxOp boxA;
        boxA.name = "block"; boxA.width = boxA.depth = boxA.height = 2.0; boxA.z = 1.0;
        before.ops.push_back(boxA);

        RecipeOpStream after = before;
        std::get<AddBoxOp>(after.ops[0]).height = 3.0; // one field changed

        const auto diffResult = exportRecipeStreamDiffToToon(before, after);
        EDI_CHECK(diffResult.ok && diffResult.value.has_value());
        const std::string &diffText = *diffResult.value;

        // TOON shape: kind/title header then flat key: value lines.
        EDI_CHECK(diffText.find("kind: recipe-diff\n") != std::string::npos);
        EDI_CHECK(diffText.find("title: Diff Demo\n") != std::string::npos); // same name → after.name

        // The changed key appears with "old -> new" (raw config values, no quotes).
        EDI_CHECK(diffText.find("op.0.height: 2 -> 3\n") != std::string::npos);

        // Unchanged keys are OMITTED — the diff carries only deltas.
        EDI_CHECK(diffText.find("op.0.type:") == std::string::npos);
        EDI_CHECK(diffText.find("op.0.name:") == std::string::npos);
        EDI_CHECK(diffText.find("recipe.id:") == std::string::npos);  // recipe keys also omitted when unchanged

        // No JSON object syntax.
        EDI_CHECK(diffText.find('{') == std::string::npos && diffText.find('}') == std::string::npos);

        // ---- Case 2: added op → the new op's keys appear as "(added) -> value". ----
        RecipeOpStream afterAdd = before;
        AddPrismOp extra;
        extra.name = "lid"; extra.footprint = {{0,0},{1,0},{1,1}}; extra.height = 0.5; extra.baseZ = 2.0;
        afterAdd.ops.push_back(extra);

        const auto diffAdd = exportRecipeStreamDiffToToon(before, afterAdd);
        EDI_CHECK(diffAdd.ok && diffAdd.value.has_value());
        const std::string &addText = *diffAdd.value;
        // The added op's type key appears as "(added) -> AddPrism".
        EDI_CHECK(addText.find("op.1.type: (added) -> AddPrism\n") != std::string::npos);
        // The first op (unchanged) does NOT appear.
        EDI_CHECK(addText.find("op.0.") == std::string::npos);

        // ---- Case 3: removed op → the old op's keys appear as "value -> (removed)". ----
        const auto diffRem = exportRecipeStreamDiffToToon(afterAdd, before); // swap: 2 ops → 1 op
        EDI_CHECK(diffRem.ok && diffRem.value.has_value());
        const std::string &remText = *diffRem.value;
        EDI_CHECK(remText.find("op.1.type: AddPrism -> (removed)\n") != std::string::npos);
        EDI_CHECK(remText.find("op.0.") == std::string::npos); // first op unchanged, omitted

        // ---- Case 4: title when stream names differ ("A -> B"). ----
        RecipeOpStream renamedAfter = after;
        renamedAfter.name = "Diff Demo v2";
        const auto diffRename = exportRecipeStreamDiffToToon(before, renamedAfter);
        EDI_CHECK(diffRename.ok && diffRename.value.has_value());
        EDI_CHECK(diffRename.value->find("title: Diff Demo -> Diff Demo v2\n") != std::string::npos);

        // ---- Case 5: refusal — unresolved BEFORE (binding survives). ----
        RecipeOpStream boundBefore = before;
        boundBefore.bindings = {{0, "width", "gauge", "width"}};
        const auto refBefore = exportRecipeStreamDiffToToon(boundBefore, after);
        EDI_CHECK(!refBefore.ok);
        EDI_CHECK(refBefore.message.find("before stream must be resolved") != std::string::npos);

        // ---- Case 6: refusal — unresolved AFTER (surviving ref-op). ----
        RecipeOpStream refAfter = after;
        AddRevolvedProfileOp latHe;
        latHe.name = "col"; latHe.profile = "shaft";
        refAfter.ops.push_back(latHe);
        const auto refAfterResult = exportRecipeStreamDiffToToon(before, refAfter);
        EDI_CHECK(!refAfterResult.ok);
        EDI_CHECK(refAfterResult.message.find("after stream must be resolved") != std::string::npos);

        // ---- Case 7: identical streams → empty delta (no field lines). ----
        const auto diffSame = exportRecipeStreamDiffToToon(before, before);
        EDI_CHECK(diffSame.ok && diffSame.value.has_value());
        // Only the kind/title header lines; no op.* or recipe.* field lines.
        const std::string &sameText = *diffSame.value;
        EDI_CHECK(sameText.find("kind: recipe-diff\n") != std::string::npos);
        EDI_CHECK(sameText.find("op.") == std::string::npos);
        EDI_CHECK(sameText.find("recipe.") == std::string::npos);
    }

    // ---- P5: prism_sweep_corner_too_sharp — validate guard on the sweep path
    // of an AddPrism (the lowered sweep carrier). The bisector miter frame works
    // well for turn angles up to ~151°; beyond that the miter_scale = 2/|b|
    // exceeds 4.0 (the SVG stroke-miterlimit=4 default) and the cross-section
    // spike becomes a rendering defect. The guard uses the SAME |b| = |t_in+t_out|
    // quantity that _swept_prism_world computes, so the validate threshold and
    // the Python clamp (min(2/bl, 10)) are consistent.
    //
    // Geometry of |b|: for a turn of α (= the exterior angle, measured from
    // the straight-ahead direction), t_in+t_out has magnitude 2·cos(α/2).
    // At α=0 (straight): |b|=2, miter_scale=1. At α=90°: |b|=√2≈1.41.
    // At α=151°: |b|≈0.5, miter_scale=4 — the threshold.
    // At α=180° (U-turn): |b|=0, scale→∞ — the hard failure.
    {
        // Shared helper: does a validation report carry a specific finding code?
        const auto sawCode = [](const OpValidationReport &r, const char *code) {
            for (const OpFinding &f : r.findings) {
                if (f.code == code) return true;
            }
            return false;
        };

        // A minimal, valid footprint for all path tests below.
        const std::vector<PrismPoint> tri = {{0.0, 0.0}, {1.0, 0.0}, {0.5, 1.0}};

        // ---- Near-hairpin path: almost a U-turn, well above the 151° threshold.
        // path (0,0) → (1,0) → (0,0.01):
        //   t_in = (1,0),  t_out ≈ (-0.9999, 0.01) (almost left)
        //   b = t_in+t_out ≈ (0.0001, 0.01),  |b| ≈ 0.01 << 0.5 → fires. ----
        AddPrismOp hairpin;
        hairpin.name = "hairpin";
        hairpin.footprint = tri;
        hairpin.path = {{0.0, 0.0}, {1.0, 0.0}, {0.0, 0.01}}; // ≈180° turn
        EDI_CHECK(sawCode(validateRecipeOps({RecipeOp{hairpin}}), "prism_sweep_corner_too_sharp"));
        // The finding should be an Error, not just a warning.
        for (const OpFinding &f : validateRecipeOps({RecipeOp{hairpin}}).findings) {
            if (f.code == "prism_sweep_corner_too_sharp") {
                EDI_CHECK(f.severity == OpFinding::Severity::Error);
            }
        }

        // ---- Gentle corner: 90° (the swept_profile sample's corner at (4,0)).
        // t_in=(1,0), t_out=(0,1) → b=(1,1), |b|=√2≈1.414 >> 0.5 → passes. ----
        AddPrismOp right90;
        right90.name = "right90";
        right90.footprint = tri;
        right90.path = {{0.0, 0.0}, {4.0, 0.0}, {4.0, 3.0}}; // the swept_profile path
        EDI_CHECK(!sawCode(validateRecipeOps({RecipeOp{right90}}), "prism_sweep_corner_too_sharp"));
        EDI_CHECK(validateRecipeOps({RecipeOp{right90}}).ok);

        // ---- Two-point path (no interior point): can't fire, never enters the
        // loop. A straight two-segment extrude should validate clean. ----
        AddPrismOp straight;
        straight.name = "straight";
        straight.footprint = tri;
        straight.path = {{0.0, 0.0}, {5.0, 0.0}}; // single segment, no interior point
        EDI_CHECK(!sawCode(validateRecipeOps({RecipeOp{straight}}), "prism_sweep_corner_too_sharp"));

        // ---- Empty path (a straight extrude without a swept path): the guard
        // is only checked when path.size() >= 3, so an empty path does not fire. ----
        AddPrismOp noPath;
        noPath.name = "no_path";
        noPath.footprint = tri;
        noPath.height = 2.0; // empty path → needs a height
        EDI_CHECK(!sawCode(validateRecipeOps({RecipeOp{noPath}}), "prism_sweep_corner_too_sharp"));
    }

    return 0;
}
