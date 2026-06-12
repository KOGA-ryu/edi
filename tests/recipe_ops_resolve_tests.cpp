// The shared resolve seam (R1-B03): resolveRecipeOps turns a
// stream-with-bindings into a stream of literals by writing measured numbers
// through the B02 registry. The arithmetic deliberately mirrors pipeline A's
// recipe_document_tests.cpp — the SAME drafted objects and the SAME 12x8 grid
// — so the two pipelines' numbers cross-check: the proof the shared seam
// (RecipeMeasure) serves both with one vocabulary, not two that can drift.
#include "recipe/RecipeOpsResolve.h"
#include "recipe/RecipeOps.h"

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

    return 0;
}
