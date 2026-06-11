// Port-fidelity tests: the expectations below are lifted VERBATIM from the
// prototype (ascii_blender_dryrun_v0) — its pytest cases and the expanded
// moulding points in its generated build_doric_column_v0.py. The C++ port
// must match the v0 numbers exactly (4-decimal contract); these goldens are
// what "port" means.
#include "recipe/RecipeMouldings.h"

#include <cassert>
#include <cmath>
#include <string>
#include <vector>

using namespace edi::recipe;

namespace {

bool near(double a, double b)
{
    return std::abs(a - b) < 1e-9;
}

void assertPoint(const MouldingPoint &point, const char *term, double z, double radius)
{
    assert(point.term == term);
    assert(near(point.z, z));
    assert(near(point.radius, radius));
}

} // namespace

int main()
{
    // ---- test_profile_moulding_compiles_terms_to_points (v0 pytest) ----
    {
        const MouldingCompileResult compiled = compileMouldingSequence("test.profile", {
            {"fillet", 0.25, 2.0, 2.0, {}, {}},
            {"torus", 0.75, {}, 3.0, {}, {}},
            {"scotia", 0.5, {}, 2.4, {}, {}},
        });
        assert(compiled.ok);
        assertPoint(compiled.points.front(), "fillet_start", 0.0, 2.0);
        assert(near(compiled.points.back().z, 1.5));
        assert(near(compiled.points.back().radius, 2.4));
        assert(compiled.points.size() > 3);
    }

    // ---- base.torus_scotia_moulding: the doric base, every point as the
    // prototype generated it (build_doric_column_v0.py line 429) ----
    {
        const MouldingCompileResult base = compileMouldingSequence("base.torus_scotia_moulding", {
            {"fillet", 0.08, 5.55, 5.55, {}, {}},
            {"torus", 0.46, {}, 6.65, {}, {}},
            {"scotia", 0.3, {}, 6.05, {}, {}},
            {"fillet", 0.16, {}, 5.12, {}, {}},
        });
        assert(base.ok);
        assert(base.points.size() == 11);
        assertPoint(base.points[0], "fillet_start", 0.0, 5.55);
        assertPoint(base.points[1], "fillet_01", 0.08, 5.55);
        assertPoint(base.points[2], "torus_01", 0.195, 5.7219);
        assertPoint(base.points[3], "torus_02", 0.31, 6.1);
        assertPoint(base.points[4], "torus_03", 0.425, 6.4781);
        assertPoint(base.points[5], "torus_04", 0.54, 6.65);
        assertPoint(base.points[6], "scotia_01", 0.615, 6.6043);
        assertPoint(base.points[7], "scotia_02", 0.69, 6.4743);
        assertPoint(base.points[8], "scotia_03", 0.765, 6.2796);
        assertPoint(base.points[9], "scotia_04", 0.84, 6.05);
        assertPoint(base.points[10], "fillet_01", 1.0, 5.12);
    }

    // ---- capital.necking_annuli (8 points) ----
    {
        const MouldingCompileResult necking = compileMouldingSequence("capital.necking_annuli", {
            {"fillet", 0.25, 4.4, 5.2, {}, {}},
            {"scotia", 0.27, {}, 4.9, {}, {}},
            {"annulet", 0.38, {}, 5.4, {}, {}},
            {"fillet", 0.3, {}, 5.0, {}, {}},
        });
        assert(necking.ok);
        assert(necking.points.size() == 8);
        assertPoint(necking.points[0], "fillet_start", 0.0, 4.4);
        assertPoint(necking.points[1], "fillet_01", 0.25, 5.2);
        assertPoint(necking.points[2], "scotia_01", 0.3175, 5.1772);
        assertPoint(necking.points[3], "scotia_02", 0.385, 5.1121);
        assertPoint(necking.points[4], "scotia_03", 0.4525, 5.0148);
        assertPoint(necking.points[5], "scotia_04", 0.52, 4.9);
        assertPoint(necking.points[6], "annulet_01", 0.9, 5.4);
        assertPoint(necking.points[7], "fillet_01", 1.2, 5.0);
    }

    // ---- capital.echinus_cushion (12 points; echinus = 6 steps of
    // smoothstep^0.72 — the cushion's characteristic swell) ----
    {
        const MouldingCompileResult echinus = compileMouldingSequence("capital.echinus_cushion", {
            {"cavetto", 0.7, 5.0, 6.0, {}, {}},
            {"echinus", 2.35, {}, 7.9, {}, {}},
            {"fillet", 0.95, {}, 6.9, {}, {}},
        });
        assert(echinus.ok);
        assert(echinus.points.size() == 12);
        assertPoint(echinus.points[0], "cavetto_start", 0.0, 5.0);
        assertPoint(echinus.points[1], "cavetto_01", 0.175, 5.0761);
        assertPoint(echinus.points[2], "cavetto_02", 0.35, 5.2929);
        assertPoint(echinus.points[3], "cavetto_03", 0.525, 5.6173);
        assertPoint(echinus.points[4], "cavetto_04", 0.7, 6.0);
        assertPoint(echinus.points[5], "echinus_01", 1.0917, 6.2917);
        assertPoint(echinus.points[6], "echinus_02", 1.4833, 6.7189);
        assertPoint(echinus.points[7], "echinus_03", 1.875, 7.1535);
        assertPoint(echinus.points[8], "echinus_04", 2.2667, 7.5308);
        assertPoint(echinus.points[9], "echinus_05", 2.6583, 7.7976);
        assertPoint(echinus.points[10], "echinus_06", 3.05, 7.9);
        assertPoint(echinus.points[11], "fillet_01", 4.0, 6.9);
    }

    // ---- radius_delta chains relative to the running radius; an absolute
    // end_radius wins when both appear (v0's precedence) ----
    {
        MouldingSegment delta;
        delta.term = "fillet";
        delta.height = 0.1;
        delta.startRadius = 2.0;
        delta.radiusDelta = 0.5;
        const MouldingCompileResult chained = compileMouldingSequence("delta.profile", {delta});
        assert(chained.ok);
        assert(near(chained.points.back().radius, 2.5));

        MouldingSegment both = delta;
        both.endRadius = 3.0; // absolute beats relative
        const MouldingCompileResult absolute = compileMouldingSequence("delta.profile", {both});
        assert(absolute.ok);
        assert(near(absolute.points.back().radius, 3.0));
    }

    // ---- Failure wording matches v0 (the messages are the diagnosis) ----
    {
        assert(compileMouldingSequence("empty.profile", {}).message
               == "empty.profile needs at least one profile segment.");

        const MouldingCompileResult badTerm = compileMouldingSequence("bad.profile", {
            {"ovolo_misspelt", 0.1, 1.0, {}, {}, {}},
        });
        assert(!badTerm.ok);
        assert(badTerm.message.find("unsupported term") != std::string::npos);
        assert(badTerm.message.find("ovolo_misspelt") != std::string::npos);

        const MouldingCompileResult noStart = compileMouldingSequence("bad.profile", {
            {"torus", 1.0, {}, 3.0, {}, {}},
        });
        assert(!noStart.ok);
        assert(noStart.message == "bad.profile first segment needs start_radius.");

        const MouldingCompileResult flatBand = compileMouldingSequence("bad.profile", {
            {"fillet", 0.0, 1.0, {}, {}, {}},
        });
        assert(!flatBand.ok);
        assert(flatBand.message.find("height must be positive") != std::string::npos);

        const MouldingCompileResult negativeRadius = compileMouldingSequence("bad.profile", {
            {"fillet", 0.1, 1.0, -2.0, {}, {}},
        });
        assert(!negativeRadius.ok);
        assert(negativeRadius.message.find("radii must be positive") != std::string::npos);

        MouldingSegment zeroSteps;
        zeroSteps.term = "torus";
        zeroSteps.height = 0.5;
        zeroSteps.startRadius = 1.0;
        zeroSteps.endRadius = 2.0;
        zeroSteps.steps = 0;
        const MouldingCompileResult badSteps = compileMouldingSequence("bad.profile", {zeroSteps});
        assert(!badSteps.ok);
        assert(badSteps.message.find("steps must be at least 1") != std::string::npos);
    }

    // The vocabulary answers membership as data.
    assert(mouldingTermSupported("cyma_reversa"));
    assert(!mouldingTermSupported("ovolo"));

    return 0;
}
