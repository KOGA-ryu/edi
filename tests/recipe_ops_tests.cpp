// The op vocabulary: typed ops + strict TOML store + compile pass +
// validators, ported from the prototype. The doric recipe used as the
// round-trip body below is the prototype's own example
// (examples/doric_column_recipe_v0.json), translated key-for-key.
#include "recipe/RecipeOps.h"
#include "recipe/RecipeOpsBind.h"
#include "recipe/RecipeOpsStore.h"
#include "recipe/RecipeOpsValidate.h"

#include "recipe_doric_fixture.h"

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

    return 0;
}
