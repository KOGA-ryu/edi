#include "drafting/DraftingDocument.h"
#include "drafting/DraftingGeometry.h"
#include "drafting/DraftingHitTest.h"
#include "drafting/DraftingNumericEdit.h"
#include "drafting/DraftingObjectEdit.h"
#include "drafting/DraftingSerialize.h"
#include "drafting/DraftingToolCreation.h"

#include <cassert>
#include <cmath>
#include <string>

using namespace edi::drafting;

namespace {

bool nearlyEqual(double a, double b, double eps = 0.000001)
{
    return std::abs(a - b) <= eps;
}

DraftingObject textObject(const TextAnnotationGeometry &t)
{
    DraftingObject object = makeDraftingObject("text-1", DraftingShapeKind::TextAnnotation, t);
    object.bounds = computeBounds(object.geometry);
    return object;
}

} // namespace

int main()
{
    const TextAnnotationGeometry text{{0.3, 0.4}, "Hello", 0.1};

    // validate: good accepted; negative height / non-finite position rejected.
    assert(validateGeometry(text).ok);
    {
        TextAnnotationGeometry bad = text;
        bad.height = -1.0;
        assert(!validateGeometry(bad).ok);
        bad = text;
        bad.position.x = std::nan("");
        assert(!validateGeometry(bad).ok);
    }

    // kind round-trips through the variant and the name maps.
    assert(geometryKind(text) == DraftingShapeKind::TextAnnotation);
    assert(std::string(shapeKindName(DraftingShapeKind::TextAnnotation)) == "text");
    assert(shapeKindFromName("text") == DraftingShapeKind::TextAnnotation);

    // bounds: a box at the position; width grows with content length.
    {
        const Bounds2D b = computeBounds(DraftingGeometry{text});
        assert(nearlyEqual(b.x, 0.3) && nearlyEqual(b.y, 0.4));
        assert(nearlyEqual(b.height, 0.1));
        assert(nearlyEqual(b.width, 5 * 0.1 * 0.55)); // "Hello" = 5 chars
        // empty content collapses to the minimum width, not zero.
        const TextAnnotationGeometry empty{{0.0, 0.0}, "", 0.1};
        assert(nearlyEqual(computeBounds(DraftingGeometry{empty}).width, 0.1 * 0.3));
    }

    // translate moves the position; content and height are unchanged.
    {
        const auto moved = std::get<TextAnnotationGeometry>(translateGeometry(DraftingGeometry{text}, 0.1, -0.2));
        assert(nearlyEqual(moved.position.x, 0.4) && nearlyEqual(moved.position.y, 0.2));
        assert(moved.content == "Hello" && nearlyEqual(moved.height, 0.1));
    }

    // hit: a point inside the box is near; a far point is positive.
    {
        const DraftingObject object = textObject(text);
        assert(hitDistance(object.geometry, {0.4, 0.45}) < 0.0001); // inside the box
        assert(hitDistance(object.geometry, {2.0, 2.0}) > 1.0);     // far away
    }

    // handle: one move handle at the position; dragging it moves the text and
    // preserves the content (a move that dropped the string would fail here).
    {
        const DraftingObject object = textObject(text);
        const auto handles = draftingHandlesForObject(object);
        assert(handles.size() == 1 && handles[0].id == "text_position");
        assert(nearlyEqual(handles[0].point.x, 0.3) && nearlyEqual(handles[0].point.y, 0.4));
        const DraftingHandleEditPlan plan = handleEditPlan(object, "text_position", {0.6, 0.7});
        assert(plan.ok);
        const DraftingObjectEditResult edit = applyObjectEdit(object, plan.edit);
        assert(edit.ok);
        const auto moved = std::get<TextAnnotationGeometry>(edit.geometry);
        assert(nearlyEqual(moved.position.x, 0.6) && nearlyEqual(moved.position.y, 0.7));
        assert(moved.content == "Hello");
    }

    // numeric edit: setting the height by field id.
    {
        const DraftingObject object = textObject(text);
        const DraftingNumericEditResult e = applyNumericGeometryEdit(object, "height", 0.2);
        assert(e.ok && nearlyEqual(std::get<TextAnnotationGeometry>(e.geometry).height, 0.2));
    }

    // tool creation: a single click (the controller passes start == end) places
    // a default-content text annotation at the click point.
    {
        DraftingToolCreationRequest request;
        request.tool = DraftingToolKind::TextAnnotation;
        request.objectId = "t2";
        request.start = {0.25, 0.25};
        request.end = {0.25, 0.25};
        const DraftingObjectBuildResult built = buildDraftingObjectForTool(request);
        assert(built.ok && built.object.kind == DraftingShapeKind::TextAnnotation);
        const auto geo = std::get<TextAnnotationGeometry>(built.object.geometry);
        assert(nearlyEqual(geo.position.x, 0.25) && nearlyEqual(geo.position.y, 0.25));
        assert(geo.content == "Text" && nearlyEqual(geo.height, 0.04));
    }

    // serialize round-trip: the STRING content survives (MsgPack text, incl. UTF-8).
    {
        DraftingDocument doc = makeDraftingDocument("d", "t");
        doc.objects = {makeDraftingObject("text-rt", DraftingShapeKind::TextAnnotation,
                                          TextAnnotationGeometry{{0.2, 0.2}, "Round trip \xE2\x9C\x93", 0.06})};
        const auto value = draftingDocumentToValue(doc);
        const auto restored = draftingDocumentFromValue(value);
        assert(restored.ok && restored.value);
        const auto &g = std::get<TextAnnotationGeometry>(restored.value->objects[0].geometry);
        assert(g.content == "Round trip \xE2\x9C\x93");
        assert(nearlyEqual(g.height, 0.06));
    }

    return 0;
}
