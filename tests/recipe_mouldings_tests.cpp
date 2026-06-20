// Port-fidelity tests: the expectations below are lifted VERBATIM from the
// prototype (ascii_blender_dryrun_v0) — its pytest cases and the expanded
// moulding points in its generated build_doric_column_v0.py. The C++ port
// must match the v0 numbers exactly (4-decimal contract); these goldens are
// what "port" means.
#include "recipe/RecipeMouldings.h"

#include "EdiAssert.h"
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
    EDI_CHECK(point.term == term);
    EDI_CHECK(near(point.z, z));
    EDI_CHECK(near(point.radius, radius));
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
        EDI_CHECK(compiled.ok);
        assertPoint(compiled.points.front(), "fillet_start", 0.0, 2.0);
        EDI_CHECK(near(compiled.points.back().z, 1.5));
        EDI_CHECK(near(compiled.points.back().radius, 2.4));
        EDI_CHECK(compiled.points.size() > 3);
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
        EDI_CHECK(base.ok);
        EDI_CHECK(base.points.size() == 11);
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
        EDI_CHECK(necking.ok);
        EDI_CHECK(necking.points.size() == 8);
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
        EDI_CHECK(echinus.ok);
        EDI_CHECK(echinus.points.size() == 12);
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
        EDI_CHECK(chained.ok);
        EDI_CHECK(near(chained.points.back().radius, 2.5));

        MouldingSegment both = delta;
        both.endRadius = 3.0; // absolute beats relative
        const MouldingCompileResult absolute = compileMouldingSequence("delta.profile", {both});
        EDI_CHECK(absolute.ok);
        EDI_CHECK(near(absolute.points.back().radius, 3.0));
    }

    // ---- Vocabulary rows the doric never exercises: one single-segment
    // compile per term pins its default step count and table spelling.
    // cyma {height 1.0, 2.0→3.0}: 5 default steps → 6 points; at step 2
    // t = 0.4 and smoothstep(0.4) = 0.352, so radius = 2.352. ----
    {
        const MouldingCompileResult cyma = compileMouldingSequence("term.cyma", {
            {"cyma", 1.0, 2.0, 3.0, {}, {}},
        });
        EDI_CHECK(cyma.ok);
        EDI_CHECK(cyma.points.size() == 6);
        assertPoint(cyma.points[2], "cyma_02", 0.4, 2.352);

        const MouldingCompileResult cymaRecta = compileMouldingSequence("term.cyma_recta", {
            {"cyma_recta", 1.0, 2.0, 3.0, {}, {}},
        });
        EDI_CHECK(cymaRecta.ok);
        EDI_CHECK(cymaRecta.points.size() == 6);
        assertPoint(cymaRecta.points[2], "cyma_recta_02", 0.4, 2.352);

        // cyma_reversa's MirroredSmoothstep is algebraically the SAME curve:
        // 1 - smoothstep(1-t) = 3t² - 2t³ = smoothstep(t). The enum row
        // matches v0's branch verbatim, but only its step count and spelling
        // are observably distinct — an ease-swap mutant here is equivalent.
        const MouldingCompileResult cymaReversa = compileMouldingSequence("term.cyma_reversa", {
            {"cyma_reversa", 1.0, 2.0, 3.0, {}, {}},
        });
        EDI_CHECK(cymaReversa.ok);
        EDI_CHECK(cymaReversa.points.size() == 6);
        assertPoint(cymaReversa.points[2], "cyma_reversa_02", 0.4, 2.352);

        // bead has 4 default steps → 5 points. Assert points[1], NOT the
        // midpoint: at t = 0.5 smoothstep equals linear (both 2.5), so the
        // midpoint cannot kill a Smoothstep→Linear ease mutant. At t = 0.25
        // smoothstep(0.25) = 0.15625 → an exact tie that rounds half-to-even
        // to 2.1562, same as v0's python round() (linear would be 2.25).
        const MouldingCompileResult bead = compileMouldingSequence("term.bead", {
            {"bead", 1.0, 2.0, 3.0, {}, {}},
        });
        EDI_CHECK(bead.ok);
        EDI_CHECK(bead.points.size() == 5);
        assertPoint(bead.points[1], "bead_01", 0.25, 2.1562);
    }

    // ---- v0 strips whitespace around terms (python str.strip(), the full
    // ASCII whitespace set — not just space); the point names use the
    // stripped spelling. ----
    {
        const MouldingCompileResult padded = compileMouldingSequence("pad.profile", {
            {" torus ", 0.5, 1.0, 2.0, {}, {}},
        });
        EDI_CHECK(padded.ok);
        EDI_CHECK(padded.points[1].term == "torus_01");

        const MouldingCompileResult wrapped = compileMouldingSequence("pad.profile", {
            {"\ttorus\n", 0.5, 1.0, 2.0, {}, {}},
        });
        EDI_CHECK(wrapped.ok);
        EDI_CHECK(wrapped.points[1].term == "torus_01");
    }

    // ---- Failure wording matches v0 (the messages are the diagnosis) ----
    {
        EDI_CHECK(compileMouldingSequence("empty.profile", {}).message
               == "empty.profile needs at least one profile segment.");

        const MouldingCompileResult badTerm = compileMouldingSequence("bad.profile", {
            {"ovolo_misspelt", 0.1, 1.0, {}, {}, {}},
        });
        EDI_CHECK(!badTerm.ok);
        EDI_CHECK(badTerm.message.find("unsupported term") != std::string::npos);
        EDI_CHECK(badTerm.message.find("ovolo_misspelt") != std::string::npos);

        const MouldingCompileResult noStart = compileMouldingSequence("bad.profile", {
            {"torus", 1.0, {}, 3.0, {}, {}},
        });
        EDI_CHECK(!noStart.ok);
        EDI_CHECK(noStart.message == "bad.profile first segment needs start_radius.");

        const MouldingCompileResult flatBand = compileMouldingSequence("bad.profile", {
            {"fillet", 0.0, 1.0, {}, {}, {}},
        });
        EDI_CHECK(!flatBand.ok);
        EDI_CHECK(flatBand.message.find("height must be positive") != std::string::npos);

        const MouldingCompileResult negativeRadius = compileMouldingSequence("bad.profile", {
            {"fillet", 0.1, 1.0, -2.0, {}, {}},
        });
        EDI_CHECK(!negativeRadius.ok);
        EDI_CHECK(negativeRadius.message.find("radii must be positive") != std::string::npos);

        // Negative START with a positive end: only the startRadius half of
        // the radii check can reject this (the case above re-trips via the
        // end radius, masking a deleted startRadius check).
        const MouldingCompileResult negativeStart = compileMouldingSequence("bad.profile", {
            {"fillet", 0.1, -1.0, 2.0, {}, {}},
        });
        EDI_CHECK(!negativeStart.ok);
        EDI_CHECK(negativeStart.message.find("radii must be positive") != std::string::npos);

        MouldingSegment zeroSteps;
        zeroSteps.term = "torus";
        zeroSteps.height = 0.5;
        zeroSteps.startRadius = 1.0;
        zeroSteps.endRadius = 2.0;
        zeroSteps.steps = 0;
        const MouldingCompileResult badSteps = compileMouldingSequence("bad.profile", {zeroSteps});
        EDI_CHECK(!badSteps.ok);
        EDI_CHECK(badSteps.message.find("steps must be at least 1") != std::string::npos);
    }

    // The vocabulary answers membership as data.
    EDI_CHECK(mouldingTermSupported("cyma_reversa"));
    EDI_CHECK(!mouldingTermSupported("ovolo"));

    return 0;
}
