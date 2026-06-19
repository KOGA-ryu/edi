// Block library — Phase C, slice C1 (definition + persistence).
//
// S0 asserts the LAYOUT: the DraftingBlock struct exists and a fresh document
// carries an empty library. The ops cases assert buildBlockFromObjects NORMALIZES
// to the origin (so a placement offsets against a definition-local frame),
// add/removeBlock validate + bump revision, the definition round-trips through
// MessagePack additively (a pre-Phase-C file with no "blocks" key decodes empty),
// and highestDocumentIdSerial resumes the serial above EVERY id-bearing vector.
#include "drafting/DraftingBlockOps.h"
#include "drafting/DraftingDocument.h"
#include "drafting/DraftingSerialize.h"
#include "drafting/DraftingStore.h"

#include <cassert>
#include <cmath>
#include <string>
#include <variant>
#include <vector>

using namespace edi::drafting;

namespace {

bool nearly(double a, double b)
{
    return std::abs(a - b) < 1e-9;
}

// A line + a circle, deliberately NOT at the origin so normalization is visible.
// Union extent: x[0.3..0.6], y[0.4..0.55] => bounds {0.3, 0.4, 0.3, 0.15}.
std::vector<DraftingObject> sampleSources()
{
    DraftingObject line = makeDraftingObject("line_0001", DraftingShapeKind::Line,
                                             LineGeometry{{0.3, 0.4}, {0.6, 0.4}});
    DraftingObject circle = makeDraftingObject("circle_0002", DraftingShapeKind::Circle,
                                               CircleGeometry{{0.5, 0.5}, 0.05});
    return {line, circle};
}

} // namespace

int main()
{
    // S0: a fresh document carries an empty block library.
    {
        DraftingDocument document = makeDraftingDocument("doc-1", "Untitled");
        assert(document.blocks.empty());
    }

    // buildBlockFromObjects NORMALIZES the group to the origin and caches the
    // origin-based union extent, preserving the members' relative layout.
    {
        const std::vector<DraftingObject> sources = sampleSources();
        DraftingBlock block = buildBlockFromObjects("block_0001", "table", sources);

        assert(block.id == "block_0001");
        assert(block.name == "table");
        assert(block.assetRef.empty()); // S1: no asset ref unless one is passed
        assert(block.objects.size() == 2);

        // S1: a block can be born linked to its Blender asset.
        DraftingBlock linked = buildBlockFromObjects("block_0002", "chair", sources, "recipe.oak_chair");
        assert(linked.assetRef == "recipe.oak_chair");

        // Cached bounds: lower-left at the origin, extent = the source union span.
        assert(nearly(block.bounds.x, 0.0) && nearly(block.bounds.y, 0.0));
        assert(nearly(block.bounds.width, 0.3) && nearly(block.bounds.height, 0.15));

        // Every member shifted by the SAME (-0.3, -0.4): the line lands at the
        // origin, the circle centre follows, relative layout byte-identical.
        const auto &line = std::get<LineGeometry>(block.objects[0].geometry);
        assert(nearly(line.a.x, 0.0) && nearly(line.a.y, 0.0));
        assert(nearly(line.b.x, 0.3) && nearly(line.b.y, 0.0));
        const auto &circle = std::get<CircleGeometry>(block.objects[1].geometry);
        assert(nearly(circle.center.x, 0.2) && nearly(circle.center.y, 0.1));
        assert(nearly(circle.radius, 0.05));

        // The sources themselves are untouched (the block owns independent copies).
        const auto &srcLine = std::get<LineGeometry>(sources[0].geometry);
        assert(nearly(srcLine.a.x, 0.3) && nearly(srcLine.a.y, 0.4));
    }

    // addBlock accepts a valid definition, bumps revision, and rejects the
    // degenerate cases (empty id, duplicate id, empty objects).
    {
        DraftingDocument document = makeDraftingDocument("doc-2");
        const std::uint64_t before = document.revision;

        DraftingBlock block = buildBlockFromObjects("block_0001", "table", sampleSources());
        assert(addBlock(document, block).ok);
        assert(document.blocks.size() == 1);
        assert(document.revision == before + 1);

        // Empty id.
        DraftingBlock noId = buildBlockFromObjects("", "x", sampleSources());
        auto emptyId = addBlock(document, noId);
        assert(!emptyId.ok && emptyId.code == DraftingResultCode::EmptyObjectId);

        // Duplicate id.
        auto dup = addBlock(document, buildBlockFromObjects("block_0001", "again", sampleSources()));
        assert(!dup.ok && dup.code == DraftingResultCode::DuplicateObjectId);

        // Empty objects: a definition must define something.
        DraftingBlock empty = buildBlockFromObjects("block_0002", "nothing", {});
        assert(empty.objects.empty());
        auto emptyObjects = addBlock(document, empty);
        assert(!emptyObjects.ok && emptyObjects.code == DraftingResultCode::InvalidSelectionTarget);

        // None of the rejected adds changed the library.
        assert(document.blocks.size() == 1);
    }

    // removeBlock drops the definition (no cascade — FLATTEN owns no references),
    // and a second remove of the same id is rejected.
    {
        DraftingDocument document = makeDraftingDocument("doc-3");
        addBlock(document, buildBlockFromObjects("block_0001", "table", sampleSources()));
        assert(removeBlock(document, "block_0001").ok);
        assert(document.blocks.empty());
        auto gone = removeBlock(document, "block_0001");
        assert(!gone.ok && gone.code == DraftingResultCode::ObjectNotFound);
    }

    // Persistence: a definition round-trips through the value layer field-equal,
    // and a document with NO "blocks" key (every file before Phase C) decodes to
    // an empty library — additive-tolerant, so no document-version bump.
    {
        DraftingDocument document = makeDraftingDocument("doc-4");
        DraftingBlock def = buildBlockFromObjects("block_0001", "table", sampleSources());
        def.assetRef = "recipe.tavern_table"; // Seam B: the asset this block depicts
        addBlock(document, def);

        auto restored = draftingDocumentFromValue(draftingDocumentToValue(document));
        assert(restored.ok && restored.value);
        assert(restored.value->blocks.size() == 1);
        const DraftingBlock &rb = restored.value->blocks.front();
        assert(rb.id == "block_0001" && rb.name == "table");
        assert(rb.assetRef == "recipe.tavern_table"); // Seam-B link survives the round-trip
        assert(rb.objects.size() == 2);
        // Bounds are recomputed on load (never stored), so they survive the trip.
        assert(nearly(rb.bounds.x, 0.0) && nearly(rb.bounds.y, 0.0));
        assert(nearly(rb.bounds.width, 0.3) && nearly(rb.bounds.height, 0.15));
        // Member geometry preserved.
        const auto &line = std::get<LineGeometry>(rb.objects[0].geometry);
        assert(nearly(line.b.x, 0.3) && nearly(line.b.y, 0.0));
        const auto &circle = std::get<CircleGeometry>(rb.objects[1].geometry);
        assert(nearly(circle.radius, 0.05));

        // Tolerant additive read: a block written before Seam B has no "asset_ref"
        // key and must decode to an empty ref (no version bump needed).
        {
            edi::formats::MsgPackValue v = draftingDocumentToValue(document);
            for (auto &entry : v.mapValue) {
                if (entry.first != "document") continue;
                for (auto &docEntry : entry.second.mapValue) {
                    if (docEntry.first != "blocks") continue;
                    for (auto &blk : docEntry.second.arrayValue) {
                        auto &bm = blk.mapValue;
                        for (auto it = bm.begin(); it != bm.end();) {
                            it = (it->first == "asset_ref") ? bm.erase(it) : it + 1;
                        }
                    }
                }
            }
            auto tolerant = draftingDocumentFromValue(v);
            assert(tolerant.ok && tolerant.value);
            assert(tolerant.value->blocks.front().assetRef.empty());
        }

        // Strip the "blocks" key to simulate a pre-Phase-C file -> empty library.
        edi::formats::MsgPackValue value = draftingDocumentToValue(document);
        for (auto &entry : value.mapValue) {
            if (entry.first != "document") {
                continue;
            }
            auto &dm = entry.second.mapValue;
            for (auto it = dm.begin(); it != dm.end();) {
                it = (it->first == "blocks") ? dm.erase(it) : it + 1;
            }
        }
        auto stripped = draftingDocumentFromValue(value);
        assert(stripped.ok && stripped.value);
        assert(stripped.value->blocks.empty());
    }

    // highestDocumentIdSerial resumes above EVERY id-bearing vector — the
    // serial-recovery fix. A block_0007 alone yields 7; a higher plug_0009 wins
    // (the pre-existing plug/connection collision this closed too).
    {
        DraftingDocument document = makeDraftingDocument("doc-5");
        DraftingBlock block;
        block.id = "block_0007";
        document.blocks.push_back(block);
        assert(highestDocumentIdSerial(document) == 7);

        DraftingPlug plug;
        plug.id = "plug_0009";
        document.plugs.push_back(plug);
        assert(highestDocumentIdSerial(document) == 9);

        document.objects.push_back(makeDraftingObject("line_0003", DraftingShapeKind::Line,
                                                      LineGeometry{{0.0, 0.0}, {1.0, 1.0}}));
        assert(highestDocumentIdSerial(document) == 9); // plug still highest
    }

    // Phase-1 slice 3c: node ids ("node_NNNN") must be included in the serial scan.
    // If the scan missed nodes, reopening a document whose highest serial belongs to
    // a node would re-mint that same number with a DIFFERENT prefix ("object_0011"
    // collides with "node_0011" — same serial, different prefix = silent collision
    // in id lookups that compare by prefix+number). This is the C0/C2 lesson: every
    // id-bearing vector must participate in highestDocumentIdSerial.
    {
        DraftingDocument document = makeDraftingDocument("doc-nodes-serial");

        // Node with serial 11 — should become the new highest.
        DraftingNode node;
        node.id = "node_0011";
        document.nodes.push_back(node);
        assert(highestDocumentIdSerial(document) == 11);

        // Adding a lower-serial plug doesn't displace the node's serial.
        DraftingPlug plug;
        plug.id = "plug_0005";
        document.plugs.push_back(plug);
        assert(highestDocumentIdSerial(document) == 11); // node still highest

        // A block with a higher serial takes the lead.
        DraftingBlock block;
        block.id = "block_0015";
        document.blocks.push_back(block);
        assert(highestDocumentIdSerial(document) == 15);
    }

    return 0;
}
