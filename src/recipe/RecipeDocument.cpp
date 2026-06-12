#include "recipe/RecipeDocument.h"

#include "recipe/RecipeMeasure.h"
#include "drafting/DraftingGeometry.h"
#include "drafting/DraftingPhysicalGeometry.h"

#include <cmath>
#include <utility>

namespace edi::recipe {

namespace {

using edi::drafting::Bounds2D;
using edi::drafting::DraftingObject;

} // namespace

RecipeOpResult RecipeOpResult::accepted()
{
    RecipeOpResult result;
    result.ok = true;
    return result;
}

RecipeOpResult RecipeOpResult::rejected(std::string message)
{
    RecipeOpResult result;
    result.message = std::move(message);
    return result;
}

const std::vector<ShaperSpec> &shaperTable()
{
    // The woodworking vocabulary, first cut: two primitives that put stock
    // on the bench, two modifiers that pass it through a shaper. Parameter
    // ids deliberately match Blender's operator arguments where one exists —
    // the emitter then never needs a translation table.
    static const std::vector<ShaperSpec> table = {
        // loc_z places a part by its BOTTOM, not its centre: parts of an
        // assembly stack, and "the shaft starts at 0.10" is the number a
        // drafter can point at — the emitter does the centre arithmetic.
        {"cube", "Cube", true, {{"size_x", 1.0}, {"size_y", 1.0}, {"size_z", 1.0}, {"loc_z", 0.0}}},
        {"cylinder", "Cylinder", true, {{"radius", 0.5}, {"depth", 1.0}, {"loc_z", 0.0}}},
        // The lathe: a drafted profile spun around the vertical axis — the
        // page's left edge IS the axis, drafted x is the radius, drafted
        // height stands the part up. Its geometry lives in the profile
        // binding, so its only scalars are mesh resolution and an optional
        // base offset (the drafted heights are already authoritative).
        {"lathe", "Lathe", true, {{"segments", 64.0}, {"loc_z", 0.0}}, true},
        {"bevel", "Bevel", false, {{"width", 0.05}, {"segments", 2.0}}},
        {"array", "Array", false, {{"count", 2.0}, {"offset_x", 1.0}}},
        // The indexing-head cut: count grooves spaced evenly around the
        // current part (a turned column's flutes). The cutter bites `depth`
        // into the surface at `at_radius`, spanning z_from..z_to — five
        // numbers a drafter can point at, no angles to compute by hand.
        {"radial_groove", "Radial Groove", false,
         {{"count", 8.0}, {"cutter_radius", 0.05}, {"depth", 0.02},
          {"at_radius", 0.5}, {"z_from", 0.0}, {"z_to", 1.0}}},
    };
    return table;
}

const ShaperSpec *findShaper(const std::string &shaperId)
{
    for (const ShaperSpec &spec : shaperTable()) {
        if (shaperId == spec.id) {
            return &spec;
        }
    }
    return nullptr;
}

RecipeOpResult addShaperStep(RecipeDocument &document, const std::string &shaperId)
{
    const ShaperSpec *spec = findShaper(shaperId);
    if (spec == nullptr) {
        return RecipeOpResult::rejected("unknown shaper: " + shaperId);
    }
    if (document.steps.empty() && !spec->primitive) {
        return RecipeOpResult::rejected("a recipe starts with a primitive; nothing on the bench to modify");
    }
    ShaperStep step;
    step.shaperId = spec->id;
    for (const ShaperParamSpec &param : spec->params) {
        step.params.push_back({param.id, param.defaultValue, ParamSource::Literal, {}});
    }
    document.steps.push_back(std::move(step));
    ++document.revision;
    return RecipeOpResult::accepted();
}

RecipeOpResult removeShaperStep(RecipeDocument &document, std::size_t index)
{
    if (index >= document.steps.size()) {
        return RecipeOpResult::rejected("no such step");
    }
    // Removing the leading primitive must not leave a modifier first: the
    // grammar rule holds across edits, not only at append time.
    if (index == 0 && document.steps.size() > 1) {
        const ShaperSpec *next = findShaper(document.steps[1].shaperId);
        if (next != nullptr && !next->primitive) {
            return RecipeOpResult::rejected("removing this leaves a modifier first; remove the modifiers before the stock");
        }
    }
    document.steps.erase(document.steps.begin() + static_cast<std::ptrdiff_t>(index));
    ++document.revision;
    return RecipeOpResult::accepted();
}

RecipeOpResult moveShaperStep(RecipeDocument &document, std::size_t from, std::size_t to)
{
    if (from >= document.steps.size() || to >= document.steps.size()) {
        return RecipeOpResult::rejected("no such step");
    }
    if (from == to) {
        return RecipeOpResult::accepted();
    }
    RecipeDocument trial = document;
    ShaperStep moved = trial.steps[from];
    trial.steps.erase(trial.steps.begin() + static_cast<std::ptrdiff_t>(from));
    trial.steps.insert(trial.steps.begin() + static_cast<std::ptrdiff_t>(to), std::move(moved));
    const ShaperSpec *first = findShaper(trial.steps.front().shaperId);
    if (first == nullptr || !first->primitive) {
        return RecipeOpResult::rejected("a recipe starts with a primitive");
    }
    document.steps = std::move(trial.steps);
    ++document.revision;
    return RecipeOpResult::accepted();
}

namespace {

RecipeParam *findParam(RecipeDocument &document, std::size_t stepIndex, const std::string &paramId)
{
    if (stepIndex >= document.steps.size()) {
        return nullptr;
    }
    for (RecipeParam &param : document.steps[stepIndex].params) {
        if (param.id == paramId) {
            return &param;
        }
    }
    return nullptr;
}

} // namespace

RecipeOpResult setParamLiteral(RecipeDocument &document, std::size_t stepIndex,
                               const std::string &paramId, double value)
{
    // strtod upstream accepts "nan" and "inf"; a non-finite literal would
    // ride all the way into the emitted python as a NameError. Reject at
    // the op so every entry path (UI, TOML loader) shares one gate.
    if (!std::isfinite(value)) {
        return RecipeOpResult::rejected("not a finite number: " + paramId);
    }
    RecipeParam *param = findParam(document, stepIndex, paramId);
    if (param == nullptr) {
        return RecipeOpResult::rejected("no such parameter: " + paramId);
    }
    param->value = value;
    param->source = ParamSource::Literal;
    param->measurement = {};
    ++document.revision;
    return RecipeOpResult::accepted();
}

RecipeOpResult setStepProfile(RecipeDocument &document, std::size_t stepIndex,
                              std::string objectId)
{
    if (stepIndex >= document.steps.size()) {
        return RecipeOpResult::rejected("no such step");
    }
    const ShaperSpec *spec = findShaper(document.steps[stepIndex].shaperId);
    if (spec == nullptr || !spec->needsProfile) {
        return RecipeOpResult::rejected("this shaper does not take a profile");
    }
    if (objectId.empty()) {
        return RecipeOpResult::rejected("profile object id is empty");
    }
    document.steps[stepIndex].profileObjectId = std::move(objectId);
    ++document.revision;
    return RecipeOpResult::accepted();
}

RecipeOpResult bindParamToMeasurement(RecipeDocument &document, std::size_t stepIndex,
                                      const std::string &paramId, MeasurementRef measurement)
{
    RecipeParam *param = findParam(document, stepIndex, paramId);
    if (param == nullptr) {
        return RecipeOpResult::rejected("no such parameter: " + paramId);
    }
    if (measurement.objectId.empty() || measurement.field.empty()) {
        return RecipeOpResult::rejected("a binding names an object and a field");
    }
    param->source = ParamSource::Measurement;
    param->measurement = std::move(measurement);
    ++document.revision;
    return RecipeOpResult::accepted();
}

RecipeOpResult validateRecipe(const RecipeDocument &document)
{
    if (document.steps.empty()) {
        return RecipeOpResult::rejected("empty recipe");
    }
    for (const ShaperStep &step : document.steps) {
        if (findShaper(step.shaperId) == nullptr) {
            return RecipeOpResult::rejected("unknown shaper: " + step.shaperId);
        }
    }
    if (!findShaper(document.steps.front().shaperId)->primitive) {
        return RecipeOpResult::rejected("a recipe starts with a primitive");
    }
    return RecipeOpResult::accepted();
}

namespace {

// Pipeline A's per-parameter resolve, now a thin ADAPTER over the shared
// measurement seam (resolveMeasurementField, R1-B03). The seam owns the
// closed vocabulary and its exact wordings; A keeps only its ResolvedParam
// out-shape — the param id and the fromMeasurement flag the UI reads. The
// B01 contract pins in recipe_document_tests.cpp prove the swap preserved
// behavior (same numbers, same refusal strings).
ResolvedParam resolveMeasurement(const RecipeParam &param,
                                 const edi::drafting::DraftingDocument &drafting,
                                 const edi::drafting::DraftingGridProjection &grid)
{
    ResolvedParam resolved;
    resolved.id = param.id;
    resolved.fromMeasurement = true;

    const MeasureFieldResult field = resolveMeasurementField(
        drafting, grid, param.measurement.objectId, param.measurement.field);
    resolved.ok = field.ok;
    resolved.value = field.value;
    resolved.message = field.message;
    return resolved;
}

} // namespace

namespace {

// Pipeline A's profile resolve, now a thin ADAPTER over the shared seam
// (resolveProfilePoints, R1-B04 — the same move B03 made for
// measurements). The seam owns the source-kind dispatch, the page-to-part
// convention, the deterministic arc sampling, and the refusal wordings;
// A keeps only its ResolvedStep out-shape. The B01 profile pins in
// recipe_document_tests.cpp prove the swap preserved behavior.
void resolveStepProfile(const ShaperStep &step,
                        const edi::drafting::DraftingDocument &drafting,
                        const edi::drafting::DraftingGridProjection &grid,
                        ResolvedStep &resolvedStep)
{
    const ProfilePointsResult profile =
        resolveProfilePoints(drafting, grid, step.profileObjectId);
    resolvedStep.profileOk = profile.ok;
    resolvedStep.profileMessage = profile.message;
    resolvedStep.profilePoints = profile.points;
}

} // namespace

ResolvedRecipe resolveRecipe(const RecipeDocument &document,
                             const edi::drafting::DraftingDocument &drafting,
                             const edi::drafting::DraftingGridProjection &grid)
{
    ResolvedRecipe resolved;
    resolved.ok = validateRecipe(document).ok;
    for (const ShaperStep &step : document.steps) {
        ResolvedStep resolvedStep;
        resolvedStep.shaperId = step.shaperId;
        resolvedStep.ok = true;
        for (const RecipeParam &param : step.params) {
            if (param.source == ParamSource::Literal) {
                resolvedStep.params.push_back({param.id, param.value, true, false, {}});
                continue;
            }
            ResolvedParam fromCanvas = resolveMeasurement(param, drafting, grid);
            resolvedStep.ok = resolvedStep.ok && fromCanvas.ok;
            resolvedStep.params.push_back(std::move(fromCanvas));
        }
        const ShaperSpec *spec = findShaper(step.shaperId);
        if (spec != nullptr && spec->needsProfile) {
            resolveStepProfile(step, drafting, grid, resolvedStep);
            resolvedStep.ok = resolvedStep.ok && resolvedStep.profileOk;
        }
        resolved.ok = resolved.ok && resolvedStep.ok;
        resolved.steps.push_back(std::move(resolvedStep));
    }
    return resolved;
}

} // namespace edi::recipe
