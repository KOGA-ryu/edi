#pragma once

#include "widgets/ShellHost.h"

namespace edi::shell {

// Per-slot constants from docs/ui_restoration_spec.md §2. Sizes are px along
// the panel's resizable axis (width for Left/Right, height for Bottom). Main
// is not a panel — it takes whatever remains — so its spec is all zeros.
struct PanelSpec {
    int defaultSize = 0;
    int minSize = 0;
    int maxSize = 0;        // 0 = unbounded above
    int autoHideBelow = 0;  // hide when the relevant window dimension drops below; 0 = never
    bool openInitially = false;
};

PanelSpec panelSpec(ShellSlot slot);

// What a panel is currently doing, and why — Collapsed is the user's choice,
// AutoHidden is the window's. The distinction is visible chrome: the spec's
// toggle buttons color the two states differently (faint vs warning).
enum class PanelVisibility { Visible, Collapsed, AutoHidden };

// Live, per-panel. Plain data so H5 can serialize it into the workspace TOML.
struct PanelState {
    int size = 0;
    bool collapsed = false; // manual collapse — sticky, survives auto-hide cycles
};

struct ShellPanelsState {
    PanelState left;
    PanelState right;
    PanelState bottom;
};

// Spec defaults: left open at 260, right and bottom closed.
ShellPanelsState defaultShellPanelsState();

// The slot's live state inside the aggregate (Main aliases to a static dummy —
// callers should not ask, but a wrong call degrades instead of crashing).
PanelState &panelStateFor(ShellPanelsState &state, ShellSlot slot);
const PanelState &panelStateFor(const ShellPanelsState &state, ShellSlot slot);

// The four presets from the spec. A preset is a transform over the state, not
// a mode the state lives in afterwards — applying it and then dragging a
// splitter leaves no "preset" to be out of sync with.
// Focus subsumed the legacy "tiny" preset: spec section 2 itself buckets
// focus/tiny as one behavior ("all collapsed"), so a second name was a
// dead enum case no UI could reach.
enum class PanelPreset { Full, Focus, Review };

ShellPanelsState applyPanelPreset(const ShellPanelsState &state, PanelPreset preset);

// Manual collapse, auto-hide, and presets are three independent inputs to this
// one question (spec §2). Precedence: a manual collapse outranks auto-hide —
// the user said "closed", and that intent must survive a window resize.
PanelVisibility panelVisibility(ShellSlot slot, const PanelState &state, int windowWidth, int windowHeight);

// Size clamped into the slot's [min, max] band.
int clampPanelSize(ShellSlot slot, int size);

// F4: keep a floating palette inside its host. The whole palette stays
// visible when it fits; a palette larger than the host pins to the origin
// (so its drag strip - the top-left - is always reachable). Pure math: the
// widget layer feeds it sizes, the same function serves drags, restores,
// and host resizes.
PalettePlacement clampPalettePlacement(
    PalettePlacement placement, int hostWidth, int hostHeight, int paletteWidth, int paletteHeight);

} // namespace edi::shell
