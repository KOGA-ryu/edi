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

#include <cassert>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <random>
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
    assert(in.is_open());
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
        assert(compiledWritten.ok);
        assert(compiledWritten.text == slurp(EDI_SAMPLES_DIR "/doric_column/doric_column_ops_compiled.toml"));

        // A failing sequence surfaces the term compiler's own message.
        RecipeOpStream broken = doric;
        auto *badMoulding = std::get_if<AddProfileMouldingOp>(&broken.ops[2]);
        badMoulding->sequence[0].startRadius.reset();
        const RecipeCompileResult refused = compileRecipeOps(broken.ops);
        assert(!refused.ok);
        assert(refused.message == "base.torus_scotia_moulding first segment needs start_radius.");
        assert(refused.ops.empty());

        // An unresolved lathe reference cannot compile: compile has no
        // drawing to read, and guessing points is the forbidden move.
        AddRevolvedProfileOp unresolved;
        unresolved.name = "shaft.turned";
        unresolved.profile = "shaft";
        const RecipeCompileResult lathe = compileRecipeOps({RecipeOp{unresolved}});
        assert(!lathe.ok);
        assert(lathe.message == "AddRevolvedProfile must be resolved before compiling: shaft.turned");
        assert(lathe.ops.empty());
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
        assert(has("ring_overhang_alias"));
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
        assert(!report.ok);
        const auto count = [&report](const char *code) {
            int hits = 0;
            for (const OpFinding &finding : report.findings) {
                if (finding.code == code) {
                    ++hits;
                }
            }
            return hits;
        };
        assert(count("bad_sphere_radius") == 1);
        assert(count("low_sphere_vertices") == 1);
        assert(count("negative_moulding_base_z") == 1);
        assert(count("low_moulding_vertices") == 1);
        assert(count("bad_moulding_radius") == 1);
        assert(count("moulding_profile_not_monotonic") == 2); // one PER kink (v0 behavior)
        assert(count("short_moulding_profile") == 1);
        assert(count("bad_moulding_z") == 1);
        assert(count("negative_profile_moulding_base_z") == 1);
        assert(count("low_profile_moulding_vertices") == 1);
        assert(count("empty_profile_moulding_sequence") == 1);
        assert(count("unknown_profile_term") == 1);
        assert(count("bad_profile_segment_height") == 1);
        assert(count("missing_profile_start_radius") == 1);
        assert(count("bad_profile_segment_radius") == 2); // start AND end halves
        assert(count("bad_profile_segment_steps") == 1);
        assert(count("low_flute_count") == 1);
        assert(count("bad_flute_depth") == 1);
        // == 1, not 2: explicitOk's 0.95 ratio must NOT fire — the explicit
        // cutter pair makes the ratio lint irrelevant (R1-B04b decision 5).
        assert(count("odd_flute_width_ratio") == 1);
        assert(count("bad_cutter_radius") == 1);
        assert(count("bad_at_radius") == 1);
        assert(count("missing_profile_reference") == 1);
        assert(count("negative_revolved_profile_base_z") == 1);
        assert(count("low_revolved_profile_vertices") == 1);
        // numberKeyText formatting, not std::to_string's "0.950000".
        bool sawRatioMessage = false;
        for (const OpFinding &finding : report.findings) {
            if (finding.code == "odd_flute_width_ratio") {
                assert(finding.message.find("0.95") != std::string::npos);
                assert(finding.message.find("0.950000") == std::string::npos);
                sawRatioMessage = true;
            }
        }
        assert(sawRatioMessage);
        assert(count("flute_target_not_vertical") == 1);
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
        assert(narrow.size() == 2);
        assert(narrow[0].code == "base_not_wider_than_shaft");
        assert(narrow[1].code == "missing_capital");

        AddBoxOp wideRect = narrowPlinth;
        wideRect.width = 12.0; // max(12,4) = 12 > 10 — but min would be 4: kills max->min
        const std::vector<OpFinding> wide = lintColumnConventions({RecipeOp(wideRect), RecipeOp(shaft)});
        assert(wide.size() == 1);
        assert(wide[0].code == "missing_capital");

        AddBoxOp boundary = narrowPlinth;
        boundary.width = 10.0;
        boundary.depth = 10.0; // exactly shaft diameter: <= fires
        const std::vector<OpFinding> atBoundary = lintColumnConventions({RecipeOp(boundary), RecipeOp(shaft)});
        assert(atBoundary.size() == 2);
        assert(atBoundary[0].code == "base_not_wider_than_shaft");

        const std::vector<OpFinding> bare = lintColumnConventions({RecipeOp(shaft)});
        assert(bare.size() == 1); // no plinth box: only the capital warning
        assert(bare[0].code == "missing_capital");
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
        assert(written.ok);
        const OpStreamParseResult reloaded = recipeOpsFromToml(written.text, "zoo");
        assert(reloaded.ok);
        assert(reloaded.stream.ops.size() == 5);
        const auto *sphere = std::get_if<AddSphereOp>(&reloaded.stream.ops[0]);
        assert(sphere != nullptr && sphere->name == "probe.finial" && near(sphere->radius, 1.5)
               && near(sphere->x, 0.5) && near(sphere->y, -0.25)
               && sphere->vertices == 16 && sphere->material == "marble");
        const auto *ring = std::get_if<AddRingOp>(&reloaded.stream.ops[1]);
        assert(ring != nullptr && near(ring->tubeHeight, 0.5) && near(ring->overhang, 0.25));
        const auto *label = std::get_if<AddLabelOp>(&reloaded.stream.ops[2]);
        assert(label != nullptr && label->text == "north face"
               && near(label->x, 1.0) && near(label->y, 2.0) && near(label->z, 3.0));
        const auto *mouldingZoo = std::get_if<AddMouldingOp>(&reloaded.stream.ops[3]);
        assert(mouldingZoo != nullptr && mouldingZoo->profile.size() == 2
               && mouldingZoo->profile[1].term == "fillet_01"
               && near(mouldingZoo->profile[1].z, 0.5) && near(mouldingZoo->profile[1].radius, 2.5));
        const auto *beamBack = std::get_if<AddCylinderOp>(&reloaded.stream.ops[4]);
        assert(beamBack != nullptr && beamBack->axis == Axis::X && beamBack->zMode == ZMode::Base
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
        assert(trickyWritten.ok);
        // DEL must appear ESCAPED in the written text: our own reader would
        // happily round-trip a raw 0x7F, but tomllib (the python half of
        // the pipeline) refuses it — the escape is for the OTHER reader.
        assert(trickyWritten.text.find("\\u007F") != std::string::npos);
        const OpStreamParseResult trickyBack = recipeOpsFromToml(trickyWritten.text, "tricky");
        assert(trickyBack.ok);
        const auto *gnarlyBack = std::get_if<AddLabelOp>(&trickyBack.stream.ops[0]);
        assert(gnarlyBack != nullptr && gnarlyBack->text == gnarly.text);
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
        assert(bareLathe.ok);
        const auto *bareTurned = std::get_if<AddRevolvedProfileOp>(&bareLathe.stream.ops[0]);
        assert(bareTurned != nullptr);
        assert(bareTurned->vertices == 96 && bareTurned->material == "stone"
               && bareTurned->x == 0.0 && bareTurned->y == 0.0);

        const OpStreamParseResult minimal = recipeOpsFromToml(
            "op.0.type = \"AddCylinder\"\n"
            "op.0.name = \"bare.drum\"\n"
            "op.0.radius = \"1\"\n"
            "op.0.height = \"2\"\n"
            "op.0.z = \"0\"\n", "minimal");
        assert(minimal.ok);
        const auto *drum = std::get_if<AddCylinderOp>(&minimal.stream.ops[0]);
        assert(drum != nullptr);
        assert(drum->x == 0.0 && drum->y == 0.0 && drum->vertices == 96
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
        assert(!oversized.ok);
        assert(oversized.message.find("op.0.vertices: not an integer") != std::string::npos);
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
        assert(boundWritten.ok);
        // The binding keys stand in for the literal; the bare key must NOT
        // appear beside them (the reader refuses that file as ambiguous).
        assert(boundWritten.text.find("op.0.width.object = \"plinth_face\"") != std::string::npos);
        assert(boundWritten.text.find("op.0.width.field = \"width\"") != std::string::npos);
        assert(boundWritten.text.find("op.0.width = ") == std::string::npos);
        assert(boundWritten.text.find("op.1.radius.object = \"shaft_top\"") != std::string::npos);
        assert(boundWritten.text.find("op.1.radius = ") == std::string::npos);
        assert(boundWritten.text.find("op.2.width_ratio.object = \"flute_gauge\"") != std::string::npos);
        assert(boundWritten.text.find("op.2.width_ratio = ") == std::string::npos);

        const OpStreamParseResult boundBack = recipeOpsFromToml(boundWritten.text, "bind.zoo");
        assert(boundBack.ok);
        assert(boundBack.stream.bindings.size() == bound.bindings.size());
        for (const RecipeFieldBinding &binding : bound.bindings) {
            bool found = false;
            for (const RecipeFieldBinding &loaded : boundBack.stream.bindings) {
                if (loaded.opIndex == binding.opIndex && loaded.fieldKey == binding.fieldKey
                    && loaded.objectId == binding.objectId && loaded.field == binding.field) {
                    found = true;
                    break;
                }
            }
            assert(found);
        }
        // A bound field carries the STRUCT DEFAULT until resolve (B03): the
        // unresolved number must be the inert default, never file garbage.
        const auto *plinthBack = std::get_if<AddBoxOp>(&boundBack.stream.ops[0]);
        assert(plinthBack != nullptr && plinthBack->width == 0.0);
        assert(plinthBack->depth == 3.0); // unbound literals load normally
        const auto *flutesBack = std::get_if<CutFlutesOp>(&boundBack.stream.ops[2]);
        assert(flutesBack != nullptr && flutesBack->widthRatio == 0.28); // spec default
        // The lathe reference round-trips: the profile id is a plain string
        // field (the OTHER crown jewel — never a binding).
        const auto *turnedBack = std::get_if<AddRevolvedProfileOp>(&boundBack.stream.ops[8]);
        assert(turnedBack != nullptr && turnedBack->profile == "shaft_profile"
               && turnedBack->vertices == 64 && turnedBack->baseZ == 0.0);

        // Refusals, A's loader order: half a binding, then literal clash,
        // then empty names.
        const OpStreamParseResult half = recipeOpsFromToml(
            "op.0.type = \"AddBox\"\n"
            "op.0.name = \"b\"\n"
            "op.0.width.object = \"plank\"\n", "bad");
        assert(!half.ok);
        assert(half.message == "op.0.width: a measurement binding needs both .object and .field");

        const OpStreamParseResult clash = recipeOpsFromToml(
            "op.0.type = \"AddBox\"\n"
            "op.0.name = \"b\"\n"
            "op.0.width = \"1\"\n"
            "op.0.width.object = \"plank\"\n"
            "op.0.width.field = \"width\"\n", "bad");
        assert(!clash.ok);
        assert(clash.message
               == "op.0.width: has both a literal and a measurement binding (.object/.field)");

        const OpStreamParseResult unnamed = recipeOpsFromToml(
            "op.0.type = \"AddBox\"\n"
            "op.0.name = \"b\"\n"
            "op.0.width.object = \"\"\n"
            "op.0.width.field = \"width\"\n", "bad");
        assert(!unnamed.ok);
        assert(unnamed.message == "op.0.width: a binding names an object and a field");

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
        assert(!intBinding.ok);
        assert(intBinding.message == "unknown recipe key: op.0.vertices.field");

        // The WRITER refuses bogus bindings too — a stream must not
        // serialize a file the reader would bounce. (A refusal message can
        // only be non-empty on the !ok path, so asserting the message IS
        // asserting the refusal.)
        RecipeOpStream bogus = bound;
        bogus.bindings = {{9, "width", "plank", "width"}};
        assert(recipeOpsToToml(bogus).message == "binding for op.9.width: no such op");
        bogus.bindings = {{1, "vertices", "gauge", "width"}};
        assert(recipeOpsToToml(bogus).message == "op.1.vertices: not a bindable field");
        bogus.bindings = {{0, "width", "", "width"}};
        assert(recipeOpsToToml(bogus).message == "op.0.width: a binding names an object and a field");
        bogus.bindings = {{0, "width", "a", "width"}, {0, "width", "b", "width"}};
        assert(recipeOpsToToml(bogus).message == "op.0.width: bound more than once");

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
                assert(opFieldBindable(pin.op, key));
                RecipeOp writable = pin.op;
                assert(setOpFieldValue(writable, key, 2.5));
            }
            for (const char *key : pin.notBindable) {
                assert(!opFieldBindable(pin.op, key));
                RecipeOp writable = pin.op;
                assert(!setOpFieldValue(writable, key, 2.5));
            }
        }
        // And the pointers land in the right members, not just somewhere.
        RecipeOp probe = AddRingOp{};
        assert(setOpFieldValue(probe, "tube_height", 2.5));
        assert(std::get_if<AddRingOp>(&probe)->tubeHeight == 2.5);
        assert(std::get_if<AddRingOp>(&probe)->radius == 0.0);

        // opFields lists the same fields the registry binds, with their live
        // values — what the human inspector reads to build its spinboxes. The
        // listed keys round-trip through opFieldBindable/setOpFieldValue, so the
        // read, the write, and the predicate can never drift apart.
        RecipeOp box = AddBoxOp{};
        std::get_if<AddBoxOp>(&box)->width = 3.0;
        std::get_if<AddBoxOp>(&box)->height = 5.0;
        const std::vector<RecipeOpField> boxFields = opFields(box);
        assert(boxFields.size() == 6); // width, depth, height, z, x, y
        assert(boxFields[0].key == "width" && boxFields[0].value == 3.0);
        assert(boxFields[2].key == "height" && boxFields[2].value == 5.0);
        for (const RecipeOpField &field : boxFields) {
            assert(opFieldBindable(box, field.key));
            assert(setOpFieldValue(box, field.key, 1.5));
        }
        assert(opFields(RecipeOp{AddSphereOp{}}).size() == 4); // radius, z, x, y
        assert(opFields(RecipeOp{AddCylinderOp{}})[5].key == "entasis_ratio");

        // The step palette: every offered type makes a valid, named, unit-sized
        // op the inspector can immediately tune; an off-palette type makes
        // nothing (mouldings/lathe/flutes need more than a click to be valid).
        const std::vector<std::string> &palette = recipePaletteOpTypes();
        assert(palette.size() == 4); // box, cylinder, sphere, ring
        for (const std::string &type : palette) {
            const std::optional<RecipeOp> made = makeRecipeOp(type, "step_test");
            assert(made.has_value());
            assert(recipeOpTypeName(*made) == type);
        }
        const std::optional<RecipeOp> newBox = makeRecipeOp("AddBox", "b0");
        assert(newBox.has_value());
        assert(std::get_if<AddBoxOp>(&*newBox)->width == 1.0);
        assert(std::get_if<AddBoxOp>(&*newBox)->name == "b0");
        assert(!makeRecipeOp("AddProfileMoulding", "x").has_value());

        // Remove/reorder keep the binding table (bindings are by op INDEX) sane.
        RecipeOpStream s;
        s.ops = {makeRecipeOp("AddBox", "a").value(),
                 makeRecipeOp("AddCylinder", "b").value(),
                 makeRecipeOp("AddSphere", "c").value()};
        s.bindings = {{0, "width", "obj0", "width"}, {2, "radius", "obj2", "radius"}};
        removeRecipeOp(s, 0); // op a gone; the sphere's binding slides 2 -> 1
        assert(s.ops.size() == 2);
        assert(recipeOpTypeName(s.ops[0]) == std::string("AddCylinder"));
        assert(s.bindings.size() == 1 && s.bindings[0].opIndex == 1);
        moveRecipeOp(s, 1, 0); // sphere to the front; its binding follows to 0
        assert(recipeOpTypeName(s.ops[0]) == std::string("AddSphere"));
        assert(s.bindings[0].opIndex == 0);
        removeRecipeOp(s, 99); // out of range: no-op
        assert(s.ops.size() == 2);

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
        assert(sawMaterial && sawZMode && sawName && sawVertices);
        assert(setOpScalar(cyl, "material", std::string("marble")));
        assert(std::get_if<AddCylinderOp>(&cyl)->material == "marble");
        assert(setOpScalar(cyl, "z_mode", std::string("base")));
        assert(std::get_if<AddCylinderOp>(&cyl)->zMode == ZMode::Base);
        assert(setOpScalar(cyl, "vertices", 48));
        assert(std::get_if<AddCylinderOp>(&cyl)->vertices == 48);
        assert(setOpScalar(cyl, "entasis", true));
        assert(std::get_if<AddCylinderOp>(&cyl)->entasis);
        assert(setOpScalar(cyl, "radius", 2.0)); // a double still routes through the registry
        assert(std::get_if<AddCylinderOp>(&cyl)->radius == 2.0);
        assert(!setOpScalar(cyl, "nonsense", 1.0)); // unknown key

        // Binding picker core: add / find / replace / clear a measurement binding.
        RecipeOpStream bstream;
        bstream.ops = {makeRecipeOp("AddBox", "b").value()};
        assert(addRecipeBinding(bstream, 0, "width", "plank_1", "length"));
        const RecipeFieldBinding *found = findRecipeBinding(bstream, 0, "width");
        assert(found != nullptr && found->objectId == "plank_1" && found->field == "length");
        assert(addRecipeBinding(bstream, 0, "width", "plank_2", "width")); // re-bind REPLACES
        assert(bstream.bindings.size() == 1);
        assert(findRecipeBinding(bstream, 0, "width")->objectId == "plank_2");
        assert(!addRecipeBinding(bstream, 0, "name", "x", "width")); // not a bindable double
        clearRecipeBinding(bstream, 0, "width");
        assert(bstream.bindings.empty() && findRecipeBinding(bstream, 0, "width") == nullptr);
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
        assert(written.ok);
        // The pair is emitted; width_ratio is NOT (a file showing both is the
        // lie the reader refuses).
        assert(written.text.find("op.1.cutter_radius = \"0.16\"") != std::string::npos);
        assert(written.text.find("op.1.at_radius = \"1.056\"") != std::string::npos);
        assert(written.text.find("op.1.width_ratio") == std::string::npos);

        const OpStreamParseResult back = recipeOpsFromToml(written.text, "cutter.zoo");
        assert(back.ok);
        const auto *flutesBack = std::get_if<CutFlutesOp>(&back.stream.ops[1]);
        assert(flutesBack != nullptr);
        assert(flutesBack->cutterRadius.has_value() && near(*flutesBack->cutterRadius, 0.16));
        assert(flutesBack->atRadius.has_value() && near(*flutesBack->atRadius, 1.056));
        assert(flutesBack->widthRatio == 0.28); // the inert default; the file carried none

        // Half a pair refuses, by name.
        const OpStreamParseResult halfPair = recipeOpsFromToml(
            "op.0.type = \"CutFlutes\"\n"
            "op.0.target = \"shaft\"\n"
            "op.0.count = \"20\"\n"
            "op.0.depth = \"0.12\"\n"
            "op.0.cutter_radius = \"0.16\"\n", "bad");
        assert(!halfPair.ok);
        assert(halfPair.message == "op.0: a cutter needs both .cutter_radius and .at_radius");

        // The pair beside a width_ratio refuses, by name (both sources).
        const OpStreamParseResult pairAndRatio = recipeOpsFromToml(
            "op.0.type = \"CutFlutes\"\n"
            "op.0.target = \"shaft\"\n"
            "op.0.count = \"20\"\n"
            "op.0.depth = \"0.12\"\n"
            "op.0.cutter_radius = \"0.16\"\n"
            "op.0.at_radius = \"1.056\"\n"
            "op.0.width_ratio = \"0.34\"\n", "bad");
        assert(!pairAndRatio.ok);
        assert(pairAndRatio.message
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
        assert(!halfWritten.ok);
        assert(halfWritten.message == "op.0: a cutter needs both .cutter_radius and .at_radius");
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
        assert(w.ok);
        assert(w.text.find("op.0.type = \"Script\"") != std::string::npos);
        assert(w.text.find("op.0.script = \"twisted_column\"") != std::string::npos);
        assert(w.text.find("op.0.name = \"twist.core\"") != std::string::npos);
        assert(w.text.find("op.0.radius = \"1.25\"") != std::string::npos);
        assert(w.text.find("op.0.sides = \"6\"") != std::string::npos);
        assert(w.text.find("op.0.material = \"marble\"") != std::string::npos);

        const OpStreamParseResult rp = recipeOpsFromToml(w.text, "craft.zoo");
        assert(rp.ok && rp.stream.ops.size() == 1);
        const auto *back = std::get_if<ScriptOp>(&rp.stream.ops[0]);
        assert(back != nullptr && back->scriptId == "twisted_column" && back->name == "twist.core");
        assert(near(back->x, 0.5) && near(back->y, -0.25) && near(back->z, 4.0));
        assert(back->params.size() == 3);
        const auto pval = [&](const char *key) -> std::string {
            for (const ScriptParam &p : back->params) {
                if (p.key == key) return p.value;
            }
            return "<none>";
        };
        assert(pval("radius") == "1.25" && pval("sides") == "6" && pval("material") == "marble");
        // The canonical writer is idempotent (the lab re-serializes on every edit).
        assert(recipeOpsToToml(rp.stream).text == w.text);

        // Minimal hand file: only type + script. Name falls back to the id (the
        // python default), x/y/z default to 0, the bag is empty.
        const OpStreamParseResult mini = recipeOpsFromToml(
            "op.0.type = \"Script\"\n"
            "op.0.script = \"twisted_column\"\n", "mini");
        assert(mini.ok);
        const auto *m = std::get_if<ScriptOp>(&mini.stream.ops[0]);
        assert(m != nullptr && m->scriptId == "twisted_column" && m->name == "twisted_column");
        assert(m->x == 0.0 && m->y == 0.0 && m->z == 0.0 && m->params.empty());

        // A missing craftsman id is refused at READ, by name (requireText).
        const OpStreamParseResult noScript = recipeOpsFromToml(
            "op.0.type = \"Script\"\n"
            "op.0.name = \"x\"\n", "bad");
        assert(!noScript.ok);
        assert(noScript.message == "missing required key: op.0.script");

        // The bag accepts ANY key — there is no schema to audit it against here
        // (the craftsman's MANIFEST owns that), so the global unknown-key audit
        // does not fire on a Script op's params.
        const OpStreamParseResult wild = recipeOpsFromToml(
            "op.0.type = \"Script\"\n"
            "op.0.script = \"twisted_column\"\n"
            "op.0.name = \"w\"\n"
            "op.0.whatever = \"42\"\n"
            "op.0.another_param = \"hi\"\n", "wild");
        assert(wild.ok);
        const auto *wl = std::get_if<ScriptOp>(&wild.stream.ops[0]);
        assert(wl != nullptr && wl->params.size() == 2);

        // The param-key contract is ENFORCED at write, read, and validate (not
        // just asserted in a comment): a key that collides with a built-in
        // would overwrite it in the flat map; a dotted/spaced key would emit an
        // unreadable line or nest under the python half's tomllib. All three
        // gates refuse the same keys, by name.
        {
            // recipeScriptParamKeyProblem: the shared predicate.
            assert(recipeScriptParamKeyProblem("sides").empty());
            assert(recipeScriptParamKeyProblem("my-param_2").empty());
            assert(!recipeScriptParamKeyProblem("name").empty());  // built-in
            assert(!recipeScriptParamKeyProblem("a.b").empty());   // nests under tomllib
            assert(!recipeScriptParamKeyProblem("has space").empty());
            assert(!recipeScriptParamKeyProblem("").empty());

            // WRITE (B02): a colliding key refuses by name.
            RecipeOpStream collide;
            ScriptOp bad;
            bad.scriptId = "c";
            bad.name = "bad";
            bad.params = {{"name", "oops"}};
            collide.ops.push_back(bad);
            const OpStreamTextResult cw = recipeOpsToToml(collide);
            assert(!cw.ok);
            assert(cw.message == "op.0: param key 'name' collides with the built-in field 'name'");

            // WRITE: a dotted key refuses by name.
            RecipeOpStream dotted;
            ScriptOp dot;
            dot.scriptId = "c";
            dot.name = "d";
            dot.params = {{"a.b", "v"}};
            dotted.ops.push_back(dot);
            const OpStreamTextResult dw = recipeOpsToToml(dotted);
            assert(!dw.ok);
            assert(dw.message == "op.0: param key 'a.b' must be letters, digits, '_' or '-'");

            // READ: the strict reader refuses a dotted param key (it would round
            // -trip apart under tomllib). Reserved keys can't reach here — the
            // position readers consume them first — so the dotted case is the
            // reachable one.
            const OpStreamParseResult dottedRead = recipeOpsFromToml(
                "op.0.type = \"Script\"\n"
                "op.0.script = \"c\"\n"
                "op.0.a.b = \"v\"\n", "dotted");
            assert(!dottedRead.ok);
            assert(dottedRead.message == "op.0: param key 'a.b' must be letters, digits, '_' or '-'");

            // VALIDATE: a bad key surfaces as a named finding.
            ScriptOp badVal;
            badVal.scriptId = "c";
            badVal.name = "v";
            badVal.params = {{"bad key", "1"}};
            const OpValidationReport vr = validateRecipeOps({RecipeOp{badVal}});
            assert(!vr.ok);
            bool sawBadKey = false;
            for (const OpFinding &f : vr.findings) {
                sawBadKey = sawBadKey || f.code == "bad_param_key";
            }
            assert(sawBadKey);

            // A hyphenated key is legal and round-trips.
            RecipeOpStream okStream;
            ScriptOp okScript;
            okScript.scriptId = "c";
            okScript.name = "ok";
            okScript.params = {{"my-param", "7"}};
            okStream.ops.push_back(okScript);
            const OpStreamTextResult okw = recipeOpsToToml(okStream);
            assert(okw.ok);
            const OpStreamParseResult okr = recipeOpsFromToml(okw.text, "ok");
            assert(okr.ok);
            const auto *okBack = std::get_if<ScriptOp>(&okr.stream.ops[0]);
            assert(okBack != nullptr && okBack->params.size() == 1
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
        assert(bw.ok);
        assert(bw.text.find("op.0.x.object = \"anchor\"") != std::string::npos);
        assert(bw.text.find("op.0.x.field = \"width\"") != std::string::npos);
        assert(bw.text.find("op.0.x = ") == std::string::npos); // no literal beside the binding
        const OpStreamParseResult br = recipeOpsFromToml(bw.text, "bound.script");
        assert(br.ok && br.stream.bindings.size() == 1);
        assert(br.stream.bindings[0].opIndex == 0 && br.stream.bindings[0].fieldKey == "x"
               && br.stream.bindings[0].objectId == "anchor" && br.stream.bindings[0].field == "width");
        const auto *brBack = std::get_if<ScriptOp>(&br.stream.ops[0]);
        assert(brBack != nullptr && brBack->params.size() == 1
               && brBack->params[0].key == "sides" && brBack->params[0].value == "6");

        // compile leaves a Script op untouched (no lowering needed) — it reaches
        // the craftsmen library as-is.
        const RecipeCompileResult comp = compileRecipeOps(zoo.ops);
        assert(comp.ok && comp.ops.size() == 1
               && recipeOpTypeName(comp.ops[0]) == std::string("Script"));

        // Validate: the one C++-side invariant is a craftsman to dispatch to.
        ScriptOp ghost;
        ghost.name = "ghost"; // scriptId left empty
        const OpValidationReport noId = validateRecipeOps({RecipeOp{ghost}});
        assert(!noId.ok);
        bool sawMissing = false;
        for (const OpFinding &f : noId.findings) {
            sawMissing = sawMissing || f.code == "missing_script_reference";
        }
        assert(sawMissing);
        const OpValidationReport good = validateRecipeOps({RecipeOp{tw}});
        assert(good.ok);
        // A Script name participates in duplicate detection (opName sees it).
        AddBoxOp clash;
        clash.name = "twist.core";
        clash.width = clash.depth = clash.height = 1.0;
        const OpValidationReport dup = validateRecipeOps({RecipeOp{tw}, RecipeOp{clash}});
        bool sawDup = false;
        for (const OpFinding &f : dup.findings) {
            sawDup = sawDup || f.code == "duplicate_name";
        }
        assert(sawDup);

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
        assert(sawX && sawName && sawScriptRO && sawSides);
        assert(setOpScalar(scOp, "name", std::string("renamed")));
        assert(std::get_if<ScriptOp>(&scOp)->name == "renamed");
        assert(setOpScalar(scOp, "sides", std::string("8"))); // a param
        assert(std::get_if<ScriptOp>(&scOp)->params[1].value == "8"); // {radius, sides, material}
        assert(setOpScalar(scOp, "x", 5.0)); // a double still routes through the registry
        assert(near(std::get_if<ScriptOp>(&scOp)->x, 5.0));
        assert(!setOpScalar(scOp, "script", std::string("other"))); // read-only id
        assert(std::get_if<ScriptOp>(&scOp)->scriptId == "twisted_column");
        assert(!setOpScalar(scOp, "nope", std::string("x"))); // unknown key
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
        assert(written.ok);
        // Writer key shape — a drift that broke BL-04's parse_ops would fail here.
        assert(written.text.find("op.0.type = \"AddExtrudedProfile\"") != std::string::npos);
        assert(written.text.find("op.0.profile = \"panel_outline\"") != std::string::npos);
        assert(written.text.find("op.0.height = \"2.5\"") != std::string::npos);
        assert(written.text.find("op.0.base_z = \"1\"") != std::string::npos);
        assert(written.text.find("op.0.material = \"marble\"") != std::string::npos);

        const OpStreamParseResult back = recipeOpsFromToml(written.text, "extrude.zoo");
        assert(back.ok && back.stream.ops.size() == 1);
        const auto *prismBack = std::get_if<AddExtrudedProfileOp>(&back.stream.ops[0]);
        assert(prismBack != nullptr && prismBack->name == "wall.panel"
               && prismBack->profile == "panel_outline" && near(prismBack->height, 2.5)
               && near(prismBack->baseZ, 1.0) && near(prismBack->x, 0.5)
               && near(prismBack->y, -0.25) && prismBack->material == "marble");
        assert(recipeOpsToToml(back.stream).text == written.text); // idempotent

        // Reader defaults from a minimal hand file: x/y -> 0, material -> stone.
        const OpStreamParseResult bare = recipeOpsFromToml(
            "op.0.type = \"AddExtrudedProfile\"\n"
            "op.0.name = \"bare.prism\"\n"
            "op.0.profile = \"outline\"\n"
            "op.0.height = \"3\"\n"
            "op.0.base_z = \"0\"\n", "minimal");
        assert(bare.ok);
        const auto *bareBack = std::get_if<AddExtrudedProfileOp>(&bare.stream.ops[0]);
        assert(bareBack != nullptr && bareBack->x == 0.0 && bareBack->y == 0.0
               && bareBack->material == "stone");

        // Refusal #1: an unlowered extrude cannot compile, refused BY NAME like
        // the lathe; recipeOpsResolved agrees it is not safe downstream.
        AddExtrudedProfileOp unlowered;
        unlowered.name = "shaft.extruded";
        unlowered.profile = "outline";
        unlowered.height = 4.0;
        const RecipeCompileResult refused = compileRecipeOps({RecipeOp{unlowered}});
        assert(!refused.ok);
        assert(refused.message
               == "AddExtrudedProfile must be resolved before compiling: shaft.extruded");
        assert(refused.ops.empty());
        RecipeOpStream survives;
        survives.ops = {RecipeOp{unlowered}};
        assert(!recipeOpsResolved(survives));

        // Refusal #2: a zero height fails validate by its finding name; a valid
        // op passes; a NEGATIVE height is deliberately allowed (BL-05 push/pull).
        AddExtrudedProfileOp flat = unlowered;
        flat.height = 0.0;
        const OpValidationReport flatReport = validateRecipeOps({RecipeOp{flat}});
        assert(!flatReport.ok);
        bool sawZeroHeight = false;
        for (const OpFinding &f : flatReport.findings) {
            sawZeroHeight = sawZeroHeight || f.code == "extruded_profile_zero_height";
        }
        assert(sawZeroHeight);
        assert(validateRecipeOps({RecipeOp{unlowered}}).ok); // height 4.0, valid
        AddExtrudedProfileOp pushPull = unlowered;
        pushPull.height = -1.5; // allowed: not refused here
        const OpValidationReport negReport = validateRecipeOps({RecipeOp{pushPull}});
        for (const OpFinding &f : negReport.findings) {
            assert(f.code != "extruded_profile_zero_height");
        }

        // Bind registry + schema, pinned: height/base_z/x/y bind; profile and
        // material stay literal-only; the inspector exposes name + read-only
        // profile + material.
        RecipeOp bindProbe = RecipeOp{prism};
        for (const char *key : {"height", "base_z", "x", "y"}) {
            assert(opFieldBindable(bindProbe, key));
        }
        for (const char *key : {"profile", "material", "name"}) {
            assert(!opFieldBindable(bindProbe, key));
        }
        bool sawProfileRO = false, sawMaterial = false, sawHeight = false;
        for (const RecipeOpScalar &s : opEditableScalars(bindProbe)) {
            if (s.key == "profile") sawProfileRO = !s.editable && s.text == "panel_outline";
            else if (s.key == "material") sawMaterial = s.kind == RecipeFieldKind::Choice;
            else if (s.key == "height") sawHeight = s.kind == RecipeFieldKind::Number && near(s.number, 2.5);
        }
        assert(sawProfileRO && sawMaterial && sawHeight);
    }

    // ---- BL-05: pin the push/pull AUTHORING SURFACE on AddExtrudedProfile —
    // `height` is a depth verb that needs NO new mechanism: it is a bindable
    // Number opField (the right-click bind affordance's target) AND an editable
    // Number scalar (the field editor renders it), and a write actually REACHES
    // the height member (so the bind/measure path lands on the right double).
    {
        RecipeOp op = RecipeOp{AddExtrudedProfileOp{}};
        // (a) the bind affordance target: height is bindable.
        assert(opFieldBindable(op, "height"));
        // (b) a write reaches the height MEMBER (the gap-check: the binding/
        //     measure path must land on height, not silently miss it).
        assert(setOpFieldValue(op, "height", 7.5));
        assert(near(std::get_if<AddExtrudedProfileOp>(&op)->height, 7.5));
        // (c) the field editor sees an editable Number for height.
        bool heightIsEditableNumber = false;
        for (const RecipeOpScalar &s : opEditableScalars(op)) {
            if (s.key == "height") {
                heightIsEditableNumber = s.kind == RecipeFieldKind::Number && near(s.number, 7.5);
            }
        }
        assert(heightIsEditableNumber);
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
        assert(written.ok);
        assert(written.text.find("op.0.type = \"AddPrism\"") != std::string::npos);
        assert(written.text.find("op.0.height = \"2\"") != std::string::npos);
        assert(written.text.find("op.0.base_z = \"1\"") != std::string::npos);
        assert(written.text.find("op.0.footprint.0.x = \"1.2\"") != std::string::npos);
        assert(written.text.find("op.0.footprint.2.y = \"2.4\"") != std::string::npos);

        const OpStreamParseResult back = recipeOpsFromToml(written.text, "prism.zoo");
        assert(back.ok && back.stream.ops.size() == 1);
        const auto *prismBack = std::get_if<AddPrismOp>(&back.stream.ops[0]);
        assert(prismBack != nullptr && prismBack->name == "wall.block"
               && near(prismBack->height, 2.0) && near(prismBack->baseZ, 1.0)
               && near(prismBack->x, 0.5) && near(prismBack->y, -0.25)
               && prismBack->material == "marble");
        assert(prismBack->footprint.size() == 4);
        assert(near(prismBack->footprint[0].x, 1.2) && near(prismBack->footprint[0].y, 0.8));
        assert(near(prismBack->footprint[3].x, 1.2) && near(prismBack->footprint[3].y, 2.4));
        assert(recipeOpsToToml(back.stream).text == written.text); // idempotent

        // BUILDABLE carrier: compile passes it through untouched (like
        // AddMoulding), and it does NOT make the stream unresolved.
        const RecipeCompileResult compiled = compileRecipeOps({RecipeOp{prism}});
        assert(compiled.ok && compiled.ops.size() == 1
               && recipeOpTypeName(compiled.ops[0]) == std::string("AddPrism"));
        RecipeOpStream resolvedStream;
        resolvedStream.ops = {RecipeOp{prism}};
        assert(recipeOpsResolved(resolvedStream));
        assert(validateRecipeOps({RecipeOp{prism}}).ok);

        // Validate refusals by name: a degenerate footprint (< 3 points) and a
        // zero/non-finite height. A NEGATIVE height is ALLOWED (BL-05).
        AddPrismOp sliver = prism;
        sliver.footprint = {{0.0, 0.0}, {1.0, 0.0}}; // a line, not an area
        const OpValidationReport sliverReport = validateRecipeOps({RecipeOp{sliver}});
        assert(!sliverReport.ok);
        bool sawDegenerate = false;
        for (const OpFinding &f : sliverReport.findings) {
            sawDegenerate = sawDegenerate || f.code == "prism_degenerate_footprint";
        }
        assert(sawDegenerate);

        AddPrismOp flat = prism;
        flat.height = 0.0;
        const OpValidationReport flatReport = validateRecipeOps({RecipeOp{flat}});
        assert(!flatReport.ok);
        bool sawZeroHeight = false;
        for (const OpFinding &f : flatReport.findings) {
            sawZeroHeight = sawZeroHeight || f.code == "prism_zero_height";
        }
        assert(sawZeroHeight);

        AddPrismOp pushPull = prism;
        pushPull.height = -1.5; // allowed
        const OpValidationReport negReport = validateRecipeOps({RecipeOp{pushPull}});
        for (const OpFinding &f : negReport.findings) {
            assert(f.code != "prism_zero_height");
        }
        assert(negReport.ok);
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
        assert(near(m.sweepDegrees, 360.0)); // struct default
        RecipeOpStream ms;
        ms.ops.push_back(m);
        const OpStreamTextResult mw = recipeOpsToToml(ms);
        assert(mw.ok);
        assert(mw.text.find("op.0.sweep_degrees = \"360\"") != std::string::npos);
        const auto *mBack = std::get_if<AddMouldingOp>(&recipeOpsFromToml(mw.text, "m").stream.ops[0]);
        assert(mBack != nullptr && near(mBack->sweepDegrees, 360.0));

        // A non-default sweep round-trips on the lathe.
        AddRevolvedProfileOp lathe;
        lathe.name = "niche";
        lathe.profile = "arch_profile";
        lathe.sweepDegrees = 180.0;
        RecipeOpStream ls;
        ls.ops.push_back(lathe);
        const OpStreamTextResult lw = recipeOpsToToml(ls);
        assert(lw.ok && lw.text.find("op.0.sweep_degrees = \"180\"") != std::string::npos);
        const auto *lBack = std::get_if<AddRevolvedProfileOp>(&recipeOpsFromToml(lw.text, "l").stream.ops[0]);
        assert(lBack != nullptr && near(lBack->sweepDegrees, 180.0));

        // An absent key parses to the 360 default (the byte-preserving guarantee
        // for any pre-BL-06 hand file).
        const OpStreamParseResult bare = recipeOpsFromToml(
            "op.0.type = \"AddRevolvedProfile\"\n"
            "op.0.name = \"bare\"\n"
            "op.0.profile = \"p\"\n"
            "op.0.base_z = \"0\"\n", "bare");
        assert(bare.ok);
        assert(near(std::get_if<AddRevolvedProfileOp>(&bare.stream.ops[0])->sweepDegrees, 360.0));

        // Validate refuses 0 / negative / > 360 by name, on BOTH ops.
        const auto sawBadSweep = [](const OpValidationReport &r) {
            for (const OpFinding &f : r.findings) {
                if (f.code == "bad_sweep_degrees") return true;
            }
            return false;
        };
        AddMouldingOp zero = m; zero.sweepDegrees = 0.0;
        assert(sawBadSweep(validateRecipeOps({RecipeOp{zero}})));
        AddMouldingOp over = m; over.sweepDegrees = 361.0;
        assert(sawBadSweep(validateRecipeOps({RecipeOp{over}})));
        AddRevolvedProfileOp neg; neg.name = "n"; neg.profile = "p"; neg.sweepDegrees = -90.0;
        assert(sawBadSweep(validateRecipeOps({RecipeOp{neg}})));
        // The boundary 360 is allowed (full revolve); 180 is allowed.
        AddMouldingOp ok360 = m; ok360.sweepDegrees = 360.0;
        assert(!sawBadSweep(validateRecipeOps({RecipeOp{ok360}})));

        // The bind affordance: sweep_degrees is a bindable Number on both ops
        // (a drafted angle could drive it) — consistent with how the reader
        // reads it (bindableNumber) and the inspector surfaces it.
        assert(opFieldBindable(RecipeOp{AddMouldingOp{}}, "sweep_degrees"));
        assert(opFieldBindable(RecipeOp{AddRevolvedProfileOp{}}, "sweep_degrees"));
        RecipeOp probe = RecipeOp{AddMouldingOp{}};
        assert(setOpFieldValue(probe, "sweep_degrees", 90.0));
        assert(near(std::get_if<AddMouldingOp>(&probe)->sweepDegrees, 90.0));
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
        assert(near(m.screwRise, 0.0) && near(m.screwTurns, 1.0)); // struct defaults
        m.screwRise = 2.0;
        m.screwTurns = 4.0;
        RecipeOpStream ms;
        ms.ops.push_back(m);
        const OpStreamTextResult mw = recipeOpsToToml(ms);
        assert(mw.ok);
        assert(mw.text.find("op.0.screw_rise = \"2\"") != std::string::npos);
        assert(mw.text.find("op.0.screw_turns = \"4\"") != std::string::npos);
        const auto *mBack = std::get_if<AddMouldingOp>(&recipeOpsFromToml(mw.text, "m").stream.ops[0]);
        assert(mBack != nullptr && near(mBack->screwRise, 2.0) && near(mBack->screwTurns, 4.0));

        // Absent keys default to 0 / 1 on the lathe (the pre-BL-07 guarantee).
        const OpStreamParseResult bare = recipeOpsFromToml(
            "op.0.type = \"AddRevolvedProfile\"\n"
            "op.0.name = \"bare\"\n"
            "op.0.profile = \"p\"\n"
            "op.0.base_z = \"0\"\n", "bare");
        assert(bare.ok);
        const auto *bareL = std::get_if<AddRevolvedProfileOp>(&bare.stream.ops[0]);
        assert(near(bareL->screwRise, 0.0) && near(bareL->screwTurns, 1.0));

        // Validate: screw_turns <= 0 refused by name on BOTH ops; a NEGATIVE
        // screw_rise (left-hand spiral) is allowed.
        const auto sawBadTurns = [](const OpValidationReport &r) {
            for (const OpFinding &f : r.findings) {
                if (f.code == "bad_screw_turns") return true;
            }
            return false;
        };
        AddMouldingOp zeroTurns = m; zeroTurns.screwTurns = 0.0;
        assert(sawBadTurns(validateRecipeOps({RecipeOp{zeroTurns}})));
        AddRevolvedProfileOp negTurns; negTurns.name = "n"; negTurns.profile = "p";
        negTurns.screwTurns = -1.0;
        assert(sawBadTurns(validateRecipeOps({RecipeOp{negTurns}})));
        AddMouldingOp leftHand = m; leftHand.screwRise = -3.0; // left-hand spiral, allowed
        const OpValidationReport leftReport = validateRecipeOps({RecipeOp{leftHand}});
        assert(leftReport.ok);

        // The bind affordance: both screw params are bindable Numbers on both ops.
        for (const char *key : {"screw_rise", "screw_turns"}) {
            assert(opFieldBindable(RecipeOp{AddMouldingOp{}}, key));
            assert(opFieldBindable(RecipeOp{AddRevolvedProfileOp{}}, key));
        }
        RecipeOp probe = RecipeOp{AddRevolvedProfileOp{}};
        assert(setOpFieldValue(probe, "screw_rise", 5.0));
        assert(near(std::get_if<AddRevolvedProfileOp>(&probe)->screwRise, 5.0));
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
        assert(saved.ok);
        const std::vector<std::string> names = listLibraryRecipes(dir.string());
        assert(names.size() == 1 && names[0] == "pedestal"); // filename = sanitized name
        const OpStreamParseResult loaded = loadLibraryRecipe(dir.string(), "pedestal");
        assert(loaded.ok);
        assert(loaded.stream.ops.size() == 2);
        assert(loaded.stream.id == "lib_pedestal" && loaded.stream.name == "pedestal");
        assert(std::get_if<AddBoxOp>(&loaded.stream.ops[0])->name == "slab");
        assert(loaded.stream.bindings.size() == 1
               && loaded.stream.bindings[0].opIndex == 1
               && loaded.stream.bindings[0].objectId == "gauge");
        // A missing recipe fails by name, not a crash.
        const OpStreamParseResult missing = loadLibraryRecipe(dir.string(), "nope");
        assert(!missing.ok && missing.message.find("recipe not found") != std::string::npos);
        // A missing directory lists empty (not an error).
        assert(listLibraryRecipes((dir / "no_such").string()).empty());
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
        assert(target.ops.size() == 3);
        assert(target.bindings.size() == 2);
        // The target binding is untouched.
        assert(target.bindings[0].opIndex == 0 && target.bindings[0].objectId == "t_obj");
        // The source binding re-offset by M=2: j(0) -> 2.
        assert(target.bindings[1].opIndex == 2 && target.bindings[1].objectId == "s_obj");
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
        assert(target.ops.size() == 3);
        // The target shaft keeps its bare name (never namespaced).
        assert(std::get_if<AddCylinderOp>(&target.ops[0])->name == "shaft");
        // The source ops are namespaced under "capital::".
        assert(std::get_if<AddCylinderOp>(&target.ops[1])->name == "capital::shaft");
        // The source flute's target was rewritten to the NAMESPACED source shaft,
        // not left pointing at the target's bare "shaft".
        assert(std::get_if<CutFlutesOp>(&target.ops[2])->target == "capital::shaft");
        // The merged stream validates: the flute finds its target within the
        // spliced block (validation matches names in order).
        assert(validateRecipeOps(target.ops).ok);
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
        assert(sw.ok);
        assert(sw.text.find("op.0.type = \"AddSweepProfile\"") != std::string::npos);
        assert(sw.text.find("op.0.profile = \"section\"") != std::string::npos);
        assert(sw.text.find("op.0.path = \"run_path\"") != std::string::npos);
        const auto *swBack = std::get_if<AddSweepProfileOp>(&recipeOpsFromToml(sw.text, "s").stream.ops[0]);
        assert(swBack != nullptr && swBack->profile == "section" && swBack->path == "run_path"
               && near(swBack->baseZ, 1.0) && swBack->material == "marble");

        // Refused-before-build by name; recipeOpsResolved false while it survives.
        const RecipeCompileResult refused = compileRecipeOps({RecipeOp{sweep}});
        assert(!refused.ok);
        assert(refused.message == "AddSweepProfile must be resolved before compiling: cornice.run");
        RecipeOpStream survives;
        survives.ops = {RecipeOp{sweep}};
        assert(!recipeOpsResolved(survives));
        // Validate refuses an empty profile/path by name.
        AddSweepProfileOp noPath;
        noPath.name = "n";
        noPath.profile = "section"; // path empty
        const OpValidationReport npr = validateRecipeOps({RecipeOp{noPath}});
        bool sawMissingPath = false;
        for (const OpFinding &f : npr.findings) {
            sawMissingPath = sawMissingPath || f.code == "missing_path_reference";
        }
        assert(sawMissingPath);

        // AddPrism with a non-empty path round-trips the path.i.{x,y} run.
        RecipeOpStream ps;
        AddPrismOp prism;
        prism.name = "swept";
        prism.footprint = {{-0.5, 0.0}, {0.5, 0.0}, {0.5, 1.0}, {-0.5, 1.0}};
        prism.path = {{0.0, 0.0}, {4.0, 0.0}, {4.0, 3.0}};
        ps.ops.push_back(prism);
        const OpStreamTextResult pw = recipeOpsToToml(ps);
        assert(pw.ok);
        assert(pw.text.find("op.0.path.0.x = \"0\"") != std::string::npos);
        assert(pw.text.find("op.0.path.2.y = \"3\"") != std::string::npos);
        const auto *pBack = std::get_if<AddPrismOp>(&recipeOpsFromToml(pw.text, "p").stream.ops[0]);
        assert(pBack != nullptr && pBack->path.size() == 3
               && near(pBack->path[2].x, 4.0) && near(pBack->path[2].y, 3.0));
        // A swept prism (path present) validates with NO zero-height finding
        // (height is irrelevant when a path drives the solid).
        const OpValidationReport pvr = validateRecipeOps({RecipeOp{prism}});
        assert(pvr.ok);
        for (const OpFinding &f : pvr.findings) {
            assert(f.code != "prism_zero_height");
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
        assert(ew.ok);
        assert(ew.text.find(".path.") == std::string::npos); // no path keys at all
        const auto *eBack = std::get_if<AddPrismOp>(&recipeOpsFromToml(ew.text, "e").stream.ops[0]);
        assert(eBack != nullptr && eBack->path.empty() && near(eBack->height, 2.0));
    }

    // ---- BL-09: taper_end on the sweep + the prism it lowers to. Default 1.0,
    // round-trip on both ops, validate refuses <= 0 / non-finite by name. ----
    {
        // Default 1.0 round-trips on both ops.
        AddSweepProfileOp sweep;
        sweep.name = "spire";
        sweep.profile = "section";
        sweep.path = "rib";
        assert(near(sweep.taperEnd, 1.0)); // struct default
        sweep.taperEnd = 0.4;
        RecipeOpStream ss;
        ss.ops.push_back(sweep);
        const OpStreamTextResult sw = recipeOpsToToml(ss);
        assert(sw.ok && sw.text.find("op.0.taper_end = \"0.4\"") != std::string::npos);
        const auto *swBack = std::get_if<AddSweepProfileOp>(&recipeOpsFromToml(sw.text, "s").stream.ops[0]);
        assert(swBack != nullptr && near(swBack->taperEnd, 0.4));

        AddPrismOp prism;
        prism.name = "p";
        prism.footprint = {{0.0, 0.0}, {1.0, 0.0}, {1.0, 1.0}};
        prism.path = {{0.0, 0.0}, {2.0, 0.0}};
        prism.taperEnd = 0.5;
        RecipeOpStream ps;
        ps.ops.push_back(prism);
        const OpStreamTextResult pw = recipeOpsToToml(ps);
        assert(pw.ok && pw.text.find("op.0.taper_end = \"0.5\"") != std::string::npos);
        const auto *pBack = std::get_if<AddPrismOp>(&recipeOpsFromToml(pw.text, "p").stream.ops[0]);
        assert(pBack != nullptr && near(pBack->taperEnd, 0.5));

        // An absent taper_end defaults to 1.0 (a pre-BL-09 file is untapered).
        const OpStreamParseResult bare = recipeOpsFromToml(
            "op.0.type = \"AddSweepProfile\"\n"
            "op.0.name = \"bare\"\n"
            "op.0.profile = \"s\"\n"
            "op.0.path = \"p\"\n"
            "op.0.base_z = \"0\"\n", "bare");
        assert(bare.ok);
        assert(near(std::get_if<AddSweepProfileOp>(&bare.stream.ops[0])->taperEnd, 1.0));

        // Validate refuses taper_end <= 0 / non-finite by name on BOTH ops.
        const auto sawBadTaper = [](const OpValidationReport &r) {
            for (const OpFinding &f : r.findings) {
                if (f.code == "bad_taper_end") return true;
            }
            return false;
        };
        AddSweepProfileOp zeroTaper = sweep; zeroTaper.taperEnd = 0.0;
        assert(sawBadTaper(validateRecipeOps({RecipeOp{zeroTaper}})));
        AddPrismOp negTaper = prism; negTaper.taperEnd = -1.0;
        assert(sawBadTaper(validateRecipeOps({RecipeOp{negTaper}})));

        // The bind affordance: taper_end is a bindable Number on both ops.
        assert(opFieldBindable(RecipeOp{AddSweepProfileOp{}}, "taper_end"));
        assert(opFieldBindable(RecipeOp{AddPrismOp{}}, "taper_end"));
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
            assert(w.ok);
            assert(w.text.find(std::string("op.0.kind = \"") + c.text + "\"") != std::string::npos);
            assert(w.text.find("op.0.a = \"lhs\"") != std::string::npos);
            assert(w.text.find("op.0.b = \"rhs\"") != std::string::npos);
            const auto *back = std::get_if<AddBooleanOp>(&recipeOpsFromToml(w.text, "b").stream.ops[0]);
            assert(back != nullptr && back->a == "lhs" && back->b == "rhs" && back->kind == c.kind);
        }
        // A bad kind value is refused by name.
        const OpStreamParseResult badKind = recipeOpsFromToml(
            "op.0.type = \"AddBoolean\"\n"
            "op.0.name = \"c\"\nop.0.a = \"x\"\nop.0.b = \"y\"\n"
            "op.0.kind = \"merge\"\n", "bad");
        assert(!badKind.ok && badKind.message.find("union, subtract, or intersect") != std::string::npos);

        // Validate: a/b must name an EARLIER op; a later or absent operand is
        // refused by name. A valid earlier-operand boolean validates clean.
        AddBoxOp a; a.name = "solid.a"; a.width = a.depth = a.height = 1.0;
        AddBoxOp b; b.name = "solid.b"; b.width = b.depth = b.height = 1.0;
        AddBooleanOp good;
        good.name = "good.union"; good.a = "solid.a"; good.b = "solid.b";
        assert(validateRecipeOps({RecipeOp{a}, RecipeOp{b}, RecipeOp{good}}).ok);
        const auto sawCode = [](const OpValidationReport &r, const char *code) {
            for (const OpFinding &f : r.findings) {
                if (f.code == code) return true;
            }
            return false;
        };
        // Operand b is absent.
        AddBooleanOp missing;
        missing.name = "bad"; missing.a = "solid.a"; missing.b = "ghost";
        assert(sawCode(validateRecipeOps({RecipeOp{a}, RecipeOp{missing}}), "boolean_missing_operand"));
        // Operand named LATER (the boolean comes before its operand).
        AddBooleanOp early;
        early.name = "early"; early.a = "solid.a"; early.b = "solid.a";
        const OpValidationReport beforeOrder = validateRecipeOps({RecipeOp{early}, RecipeOp{a}});
        assert(sawCode(beforeOrder, "boolean_missing_operand"));
        // a == b is degenerate, refused by name.
        assert(sawCode(validateRecipeOps({RecipeOp{a}, RecipeOp{early}}), "boolean_self_operand"));

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
        assert(target.ops.size() == 4);
        const auto *mergedBool = std::get_if<AddBooleanOp>(&target.ops[3]);
        assert(mergedBool != nullptr);
        // Both operands rewritten to the namespaced SOURCE names.
        assert(mergedBool->a == "cap::shaft"); // not the target's bare "shaft"
        assert(mergedBool->b == "cap::bore");
        assert(validateRecipeOps(target.ops).ok); // the boolean finds its namespaced operands
    }

    return 0;
}
