#include "recipe/RecipeDocument.h"

#include "drafting/DraftingGeometry.h"
#include "drafting/DraftingPhysicalGeometry.h"

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
        {"bevel", "Bevel", false, {{"width", 0.05}, {"segments", 2.0}}},
        {"array", "Array", false, {{"count", 2.0}, {"offset_x", 1.0}}},
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

// One bound parameter -> one physical number. Field validity depends on the
// object's kind; an unanswerable field fails THIS parameter with a message
// the UI can show next to the binding.
ResolvedParam resolveMeasurement(const RecipeParam &param,
                                 const edi::drafting::DraftingDocument &drafting,
                                 const edi::drafting::DraftingGridProjection &grid)
{
    using namespace edi::drafting;

    ResolvedParam resolved;
    resolved.id = param.id;
    resolved.fromMeasurement = true;

    const DraftingObject *object = findObject(drafting, param.measurement.objectId);
    if (object == nullptr) {
        resolved.message = "object not found: " + param.measurement.objectId;
        return resolved;
    }
    const std::string &field = param.measurement.field;
    const Bounds2D bounds = computeBounds(object->geometry);

    if (field == "width") {
        resolved.value = physicalWidth(bounds.width, grid);
        resolved.ok = true;
    } else if (field == "height") {
        resolved.value = physicalHeight(bounds.height, grid);
        resolved.ok = true;
    } else if (field == "length") {
        if (const auto *line = std::get_if<LineGeometry>(&object->geometry)) {
            resolved.value = physicalDistance(line->a, line->b, grid);
            resolved.ok = true;
        } else {
            resolved.message = "length needs a line";
        }
    } else if (field == "radius") {
        // Radius scales along the grid's X axis — the same convention the
        // canvas projection uses for physical circle readouts.
        if (const auto *circle = std::get_if<CircleGeometry>(&object->geometry)) {
            resolved.value = physicalWidth(circle->radius, grid);
            resolved.ok = true;
        } else if (const auto *arc = std::get_if<ArcGeometry>(&object->geometry)) {
            resolved.value = physicalWidth(arc->radius, grid);
            resolved.ok = true;
        } else {
            resolved.message = "radius needs a circle or arc";
        }
    } else {
        resolved.message = "unknown measurement field: " + field;
    }
    return resolved;
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
        resolved.ok = resolved.ok && resolvedStep.ok;
        resolved.steps.push_back(std::move(resolvedStep));
    }
    return resolved;
}

} // namespace edi::recipe
