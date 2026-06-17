// The shared resolve seam (R1-B03): resolveRecipeOps turns a
// stream-with-bindings into a stream of literals by writing measured numbers
// through the B02 registry. The arithmetic deliberately mirrors pipeline A's
// recipe_document_tests.cpp — the SAME drafted objects and the SAME 12x8 grid
// — so the two pipelines' numbers cross-check: the proof the shared seam
// (RecipeMeasure) serves both with one vocabulary, not two that can drift.
#include "recipe/RecipeOpsResolve.h"
#include "recipe/RecipeOps.h"
#include "recipe/RecipeOpsValidate.h"

#include "drafting/DraftingStore.h"

#include <cassert>
#include <cmath>
#include <string>

using namespace edi::recipe;
using namespace edi::drafting;

namespace {

bool near(double a, double b, double tolerance = 1e-9)
{
    return std::abs(a - b) <= tolerance;
}

DraftingObject object(DraftingObjectId id, DraftingShapeKind kind, DraftingGeometry geometry)
{
    auto built = buildDraftingObject(std::move(id), kind, std::move(geometry));
    assert(built.ok);
    return built.object;
}

// The drafted document pipeline A's tests measure against: a rectangle
// (0.5 x 0.25), a circle (radius 0.1), a line spanning (0.3, 0.4) normalized,
// and an arc (radius 0.25 — A's "flare"). On a 12x8 physical grid the
// physical numbers are width 6.0, height 2.0, radius 1.2 (X-axis
// convention), length hypot(3.6, 3.2), arc radius 3.0.
DraftingDocument labDocument()
{
    DraftingDocument drafting = makeDraftingDocument("lab_doc");
    assert(addObject(drafting, object("plank", DraftingShapeKind::Rectangle,
        RectangleGeometry{{0.1, 0.1}, 0.5, 0.25})).ok);
    assert(addObject(drafting, object("hole", DraftingShapeKind::Circle,
        CircleGeometry{{0.5, 0.5}, 0.1})).ok);
    assert(addObject(drafting, object("cut", DraftingShapeKind::Line,
        LineGeometry{{0.2, 0.2}, {0.5, 0.6}})).ok);
    assert(addObject(drafting, object("arch", DraftingShapeKind::Arc,
        ArcGeometry{{0.5, 0.5}, 0.25, 0.0, 90.0})).ok);
    PolylineGeometry shaft;
    shaft.vertices = {{0.105, 0.90}, {0.085, 0.20}}; // A's lathe test profile
    assert(addObject(drafting, object("shaft", DraftingShapeKind::Polyline, shaft)).ok);
    PolylineGeometry shaftRev;
    shaftRev.vertices = {{0.085, 0.20}, {0.105, 0.90}}; // the SAME shaft drawn top-down
    assert(addObject(drafting, object("shaft_rev", DraftingShapeKind::Polyline, shaftRev)).ok);
    PolylineGeometry folded;
    folded.vertices = {{0.1, 0.5}, {0.2, 0.2}, {0.15, 0.7}}; // physical z 4.0,6.4,2.4 — non-monotonic
    assert(addObject(drafting, object("folded", DraftingShapeKind::Polyline, folded)).ok);
    // A closed square loop for the extrude (BL-03) footprint test — the last
    // vertex repeats the first, as a closed drafted polyline does.
    PolylineGeometry panel;
    panel.vertices = {{0.1, 0.1}, {0.3, 0.1}, {0.3, 0.3}, {0.1, 0.3}, {0.1, 0.1}};
    assert(addObject(drafting, object("panel", DraftingShapeKind::Polyline, panel)).ok);
    return drafting;
}

DraftingGridProjection labGrid()
{
    DraftingGridSettings settings;
    settings.width = 12.0;
    settings.height = 8.0;
    return projectDraftingGrid(settings);
}

const OpResolveFinding *findingFor(const OpResolveResult &result, const std::string &key)
{
    for (const OpResolveFinding &finding : result.findings) {
        if (finding.key == key) {
            return &finding;
        }
    }
    return nullptr;
}

} // namespace

int main()
{
    const DraftingDocument drafting = labDocument();
    const DraftingGridProjection grid = labGrid();

    // ---- Happy path: bindings on three op kinds resolve to exact physical
    // numbers; unbound literals pass through untouched; the resolved stream has
    // no bindings left; the INPUT is never mutated (the pass is pure). ----
    {
        RecipeOpStream stream;
        stream.id = "resolve.happy";
        stream.name = "Resolve Happy";
        AddBoxOp base;
        base.name = "base.block";
        base.depth = 9.0; // literal, must survive untouched
        stream.ops.push_back(base);
        AddCylinderOp shaft;
        shaft.name = "shaft.core";
        shaft.z = 3.5; // literal, must survive untouched
        stream.ops.push_back(shaft);
        CutFlutesOp flutes;
        flutes.target = "shaft.core";
        flutes.count = 20; // int literal, never bindable
        stream.ops.push_back(flutes);
        stream.bindings = {
            {0, "width", "plank", "width"},   // 6.0
            {0, "height", "plank", "height"}, // 2.0
            {0, "z", "arch", "radius"},       // 3.0 — radius answers an ARC too
            {1, "radius", "hole", "radius"},  // 1.2  (X-axis convention)
            {1, "height", "cut", "length"},   // hypot(3.6, 3.2)
            {2, "depth", "hole", "radius"},   // 1.2  on a CutFlutes field
        };

        const OpResolveResult result = resolveRecipeOps(stream, drafting, grid);
        assert(result.ok);
        assert(result.findings.empty());
        assert(result.stream.bindings.empty());

        const auto *box = std::get_if<AddBoxOp>(&result.stream.ops[0]);
        assert(box != nullptr);
        assert(near(box->width, 0.5 * 12.0));  // 6.0
        assert(near(box->height, 0.25 * 8.0)); // 2.0
        // The ARC half of the radius vocabulary, pinned: without this the
        // seam's arc branch is mutation-invisible (deleting it survived both
        // suites — caught in the B03 planner review).
        assert(near(box->z, 0.25 * 12.0));     // 3.0, arc radius, X-axis
        assert(near(box->depth, 9.0));         // untouched literal
        const auto *cyl = std::get_if<AddCylinderOp>(&result.stream.ops[1]);
        assert(cyl != nullptr);
        assert(near(cyl->radius, 0.1 * 12.0)); // 1.2 — matches A (recipe_document_tests:114)
        assert(near(cyl->height, std::hypot(0.3 * 12.0, 0.4 * 8.0))); // matches A (:115)
        assert(near(cyl->z, 3.5));             // untouched literal
        const auto *cut = std::get_if<CutFlutesOp>(&result.stream.ops[2]);
        assert(cut != nullptr);
        assert(near(cut->depth, 0.1 * 12.0)); // 1.2
        assert(cut->count == 20);             // untouched int literal

        // The input is pure-functionally untouched: bindings still present,
        // bound fields still their inert struct defaults.
        assert(stream.bindings.size() == 6);
        const auto *inputBox = std::get_if<AddBoxOp>(&stream.ops[0]);
        assert(inputBox != nullptr && inputBox->width == 0.0 && inputBox->height == 0.0);
        assert(inputBox->depth == 9.0);
        const auto *inputCyl = std::get_if<AddCylinderOp>(&stream.ops[1]);
        assert(inputCyl != nullptr && inputCyl->radius == 0.0 && inputCyl->height == 0.0);
    }

    // ---- Every refusal, exact composed wording. One bad binding fails the
    // whole resolve (ops empty), one finding addressed op.<index>.<fieldKey>.
    // The wordings are pipeline A's contract, verbatim through the seam. ----
    {
        const auto oneBox = []() {
            RecipeOpStream s;
            AddBoxOp b;
            b.name = "b";
            s.ops.push_back(b);
            return s;
        };
        const auto oneCylinder = []() {
            RecipeOpStream s;
            AddCylinderOp c;
            c.name = "c";
            s.ops.push_back(c);
            return s;
        };

        // missing object
        {
            RecipeOpStream s = oneBox();
            s.bindings = {{0, "width", "gone", "width"}};
            const OpResolveResult r = resolveRecipeOps(s, drafting, grid);
            assert(!r.ok && r.stream.ops.empty());
            assert(r.findings.size() == 1);
            assert(r.findings[0].key == "op.0.width");
            assert(r.findings[0].message == "object not found: gone");
        }
        // length on a circle
        {
            RecipeOpStream s = oneCylinder();
            s.bindings = {{0, "height", "hole", "length"}};
            const OpResolveResult r = resolveRecipeOps(s, drafting, grid);
            assert(!r.ok && r.stream.ops.empty());
            assert(r.findings.size() == 1);
            assert(r.findings[0].key == "op.0.height");
            assert(r.findings[0].message == "length needs a line");
        }
        // radius on a line
        {
            RecipeOpStream s = oneCylinder();
            s.bindings = {{0, "radius", "cut", "radius"}};
            const OpResolveResult r = resolveRecipeOps(s, drafting, grid);
            assert(!r.ok && r.stream.ops.empty());
            assert(r.findings.size() == 1);
            assert(r.findings[0].key == "op.0.radius");
            assert(r.findings[0].message == "radius needs a circle or arc");
        }
        // unknown measurement field
        {
            RecipeOpStream s = oneBox();
            s.bindings = {{0, "width", "plank", "girth"}};
            const OpResolveResult r = resolveRecipeOps(s, drafting, grid);
            assert(!r.ok && r.stream.ops.empty());
            assert(r.findings.size() == 1);
            assert(r.findings[0].key == "op.0.width");
            assert(r.findings[0].message == "unknown measurement field: girth");
        }
        // unbindable fieldKey: a hand-built binding on "vertices" (an int,
        // never bindable — the store refuses this file). The object resolves;
        // the WRITE is what the registry refuses, in the writer's wording
        // family. Reaching it proves the value path goes through setOpFieldValue.
        {
            RecipeOpStream s = oneCylinder();
            s.bindings = {{0, "vertices", "hole", "radius"}};
            const OpResolveResult r = resolveRecipeOps(s, drafting, grid);
            assert(!r.ok && r.stream.ops.empty());
            assert(r.findings.size() == 1);
            assert(r.findings[0].key == "op.0.vertices");
            assert(r.findings[0].message == "not a bindable field");
        }
    }

    // ---- Multi-failure: per-binding isolation. Two bad bindings → BOTH
    // findings present (named at once, not peeled one re-run at a time),
    // ok=false, and the result ops are empty (all-or-nothing — no
    // partially-resolved stream escapes). ----
    {
        RecipeOpStream s;
        AddBoxOp b;
        b.name = "b";
        s.ops.push_back(b);
        AddCylinderOp c;
        c.name = "c";
        s.ops.push_back(c);
        s.bindings = {
            {0, "width", "gone", "width"},  // object not found
            {1, "radius", "cut", "radius"}, // radius on a line
        };
        const OpResolveResult r = resolveRecipeOps(s, drafting, grid);
        assert(!r.ok);
        assert(r.stream.ops.empty());
        assert(r.findings.size() == 2);
        const OpResolveFinding *first = findingFor(r, "op.0.width");
        const OpResolveFinding *second = findingFor(r, "op.1.radius");
        assert(first != nullptr && first->message == "object not found: gone");
        assert(second != nullptr && second->message == "radius needs a circle or arc");
    }

    // ---- A healthy binding ALONGSIDE a bad one still resolves nothing: the
    // failure is all-or-nothing, yet the good binding leaves no finding (only
    // the broken one is named). ----
    {
        RecipeOpStream s;
        AddBoxOp b;
        b.name = "b";
        s.ops.push_back(b);
        s.bindings = {
            {0, "width", "plank", "width"},  // good — 6.0
            {0, "height", "gone", "height"}, // bad  — object not found
        };
        const OpResolveResult r = resolveRecipeOps(s, drafting, grid);
        assert(!r.ok);
        assert(r.stream.ops.empty());
        assert(r.findings.size() == 1);
        assert(r.findings[0].key == "op.0.height");
        assert(r.findings[0].message == "object not found: gone");
    }

    // ---- Out-of-range opIndex: only reachable on a hand-built stream (the
    // store refuses it at read AND write), but a pure function must not index
    // out of bounds, so the guard exists — and a guard that exists gets a pin
    // (ratifies the builder's flagged decision #1; wording reuses the B02
    // writer's "no such op"). ----
    {
        RecipeOpStream s;
        AddBoxOp b;
        b.name = "b";
        s.ops.push_back(b);
        s.bindings = {{9, "width", "plank", "width"}};
        const OpResolveResult r = resolveRecipeOps(s, drafting, grid);
        assert(!r.ok);
        assert(r.stream.ops.empty());
        assert(r.findings.size() == 1);
        assert(r.findings[0].key == "op.9.width");
        assert(r.findings[0].message == "no such op");
    }

    // ---- Profile lowering (R1-B04): the lathe, op-vocabulary native. The
    // numbers mirror A's lathe test EXACTLY (recipe_document_tests.cpp:
    // shaft polyline (0.105, 0.90)/(0.085, 0.20) on the 12x8 grid →
    // physical (radius 1.26, z 0.8) and (1.02, 6.4)); a bound base_z lands
    // on the lowered moulding; points get pointable generated names. ----
    {
        RecipeOpStream s;
        s.id = "resolve.lathe";
        AddRevolvedProfileOp lathe;
        lathe.name = "shaft.turned";
        lathe.profile = "shaft";
        lathe.vertices = 64;
        lathe.material = "limestone";
        s.ops.push_back(lathe);
        // A binding and a lowering on the SAME op: both effects must land.
        // (The internal order — bind, then lower — is a code guarantee; with
        // identical bindable keys on both kinds it is not observable here.)
        s.bindings = {{0, "base_z", "plank", "height"}}; // 2.0

        const OpResolveResult r = resolveRecipeOps(s, drafting, grid);
        assert(r.ok);
        assert(r.stream.bindings.empty());
        const auto *lowered = std::get_if<AddMouldingOp>(&r.stream.ops[0]);
        assert(lowered != nullptr); // the revolved op is GONE, a moulding stands
        assert(lowered->name == "shaft.turned");
        assert(near(lowered->baseZ, 0.25 * 8.0)); // the bound base_z landed
        assert(lowered->vertices == 64);
        assert(lowered->material == "limestone");
        assert(lowered->profile.size() == 2);
        assert(lowered->profile[0].term == "profile_00");
        assert(near(lowered->profile[0].radius, 0.105 * 12.0)); // 1.26 — matches A
        assert(near(lowered->profile[0].z, 0.10 * 8.0));        // 0.8  — page-bottom up
        assert(lowered->profile[1].term == "profile_01");
        assert(near(lowered->profile[1].radius, 0.085 * 12.0)); // 1.02
        assert(near(lowered->profile[1].z, 0.80 * 8.0));        // 6.4

        // The input stream still holds the un-lowered reference (purity).
        assert(std::get_if<AddRevolvedProfileOp>(&s.ops[0]) != nullptr);
    }

    // ---- Arc profile through the seam: 64 segments per full circle, so a
    // quarter sweep is 17 points with exact endpoints (A's flare numbers). ----
    {
        RecipeOpStream s;
        AddRevolvedProfileOp flare;
        flare.name = "flare.turned";
        flare.profile = "arch";
        s.ops.push_back(flare);
        const OpResolveResult r = resolveRecipeOps(s, drafting, grid);
        assert(r.ok);
        const auto *lowered = std::get_if<AddMouldingOp>(&r.stream.ops[0]);
        assert(lowered != nullptr);
        assert(lowered->profile.size() == 17);
        // The arc (drawn 0->90 deg) has strictly-FALLING physical z, so
        // decision-7 normalization (R1-B05) reverses it to canonical rising
        // order — the arc END is now first. Before B05 this lowered to a
        // DESCENDING moulding that would fail validation; normalized, it lofts
        // bottom-up like every other profile.
        assert(near(lowered->profile.front().radius, 0.50 * 12.0)); // arc end (0.5, 0.75), now first
        assert(near(lowered->profile.front().z, 0.25 * 8.0));        // z 2.0 (lowest ring)
        assert(near(lowered->profile.back().radius, 0.75 * 12.0));   // arc start (0.75, 0.5), now last
        assert(near(lowered->profile.back().z, 0.50 * 8.0));         // z 4.0
    }

    // ---- Profile refusals through resolve, key op.<i>.profile, the seam's
    // wordings verbatim (the same four A pins): empty reference, missing
    // object, wrong kind, short polyline is covered by A's corrupt-file pin
    // (the seam is shared — B01's pins prove these exact strings). ----
    {
        const auto oneLathe = [](std::string profileId) {
            RecipeOpStream s;
            AddRevolvedProfileOp lathe;
            lathe.name = "l";
            lathe.profile = std::move(profileId);
            s.ops.push_back(lathe);
            return s;
        };
        const OpResolveResult unbound = resolveRecipeOps(oneLathe(""), drafting, grid);
        assert(!unbound.ok && unbound.stream.ops.empty());
        assert(unbound.findings.size() == 1);
        assert(unbound.findings[0].key == "op.0.profile");
        assert(unbound.findings[0].message == "no profile bound");

        const OpResolveResult gone = resolveRecipeOps(oneLathe("gone"), drafting, grid);
        assert(!gone.ok);
        assert(gone.findings[0].key == "op.0.profile");
        assert(gone.findings[0].message == "profile object not found: gone");

        const OpResolveResult wrongKind = resolveRecipeOps(oneLathe("hole"), drafting, grid);
        assert(!wrongKind.ok);
        assert(wrongKind.findings[0].message == "profile needs a line, polyline, or arc");

        // The fourth wording, through THIS pass (A's corrupt-file pin dies
        // with pipeline A in B06; the op pipeline needs its own witness).
        // A one-vertex polyline cannot be built through the ops — hand-
        // assemble it the way a damaged .edidraw would deliver it.
        DraftingObject corrupt;
        corrupt.id = "stub";
        corrupt.kind = DraftingShapeKind::Polyline;
        PolylineGeometry oneVertex;
        oneVertex.vertices = {{0.5, 0.5}};
        corrupt.geometry = oneVertex;
        DraftingDocument corruptDoc = makeDraftingDocument("corrupt");
        corruptDoc.objects.push_back(corrupt);
        const OpResolveResult shortPolyline =
            resolveRecipeOps(oneLathe("stub"), corruptDoc, grid);
        assert(!shortPolyline.ok);
        assert(shortPolyline.findings[0].key == "op.0.profile");
        assert(shortPolyline.findings[0].message == "profile polyline needs at least two vertices");

        // A failed lowering joins binding failures under one all-or-nothing
        // roof: both findings, no stream.
        RecipeOpStream mixed = oneLathe("gone");
        mixed.bindings = {{0, "x", "missing", "width"}};
        const OpResolveResult both = resolveRecipeOps(mixed, drafting, grid);
        assert(!both.ok && both.stream.ops.empty());
        assert(both.findings.size() == 2);
        assert(findingFor(both, "op.0.x") != nullptr);
        assert(findingFor(both, "op.0.profile") != nullptr);
    }

    // ---- The gate predicate (R1-B05 decision 1): resolved means no bindings
    // AND no surviving lathe reference. All four corners pinned, plus the proof
    // that a successful resolve produces a gate-passing stream. ----
    {
        RecipeOpStream clean;
        AddBoxOp b;
        b.name = "b";
        clean.ops.push_back(b);
        assert(recipeOpsResolved(clean)); // neither: resolved

        RecipeOpStream bound = clean;
        bound.bindings = {{0, "width", "plank", "width"}};
        assert(!recipeOpsResolved(bound)); // bindings only

        RecipeOpStream lathe;
        AddRevolvedProfileOp r;
        r.name = "r";
        r.profile = "shaft";
        lathe.ops.push_back(r);
        assert(!recipeOpsResolved(lathe)); // revolved only — an empty binding table is not enough

        RecipeOpStream both = lathe;
        both.bindings = {{0, "base_z", "plank", "height"}};
        assert(!recipeOpsResolved(both)); // both

        // The contract the gate exists to enforce: resolveRecipeOps's output
        // always passes — bindings cleared AND the lathe lowered to a moulding.
        const OpResolveResult resolved = resolveRecipeOps(lathe, drafting, grid);
        assert(resolved.ok);
        assert(recipeOpsResolved(resolved.stream));
    }

    // ---- Direction-normalization (R1-B05 decision 7): the shaft drawn
    // top-down (strictly-falling physical z) lowers to the IDENTICAL moulding
    // as the bottom-up drawing, point for point — only the order flips, no
    // coordinate invented. A FOLDED profile is NOT rescued: it lowers and still
    // FAILS validation by name. ----
    {
        RecipeOpStream forward;
        AddRevolvedProfileOp f;
        f.name = "turned";
        f.profile = "shaft";
        forward.ops.push_back(f);
        const OpResolveResult fr = resolveRecipeOps(forward, drafting, grid);
        assert(fr.ok);
        const auto *forwardM = std::get_if<AddMouldingOp>(&fr.stream.ops[0]);
        assert(forwardM != nullptr);

        RecipeOpStream reversed;
        AddRevolvedProfileOp v;
        v.name = "turned";
        v.profile = "shaft_rev";
        reversed.ops.push_back(v);
        const OpResolveResult rr = resolveRecipeOps(reversed, drafting, grid);
        assert(rr.ok);
        const auto *reversedM = std::get_if<AddMouldingOp>(&rr.stream.ops[0]);
        assert(reversedM != nullptr);

        // Point-for-point identical: normalization restored A's orientation-
        // agnostic promise (profile_00 is the page-bottom ring in both).
        assert(reversedM->profile.size() == forwardM->profile.size());
        assert(forwardM->profile.size() == 2);
        for (std::size_t i = 0; i < forwardM->profile.size(); ++i) {
            assert(reversedM->profile[i].term == forwardM->profile[i].term);
            assert(near(reversedM->profile[i].z, forwardM->profile[i].z));
            assert(near(reversedM->profile[i].radius, forwardM->profile[i].radius));
        }
        // And it really did flip: profile_00 is the LOW ring (z 0.8), not the
        // drafted-first point (z 6.4). Without the reversal this would be 6.4.
        assert(near(reversedM->profile[0].z, 0.10 * 8.0));

        // A folded profile lowers (lowering is not validation) but the moulding
        // it becomes fails validation by name.
        RecipeOpStream foldedStream;
        AddRevolvedProfileOp folds;
        folds.name = "folded.turned";
        folds.profile = "folded";
        foldedStream.ops.push_back(folds);
        const OpResolveResult foldedResolved = resolveRecipeOps(foldedStream, drafting, grid);
        assert(foldedResolved.ok); // lowered, not yet judged
        // Planner ruling on the builder's flag #2 (all-pairs, not endpoints):
        // this fold's ENDPOINTS happen to descend net (4.0 → 2.4), but it is
        // not strictly falling, so it lowers VERBATIM — drafted-first stays
        // first. The endpoint-test variant would have reversed it; only a
        // profile the loft can fully understand gets normalized.
        const auto *foldedM = std::get_if<AddMouldingOp>(&foldedResolved.stream.ops[0]);
        assert(foldedM != nullptr);
        assert(near(foldedM->profile[0].z, 4.0));
        assert(near(foldedM->profile[1].z, 6.4));
        assert(near(foldedM->profile[2].z, 2.4));
        const OpValidationReport report = validateRecipeOps(foldedResolved.stream.ops);
        assert(!report.ok);
        bool sawNonMonotonic = false;
        for (const OpFinding &finding : report.findings) {
            if (finding.code == "moulding_profile_not_monotonic") {
                sawNonMonotonic = true;
            }
        }
        assert(sawNonMonotonic);
    }

    // ---- Finiteness gate (R1-B05 decision 3): a resolved number that comes
    // back non-finite is refused by name, per offender, before the stream is
    // declared resolved. The number cannot arise from grid settings —
    // projectDraftingGrid sanitizes width/height through finiteOr, so they are
    // always finite (flagged mismatch with the bucket's "via grid settings").
    // The reachable source is degenerate GEOMETRY a corrupt .edidraw could
    // deliver; hand-assemble it like A's corrupt-polyline pin. radius 1e308
    // scaled along the grid's X (x12) overflows to +inf. ----
    {
        DraftingObject huge;
        huge.id = "huge";
        huge.kind = DraftingShapeKind::Circle;
        huge.geometry = CircleGeometry{{0.5, 0.5}, 1e308};
        DraftingDocument hugeDoc = makeDraftingDocument("huge_doc");
        hugeDoc.objects.push_back(huge);

        RecipeOpStream s;
        AddCylinderOp c;
        c.name = "c";
        s.ops.push_back(c);
        s.bindings = {{0, "radius", "huge", "radius"}}; // 1e308 * 12 = inf
        const OpResolveResult r = resolveRecipeOps(s, hugeDoc, grid);
        assert(!r.ok && r.stream.ops.empty());
        assert(r.findings.size() == 1);
        assert(r.findings[0].key == "op.0.radius");
        assert(r.findings[0].message == "not a finite number");
    }

    // ---- Extrude lowering (BL-03): every AddExtrudedProfileOp lowers to a
    // concrete AddPrismOp whose footprint is the drafted figure's PHYSICAL
    // x/y (no radius reinterpretation, no z-flip — the projector that
    // contrasts the lathe). A bound height lands before lowering. ----
    {
        RecipeOpStream s;
        s.id = "resolve.extrude";
        AddExtrudedProfileOp prism;
        prism.name = "wall.panel";
        prism.profile = "panel";
        prism.baseZ = 1.0;
        prism.material = "limestone";
        s.ops.push_back(prism);
        s.bindings = {{0, "height", "plank", "height"}}; // 2.0 — lands pre-lowering

        const OpResolveResult r = resolveRecipeOps(s, drafting, grid);
        assert(r.ok);
        assert(r.stream.bindings.empty());
        const auto *lowered = std::get_if<AddPrismOp>(&r.stream.ops[0]);
        assert(lowered != nullptr); // the extrude reference is GONE, a prism stands
        assert(lowered->name == "wall.panel");
        assert(near(lowered->height, 0.25 * 8.0)); // the bound height landed: 2.0
        assert(near(lowered->baseZ, 1.0));
        assert(lowered->material == "limestone");
        // Footprint = physical x/y of the closed square (x*12, y*8), the
        // repeated closing vertex preserved.
        assert(lowered->footprint.size() == 5);
        assert(near(lowered->footprint[0].x, 0.1 * 12.0) && near(lowered->footprint[0].y, 0.1 * 8.0));
        assert(near(lowered->footprint[1].x, 0.3 * 12.0) && near(lowered->footprint[1].y, 0.1 * 8.0));
        assert(near(lowered->footprint[2].x, 0.3 * 12.0) && near(lowered->footprint[2].y, 0.3 * 8.0));
        assert(near(lowered->footprint[4].x, 0.1 * 12.0) && near(lowered->footprint[4].y, 0.1 * 8.0));
        // The lowered prism passes validation and the resolve gate.
        assert(validateRecipeOps(r.stream.ops).ok);
        assert(recipeOpsResolved(r.stream));
        // The input is pure-functionally untouched.
        assert(std::get_if<AddExtrudedProfileOp>(&s.ops[0]) != nullptr);
    }

    // ---- Extrude refusals: deleted / wrong-kind / open use the SHARED profile
    // wordings (the same four the lathe uses); a line (2 points) is degenerate
    // for a footprint and refused by name. All keyed op.<i>.profile. ----
    {
        const auto oneExtrude = [](std::string profileId) {
            RecipeOpStream s;
            AddExtrudedProfileOp e;
            e.name = "e";
            e.profile = std::move(profileId);
            e.height = 2.0;
            s.ops.push_back(e);
            return s;
        };
        // deleted object — shared wording
        const OpResolveResult gone = resolveRecipeOps(oneExtrude("gone"), drafting, grid);
        assert(!gone.ok && gone.stream.ops.empty());
        assert(gone.findings[0].key == "op.0.profile");
        assert(gone.findings[0].message == "profile object not found: gone");
        // wrong kind — shared wording
        const OpResolveResult wrong = resolveRecipeOps(oneExtrude("hole"), drafting, grid);
        assert(!wrong.ok);
        assert(wrong.findings[0].message == "profile needs a line, polyline, or arc");
        // empty reference — shared wording
        const OpResolveResult unbound = resolveRecipeOps(oneExtrude(""), drafting, grid);
        assert(!unbound.ok);
        assert(unbound.findings[0].message == "no profile bound");
        // a LINE (the 2-vertex shaft polyline) projects to 2 distinct points —
        // not an area, so the footprint is degenerate, refused by name.
        const OpResolveResult line = resolveRecipeOps(oneExtrude("shaft"), drafting, grid);
        assert(!line.ok);
        assert(line.findings[0].key == "op.0.profile");
        assert(line.findings[0].message == "profile must enclose at least three distinct points");
    }

    // ---- BL-05 HEADLINE: push/pull as a depth verb — a DRAFTED MEASUREMENT
    // drives the extrude height end-to-end. `height` is just a bindable Number
    // opField on the extrude arm (BL-01), so the SAME bindings pass that fills
    // a box width fills it: bind height -> the "cut" line's physical length,
    // resolve, and the lowered AddPrism carries that measured depth. No depth
    // mechanism of its own — a signed double on an existing arm, measured like
    // any other field. ----
    {
        RecipeOpStream s;
        s.id = "resolve.pushpull";
        AddExtrudedProfileOp prism;
        prism.name = "wall.pull";
        prism.profile = "panel";
        prism.baseZ = 0.0;
        s.ops.push_back(prism);
        s.bindings = {{0, "height", "cut", "length"}}; // measured line length -> depth

        const OpResolveResult r = resolveRecipeOps(s, drafting, grid);
        assert(r.ok);
        assert(r.stream.bindings.empty());
        const auto *lowered = std::get_if<AddPrismOp>(&r.stream.ops[0]);
        assert(lowered != nullptr);
        // The drafted "cut" line spans (0.3, 0.4) normalized on the 12x8 grid:
        // physical length hypot(3.6, 3.2) — the SAME number the box-height
        // binding measures in the happy-path block above. The extrude DEPTH is
        // now a drafted measurement.
        assert(near(lowered->height, std::hypot(0.3 * 12.0, 0.4 * 8.0)));
        assert(validateRecipeOps(r.stream.ops).ok);
        assert(recipeOpsResolved(r.stream));
    }

    // ---- BL-05: a NEGATIVE height is a buildable PUSH (downward cut), not a
    // refusal. It passes validate with no zero-height finding and survives
    // resolve into the carrier verbatim (BL-04's Python _prism_world then
    // extrudes it downward — never abs()'d). The carrier carries the SIGN. ----
    {
        AddExtrudedProfileOp push;
        push.name = "wall.push";
        push.profile = "panel";
        push.height = -2.5; // a downward push/pull cut
        push.baseZ = 4.0;

        // The extrude op itself validates: negative height is allowed (no
        // extruded_profile_zero_height), only zero/non-finite is refused.
        const OpValidationReport extrudeReport = validateRecipeOps({RecipeOp{push}});
        for (const OpFinding &f : extrudeReport.findings) {
            assert(f.code != "extruded_profile_zero_height");
        }

        RecipeOpStream s;
        s.ops = {RecipeOp{push}};
        const OpResolveResult r = resolveRecipeOps(s, drafting, grid);
        assert(r.ok);
        const auto *lowered = std::get_if<AddPrismOp>(&r.stream.ops[0]);
        assert(lowered != nullptr);
        assert(near(lowered->height, -2.5)); // the negative depth survived into the carrier
        // The lowered prism also validates (negative prism height is allowed)
        // and carries no zero-height finding.
        const OpValidationReport prismReport = validateRecipeOps(r.stream.ops);
        assert(prismReport.ok);
        for (const OpFinding &f : prismReport.findings) {
            assert(f.code != "prism_zero_height");
        }
    }

    return 0;
}
