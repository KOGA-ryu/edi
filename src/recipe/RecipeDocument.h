#pragma once

#include "drafting/DraftingDocument.h"
#include "drafting/DraftingGrid.h"

#include <cstddef>
#include <string>
#include <vector>

namespace edi::recipe {

// Feature #2's document: a Blender build recipe in the woodworking-shop
// grammar — a shape passes through *shapers* in a specific order. Plain
// structs + free functions, no Qt, exactly like the drafting core; the
// FeatureContext bus will own a live instance and signal changes.
//
// The point of the whole feature (user vision): the drafting grid supplies
// EXACT measurements. A shaper parameter is either a typed-in literal or a
// binding to a canvas object's measurement, resolved at emit time — no
// guesswork between user, script, and AI.

// Where a parameter's number comes from.
enum class ParamSource { Literal, Measurement };

// A measurement pulled from the drafting document: which object, which
// aspect of it. Fields are a closed vocabulary validated at resolve time:
// "width" / "height" (physical bounds), "length" (lines), "radius"
// (circles and arcs; physical scale follows the grid's X axis).
struct MeasurementRef {
    std::string objectId;
    std::string field;
};

struct RecipeParam {
    std::string id;     // from the shaper spec ("size_x", "radius", ...)
    double value = 0.0; // the literal (ignored when bound)
    ParamSource source = ParamSource::Literal;
    MeasurementRef measurement; // meaningful when source == Measurement
};

struct ShaperStep {
    std::string shaperId;
    std::vector<RecipeParam> params;
    // Some shapers (lathe) consume a PROFILE: a drafted line/polyline/arc
    // whose exact coordinates become the shape. A reference, not a copy —
    // the drafting document stays the single measurement authority, and
    // re-resolving after a profile edit picks up the new numbers.
    std::string profileObjectId; // empty = no profile bound
};

struct RecipeDocument {
    std::string id;
    std::string name;
    std::vector<ShaperStep> steps;
    int revision = 0;
};

// ---- Shaper vocabulary (data, not subclasses) ------------------------------

struct ShaperParamSpec {
    const char *id;
    double defaultValue;
};

struct ShaperSpec {
    const char *id;
    const char *label;
    // Primitives start a shape on the bench; modifiers reshape whatever is
    // already there. The grammar's one structural rule lives on this flag.
    bool primitive;
    std::vector<ShaperParamSpec> params;
    // True for shapers whose geometry comes from a drafted profile (lathe).
    bool needsProfile = false;
};

const std::vector<ShaperSpec> &shaperTable();
const ShaperSpec *findShaper(const std::string &shaperId);

// ---- Document ops (plan-style: ok + message, document untouched on reject) -

struct RecipeOpResult {
    bool ok = false;
    std::string message;

    static RecipeOpResult accepted();
    static RecipeOpResult rejected(std::string message);
};

// Appends a step with the spec's default params. Rejects unknown shapers and
// a modifier as the first step (nothing on the bench to modify yet).
RecipeOpResult addShaperStep(RecipeDocument &document, const std::string &shaperId);
RecipeOpResult removeShaperStep(RecipeDocument &document, std::size_t index);
RecipeOpResult moveShaperStep(RecipeDocument &document, std::size_t from, std::size_t to);
RecipeOpResult setParamLiteral(RecipeDocument &document, std::size_t stepIndex,
                               const std::string &paramId, double value);
RecipeOpResult bindParamToMeasurement(RecipeDocument &document, std::size_t stepIndex,
                                      const std::string &paramId, MeasurementRef measurement);
// Binds a drafted object as the step's profile. Rejected when the step's
// shaper takes none — pointing a profile at a cube is a category error the
// document should refuse, not silently store.
RecipeOpResult setStepProfile(RecipeDocument &document, std::size_t stepIndex,
                              std::string objectId);

// The grammar's structural validity: non-empty, starts with a primitive,
// every shaper known. (Per-param resolution problems are resolve concerns.)
RecipeOpResult validateRecipe(const RecipeDocument &document);

// ---- Resolution against the canvas -----------------------------------------

struct ResolvedParam {
    std::string id;
    double value = 0.0;
    bool ok = false;
    bool fromMeasurement = false;
    std::string message; // why resolution failed, for the UI/script comment
};

struct ResolvedStep {
    std::string shaperId;
    std::vector<ResolvedParam> params;
    // Profile resolution result: ordered PHYSICAL points (x = radius from
    // the spin axis at page-left, y = height above the page BOTTOM — the
    // drafted page maps to the part standing upright). Empty when the
    // shaper takes no profile.
    std::vector<edi::drafting::Point2D> profilePoints;
    bool profileOk = true;       // false fails the step like a stale binding
    std::string profileMessage;
    bool ok = false;
};

struct ResolvedRecipe {
    bool ok = false;
    std::vector<ResolvedStep> steps;
};

// Every bound parameter becomes a physical number from the drafting
// document and grid; literals pass through. A missing object or a field the
// object's kind cannot answer fails THAT parameter (with a message), never
// the whole resolve — the UI shows precisely which binding went stale.
ResolvedRecipe resolveRecipe(const RecipeDocument &document,
                             const edi::drafting::DraftingDocument &drafting,
                             const edi::drafting::DraftingGridProjection &grid);

} // namespace edi::recipe
