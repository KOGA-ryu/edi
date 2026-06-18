#include "drafting/DraftingMotifOps.h"

#include "drafting/DraftingDocument.h"
#include "drafting/DraftingGeometry.h" // computeBounds

#include <cassert>
#include <cmath>

using namespace edi::drafting;

static bool near(double a, double b, double eps = 1e-9)
{
    return std::abs(a - b) < eps;
}

int main()
{
    // --- buildMotifFromObjects ------------------------------------------------

    // Simple 2-object capture: normalize to (0,0) lower-left.
    {
        // A line from (2,3)→(5,7) and a point at (4,3).
        DraftingObject line = makeDraftingObject("o1", DraftingShapeKind::Line,
            DraftingGeometry{LineGeometry{{2.0, 3.0}, {5.0, 7.0}}});
        line.bounds = computeBounds(line.geometry);

        DraftingObject pt = makeDraftingObject("o2", DraftingShapeKind::Point,
            DraftingGeometry{PointGeometry{{4.0, 3.0}}});
        pt.bounds = computeBounds(pt.geometry);

        DraftingMotif motif = buildMotifFromObjects("test_motif", {line, pt});
        assert(motif.name == "test_motif");
        assert(motif.objects.size() == 2);

        // Union lower-left was (2,3), so objects are shifted by (-2,-3).
        // The line should now be (0,0)→(3,4).
        const auto *lineGeo = std::get_if<LineGeometry>(&motif.objects[0].geometry);
        assert(lineGeo != nullptr);
        assert(near(lineGeo->a.x, 0.0) && near(lineGeo->a.y, 0.0));
        assert(near(lineGeo->b.x, 3.0) && near(lineGeo->b.y, 4.0));

        // The point should now be (2,0).
        const auto *ptGeo = std::get_if<PointGeometry>(&motif.objects[1].geometry);
        assert(ptGeo != nullptr);
        assert(near(ptGeo->point.x, 2.0) && near(ptGeo->point.y, 0.0));

        // motif.bounds origin-based: x=0, y=0, width=3, height=4.
        assert(near(motif.bounds.x, 0.0) && near(motif.bounds.y, 0.0));
        assert(near(motif.bounds.width, 3.0) && near(motif.bounds.height, 4.0));
    }

    // lock/visible reset: captured objects always start unlocked+visible.
    {
        DraftingObject locked = makeDraftingObject("l1", DraftingShapeKind::Point,
            DraftingGeometry{PointGeometry{{1.0, 1.0}}});
        locked.bounds = computeBounds(locked.geometry);
        locked.locked  = true;
        locked.visible = false;

        DraftingMotif motif = buildMotifFromObjects("reset_test", {locked});
        assert(motif.objects.size() == 1);
        assert(motif.objects[0].locked  == false);
        assert(motif.objects[0].visible == true);
    }

    // Guide objects are EXCLUDED; ConstructionLine is INCLUDED.
    {
        DraftingObject guide = makeDraftingObject("g1", DraftingShapeKind::Guide,
            DraftingGeometry{GuideGeometry{GuideOrientation::Horizontal, 0.5}});
        guide.bounds = computeBounds(guide.geometry);

        DraftingObject cl = makeDraftingObject("c1", DraftingShapeKind::ConstructionLine,
            DraftingGeometry{ConstructionLineGeometry{{0.0, 0.0}, {1.0, 1.0}}});
        cl.bounds = computeBounds(cl.geometry);

        DraftingObject pt = makeDraftingObject("p1", DraftingShapeKind::Point,
            DraftingGeometry{PointGeometry{{0.5, 0.5}}});
        pt.bounds = computeBounds(pt.geometry);

        DraftingMotif motif = buildMotifFromObjects("filter_test", {guide, cl, pt});
        // Guide excluded; ConstructionLine and Point kept.
        assert(motif.objects.size() == 2);
        assert(motif.objects[0].kind == DraftingShapeKind::ConstructionLine);
        assert(motif.objects[1].kind == DraftingShapeKind::Point);
    }

    // All-guide source: yields empty motif (addMotif rejects it).
    {
        DraftingObject guide = makeDraftingObject("g2", DraftingShapeKind::Guide,
            DraftingGeometry{GuideGeometry{GuideOrientation::Vertical, 0.3}});
        guide.bounds = computeBounds(guide.geometry);

        DraftingMotif motif = buildMotifFromObjects("all_guides", {guide});
        assert(motif.objects.empty());
    }

    // --- addMotif + motifIndexByName ------------------------------------------

    DraftingDocument doc = makeDraftingDocument("doc1");

    // Build a minimal motif to work with.
    DraftingObject obj = makeDraftingObject("a1", DraftingShapeKind::Point,
        DraftingGeometry{PointGeometry{{0.1, 0.2}}});
    obj.bounds = computeBounds(obj.geometry);
    DraftingMotif m = buildMotifFromObjects("flower", {obj});

    // addMotif: success case — revision bumps, found by name.
    const auto revBefore = doc.revision;
    auto r1 = addMotif(doc, m);
    assert(r1.ok);
    assert(doc.revision == revBefore + 1);
    assert(doc.motifs.size() == 1);
    assert(doc.motifs[0].name == "flower");
    assert(motifIndexByName(doc, "flower").has_value());
    assert(!motifIndexByName(doc, "nonexistent").has_value());

    // Duplicate name: rejected.
    auto r2 = addMotif(doc, m);
    assert(!r2.ok);
    assert(r2.code == DraftingResultCode::DuplicateObjectId);

    // Empty name: rejected.
    DraftingMotif emptyName = m;
    emptyName.name = "";
    auto r3 = addMotif(doc, emptyName);
    assert(!r3.ok);
    assert(r3.code == DraftingResultCode::EmptyObjectId);

    // Empty objects: rejected.
    DraftingMotif emptyObjs;
    emptyObjs.name = "empty_motif";
    auto r4 = addMotif(doc, emptyObjs);
    assert(!r4.ok);
    assert(r4.code == DraftingResultCode::InvalidSelectionTarget);

    // --- removeMotif ----------------------------------------------------------

    auto r5 = removeMotif(doc, "flower");
    assert(r5.ok);
    assert(doc.motifs.empty());

    // Remove non-existent: rejected.
    auto r6 = removeMotif(doc, "flower");
    assert(!r6.ok);
    assert(r6.code == DraftingResultCode::ObjectNotFound);

    return 0;
}
