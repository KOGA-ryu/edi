#include "recipe/RecipeStore.h"

#include "formats/TomlReader.h"
#include "formats/TomlWriter.h"
#include "recipe/RecipeTextNumbers.h"

#include <string>
#include <vector>

namespace edi::recipe {

namespace {

// Number text lives in RecipeTextNumbers.h — ONE source of truth shared
// with the op-stream store; two formats disagreeing on number text would
// be a quiet fork.
bool parseNumber(const std::string &text, double &value)
{
    return parseNumberText(text, value);
}

std::string stepPrefix(std::size_t index)
{
    return "step." + std::to_string(index);
}

} // namespace

RecipeTextResult recipeToToml(const RecipeDocument &document)
{
    edi::formats::StaticConfig config;
    config["recipe.id"] = document.id;
    config["recipe.name"] = document.name;
    for (std::size_t i = 0; i < document.steps.size(); ++i) {
        const ShaperStep &step = document.steps[i];
        const std::string prefix = stepPrefix(i);
        config[prefix + ".shaper"] = step.shaperId;
        if (!step.profileObjectId.empty()) {
            config[prefix + ".profile"] = step.profileObjectId;
        }
        for (const RecipeParam &param : step.params) {
            const std::string paramPrefix = prefix + ".param." + param.id;
            if (param.source == ParamSource::Measurement) {
                config[paramPrefix + ".object"] = param.measurement.objectId;
                config[paramPrefix + ".field"] = param.measurement.field;
            } else {
                config[paramPrefix + ".value"] = numberKeyText(param.value);
            }
        }
    }

    const auto written = edi::formats::writeTomlStaticConfig(config, "recipe");
    RecipeTextResult result;
    if (!written.ok) {
        result.message = written.message;
        return result;
    }
    result.ok = true;
    result.text = *written.value;
    return result;
}

RecipeParseResult recipeFromToml(const std::string &text, const std::string &source)
{
    RecipeParseResult result;
    const auto parsed = edi::formats::readTomlStaticConfig(text, source);
    if (!parsed.ok) {
        result.message = parsed.message;
        return result;
    }
    const edi::formats::StaticConfig &config = *parsed.value;

    const auto value = [&config](const std::string &key) -> const std::string * {
        const auto found = config.find(key);
        return found == config.end() ? nullptr : &found->second;
    };

    RecipeDocument document;
    if (const std::string *id = value("recipe.id")) {
        document.id = *id;
    }
    if (const std::string *name = value("recipe.name")) {
        document.name = *name;
    }

    // Rebuild through the SAME ops the UI uses: unknown shapers, unknown
    // param ids, and modifier-first structure are rejected by the ops
    // themselves — the loader adds no second validator to drift from them.
    for (std::size_t i = 0;; ++i) {
        const std::string prefix = stepPrefix(i);
        const std::string *shaperId = value(prefix + ".shaper");
        if (shaperId == nullptr) {
            break;
        }
        const RecipeOpResult added = addShaperStep(document, *shaperId);
        if (!added.ok) {
            result.message = prefix + ": " + added.message;
            return result;
        }
        if (const std::string *profile = value(prefix + ".profile")) {
            const RecipeOpResult bound = setStepProfile(document, i, *profile);
            if (!bound.ok) {
                result.message = prefix + ".profile: " + bound.message;
                return result;
            }
        }
        for (const RecipeParam &param : document.steps[i].params) {
            const std::string paramPrefix = prefix + ".param." + param.id;
            const std::string *object = value(paramPrefix + ".object");
            const std::string *field = value(paramPrefix + ".field");
            const std::string *literal = value(paramPrefix + ".value");
            if (object != nullptr || field != nullptr) {
                if (object == nullptr || field == nullptr) {
                    result.message = paramPrefix + ": a measurement binding needs both .object and .field";
                    return result;
                }
                if (literal != nullptr) {
                    // A hand-edit that added a binding without deleting the
                    // old literal: the file would SHOW a number the build
                    // ignores. Refuse the ambiguity instead of picking.
                    result.message = paramPrefix + ": has both a literal (.value) and a measurement binding (.object/.field)";
                    return result;
                }
                const RecipeOpResult bound = bindParamToMeasurement(document, i, param.id, {*object, *field});
                if (!bound.ok) {
                    result.message = paramPrefix + ": " + bound.message;
                    return result;
                }
            } else if (literal != nullptr) {
                double parsedValue = 0.0;
                if (!parseNumber(*literal, parsedValue)) {
                    result.message = paramPrefix + ".value: not a number: " + *literal;
                    return result;
                }
                const RecipeOpResult set = setParamLiteral(document, i, param.id, parsedValue);
                if (!set.ok) {
                    result.message = paramPrefix + ": " + set.message;
                    return result;
                }
            }
            // No keys at all: the spec default stands — a recipe file only
            // says what differs from the vocabulary's defaults.
        }
    }

    // Strictness sweep: every step.* key in the file must belong to a step
    // and param we actually consumed. A misspelled key would otherwise drop
    // a number silently — guesswork by omission.
    for (const auto &[key, unused] : config) {
        (void)unused;
        // The sweep audits EVERY key: "steps.0.shaper" (plural typo) or
        // "recipe.nme" must reject loudly, not load as an empty recipe —
        // that would be the silent wipe this loader exists to prevent.
        if (key == "recipe.id" || key == "recipe.name") {
            continue;
        }
        const std::size_t indexEnd = key.rfind("step.", 0) == 0 ? key.find('.', 5) : std::string::npos;
        bool known = false;
        if (indexEnd != std::string::npos) {
            const std::string indexText = key.substr(5, indexEnd - 5);
            char *end = nullptr;
            const long index = std::strtol(indexText.c_str(), &end, 10);
            if (end == indexText.c_str() + indexText.size()
                && index >= 0 && static_cast<std::size_t>(index) >= document.steps.size()) {
                // A well-formed key for a step the rebuild never reached:
                // the real problem is a GAP, name the first missing index.
                result.message = "step indices must be contiguous; missing " + stepPrefix(document.steps.size());
                return result;
            }
            if (end == indexText.c_str() + indexText.size()
                && index >= 0 && static_cast<std::size_t>(index) < document.steps.size()) {
                const ShaperStep &step = document.steps[static_cast<std::size_t>(index)];
                const std::string suffix = key.substr(indexEnd + 1);
                if (suffix == "shaper" || suffix == "profile") {
                    known = true;
                } else if (suffix.rfind("param.", 0) == 0) {
                    for (const RecipeParam &param : step.params) {
                        if (suffix == "param." + param.id + ".value"
                            || suffix == "param." + param.id + ".object"
                            || suffix == "param." + param.id + ".field") {
                            known = true;
                            break;
                        }
                    }
                }
            }
        }
        if (!known) {
            result.message = "unknown recipe key: " + key;
            return result;
        }
    }

    document.revision = 0; // a freshly loaded document starts unedited
    result.ok = true;
    result.document = std::move(document);
    return result;
}

} // namespace edi::recipe
