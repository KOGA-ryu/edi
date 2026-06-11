#include "recipe/RecipeMouldings.h"

#include <cmath>
#include <cstdio>

namespace edi::recipe {

namespace {

// Each term's curve and resolution, one row per term — the vocabulary IS
// this table. Ports v0's CURVED_TERMS/LINEAR_TERMS + default_steps() +
// _interpolate_radius() branches; adding term #11 is adding a row.
enum class MouldingEase {
    Linear,            // fillet, annulet: straight band
    Smoothstep,        // torus, bead, cyma, cyma_recta: t*t*(3-2t)
    CosineEaseIn,      // cavetto, scotia: 1 - cos(t*pi/2) — concave hollow
    EchinusPower,      // echinus: smoothstep(t)^0.72 — the cushion's swell
    MirroredSmoothstep // cyma_reversa: 1 - smoothstep(1-t)
};

struct MouldingTermRow {
    const char *term;
    MouldingEase ease;
    int defaultSteps;
};

constexpr MouldingTermRow kMouldingTerms[] = {
    {"fillet", MouldingEase::Linear, 1},
    {"annulet", MouldingEase::Linear, 1},
    {"torus", MouldingEase::Smoothstep, 4},
    {"bead", MouldingEase::Smoothstep, 4},
    {"cavetto", MouldingEase::CosineEaseIn, 4},
    {"scotia", MouldingEase::CosineEaseIn, 4},
    {"echinus", MouldingEase::EchinusPower, 6},
    {"cyma", MouldingEase::Smoothstep, 5},
    {"cyma_recta", MouldingEase::Smoothstep, 5},
    {"cyma_reversa", MouldingEase::MirroredSmoothstep, 5},
};

const MouldingTermRow *findTerm(const std::string &term)
{
    for (const MouldingTermRow &row : kMouldingTerms) {
        if (term == row.term) {
            return &row;
        }
    }
    return nullptr;
}

double smoothstep(double t)
{
    return t * t * (3.0 - 2.0 * t);
}

double easeValue(MouldingEase ease, double t)
{
    switch (ease) {
    case MouldingEase::Linear:
        return t;
    case MouldingEase::Smoothstep:
        return smoothstep(t);
    case MouldingEase::CosineEaseIn:
        return 1.0 - std::cos(t * M_PI * 0.5);
    case MouldingEase::EchinusPower:
        return std::pow(smoothstep(t), 0.72);
    case MouldingEase::MirroredSmoothstep:
        return 1.0 - smoothstep(1.0 - t);
    }
    return t;
}

// v0 rounds every emitted coordinate to 4 decimals (round-half-even is
// indistinguishable here; v0's python round() on these magnitudes matches
// the classic snprintf rounding for the corpus). Rounding is part of the
// CONTRACT, not cosmetics: the goldens compare these exact values.
double round4(double value)
{
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "%.4f", value);
    return std::strtod(buffer, nullptr);
}

std::string stepSuffix(int step)
{
    char buffer[8];
    std::snprintf(buffer, sizeof(buffer), "%02d", step);
    return buffer;
}

MouldingCompileResult rejected(std::string message)
{
    MouldingCompileResult result;
    result.message = std::move(message);
    return result;
}

} // namespace

bool mouldingTermSupported(const std::string &term)
{
    return findTerm(term) != nullptr;
}

MouldingCompileResult compileMouldingSequence(const std::string &name,
                                              const std::vector<MouldingSegment> &sequence)
{
    if (sequence.empty()) {
        return rejected(name + " needs at least one profile segment.");
    }

    MouldingCompileResult result;
    double currentZ = 0.0;
    std::optional<double> currentRadius;

    for (std::size_t index = 0; index < sequence.size(); ++index) {
        const MouldingSegment &segment = sequence[index];
        const MouldingTermRow *row = findTerm(segment.term);
        if (row == nullptr) {
            return rejected(name + " segment " + std::to_string(index)
                            + " has unsupported term: '" + segment.term + "'");
        }
        if (!(segment.height > 0.0)) {
            return rejected(name + " segment " + std::to_string(index) + " height must be positive.");
        }

        // The bands chain: a segment without its own start continues from
        // the previous segment's end radius.
        const std::optional<double> startRadius =
            segment.startRadius.has_value() ? segment.startRadius : currentRadius;
        if (!startRadius.has_value()) {
            return rejected(name + " first segment needs start_radius.");
        }

        double endRadius = 0.0;
        if (segment.endRadius.has_value()) {
            endRadius = *segment.endRadius;
        } else if (segment.radiusDelta.has_value()) {
            endRadius = *startRadius + *segment.radiusDelta;
        } else {
            endRadius = *startRadius;
        }

        if (!(*startRadius > 0.0) || !(endRadius > 0.0)) {
            return rejected(name + " segment " + std::to_string(index) + " radii must be positive.");
        }

        const int steps = segment.steps.value_or(row->defaultSteps);
        if (steps < 1) {
            return rejected(name + " segment " + std::to_string(index) + " steps must be at least 1.");
        }

        if (result.points.empty()) {
            result.points.push_back({segment.term + "_start", round4(currentZ), round4(*startRadius)});
        }

        for (int step = 1; step <= steps; ++step) {
            const double t = static_cast<double>(step) / steps;
            const double eased = easeValue(row->ease, t);
            result.points.push_back({
                segment.term + "_" + stepSuffix(step),
                round4(currentZ + segment.height * t),
                round4(*startRadius + (endRadius - *startRadius) * eased),
            });
        }

        currentZ += segment.height;
        currentRadius = endRadius;
    }

    result.ok = true;
    return result;
}

} // namespace edi::recipe
