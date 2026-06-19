// Controller-level behavioral test for syncGraphForMovedObject (Phase-1 / brief 052).
//
// WHY a separate file (not inside drafting_graph_ops_tests.cpp)?
//   The pure op is in edi_drafting_core and needs no Qt. The controller path
//   exercises DrawingDocumentController — a Qt object — so it needs a
//   QCoreApplication. Mixing Qt and non-Qt tests in one executable is possible
//   but makes the two scopes' dependencies bleed together. A dedicated file keeps
//   the concern explicit: THIS file tests that the CONTROLLER WIRING is correct
//   (i.e. that applyCommandAndEmit actually calls syncGraphForMovedObject after
//   a geometry-mutation command).
//
// SCENARIO:
//   Build a crypt from buildCryptMapSpec(1.0) using the controller's
//   createMapFromSpec path.  Find the entrance room's East plug (the one
//   that connects to the crypt interior).  Verify its initial anchor position.
//   Select the anchor marker object and call moveSelectionNormalized to simulate
//   an interactive drag.  Assert that plug.anchor has been resynced — this would
//   FAIL if the controller wiring did not exist (the pre-052 bug).
//
// NOTE ON CHOICE OF PLUG:
//   The entrance room's East plug ("entrance_east_to_crypt") has its anchor
//   at the East wall midpoint of the entrance room (x = 15.0, y = 12.5 in
//   authored feet with scale=1.0).  Explicitly using the anchor from the live
//   document rather than a hardcoded position makes the test robust against
//   future room-layout tweaks in buildCryptMapSpec.

#include "core/DrawingCore.h"
#include "drafting/DraftingDocument.h"
#include "io/CryptGenerator.h"

#include <QCoreApplication>
#include <QString>

#include <cassert>
#include <cmath>
#include <optional>
#include <string>

using namespace edi::drafting;

static bool nearlyEqual(double a, double b, double eps = 1e-9)
{
    return std::abs(a - b) < eps;
}

int main(int argc, char **argv)
{
    // DrawingDocumentController uses Qt internally (id generation, signals) even
    // in headless context.  QCoreApplication must be alive before any controller.
    QCoreApplication app(argc, argv);

    // Build the crypt map (scale=1.0 → canvas units == authored feet).
    const MapSpec spec = edi::io::buildCryptMapSpec(1.0);

    DrawingDocumentController controller;
    assert(controller.createMapFromSpec(spec, 1.0));

    // The crypt has at least one plug — use the first one as the test anchor.
    // We find the anchor marker object via plug.anchorObjectId and verify
    // that after the controller move path, plug.anchor tracks the new position.
    const DraftingDocument &doc = controller.draftingDocument();
    assert(!doc.plugs.empty());

    // Capture the FIRST plug's pre-move state: its anchor position and the id of
    // the marker object it rides on.
    const DraftingPlug &firstPlug        = doc.plugs.front();
    const DraftingObjectId anchorObjId   = firstPlug.anchorObjectId;
    const Point2D preMoveAnchor          = firstPlug.anchor;

    // Verify the anchor marker object exists in the document.
    const DraftingObject *anchorObj = findObject(doc, anchorObjId);
    assert(anchorObj != nullptr);

    // Select the anchor marker, then move it +5 in X (canvas units = authored feet).
    // moveSelectionNormalized(dx, dy) goes through applyCommandAndEmit, which
    // (post-052) calls syncGraphForMovedObject for every selected object.
    const bool selected = controller.selectObjectById(
        QString::fromStdString(anchorObjId));
    assert(selected);

    // dx=5.0, dy=0.0 in canvas units.
    const double dx = 5.0;
    const double dy = 0.0;
    assert(controller.moveSelectionNormalized(dx, dy));

    // After the move, re-read the plug (reference was into the pre-mutation document
    // snapshot; read afresh from the live document).
    const DraftingDocument &docAfter  = controller.draftingDocument();
    const DraftingPlug &plugAfter     = docAfter.plugs.front();

    // Core assertion: the cached anchor must now reflect the moved position.
    // If syncGraphForMovedObject was NOT called, plugAfter.anchor would still hold
    // preMoveAnchor (the pre-052 drift bug).  With the fix it must equal the
    // object's new geometry.
    assert(nearlyEqual(plugAfter.anchor.x, preMoveAnchor.x + dx));
    assert(nearlyEqual(plugAfter.anchor.y, preMoveAnchor.y + dy));

    // The marker object's live geometry also matches.
    const DraftingObject *movedObj = findObject(docAfter, anchorObjId);
    assert(movedObj != nullptr);
    const auto *ptGeom = std::get_if<PointGeometry>(&movedObj->geometry);
    assert(ptGeom != nullptr);
    assert(nearlyEqual(ptGeom->point.x, preMoveAnchor.x + dx));
    assert(nearlyEqual(ptGeom->point.y, preMoveAnchor.y + dy));

    return 0;
}
