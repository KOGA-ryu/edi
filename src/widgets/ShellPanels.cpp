#include "widgets/ShellPanels.h"

#include <algorithm>

namespace edi::shell {

PanelSpec panelSpec(ShellSlot slot)
{
    switch (slot) {
    case ShellSlot::Left:
        return PanelSpec{260, 180, 520, 640, true};
    case ShellSlot::Right:
        return PanelSpec{300, 160, 0, 0, false};
    case ShellSlot::Bottom:
        // Max unbounded (user decision 2026-06-10): the terminal must be able
        // to grow over the whole main area and "become the main window".
        return PanelSpec{132, 96, 0, 520, false};
    case ShellSlot::Main:
        break;
    }
    return PanelSpec{};
}

ShellPanelsState defaultShellPanelsState()
{
    ShellPanelsState state;
    state.left = PanelState{panelSpec(ShellSlot::Left).defaultSize, !panelSpec(ShellSlot::Left).openInitially};
    state.right = PanelState{panelSpec(ShellSlot::Right).defaultSize, !panelSpec(ShellSlot::Right).openInitially};
    state.bottom = PanelState{panelSpec(ShellSlot::Bottom).defaultSize, !panelSpec(ShellSlot::Bottom).openInitially};
    return state;
}

PanelState &panelStateFor(ShellPanelsState &state, ShellSlot slot)
{
    static PanelState mainDummy;
    switch (slot) {
    case ShellSlot::Left:
        return state.left;
    case ShellSlot::Right:
        return state.right;
    case ShellSlot::Bottom:
        return state.bottom;
    case ShellSlot::Main:
        break;
    }
    mainDummy = PanelState{};
    return mainDummy;
}

const PanelState &panelStateFor(const ShellPanelsState &state, ShellSlot slot)
{
    return panelStateFor(const_cast<ShellPanelsState &>(state), slot);
}

ShellPanelsState applyPanelPreset(const ShellPanelsState &state, PanelPreset preset)
{
    ShellPanelsState next = state;
    switch (preset) {
    case PanelPreset::Full:
        // Everything open, sizes back to spec defaults.
        next.left = PanelState{panelSpec(ShellSlot::Left).defaultSize, false};
        next.right = PanelState{panelSpec(ShellSlot::Right).defaultSize, false};
        next.bottom = PanelState{panelSpec(ShellSlot::Bottom).defaultSize, false};
        break;
    case PanelPreset::Focus:
    case PanelPreset::Tiny:
        // Both mean "give the main slot everything"; sizes are kept so
        // reopening restores what the user had.
        next.left.collapsed = true;
        next.right.collapsed = true;
        next.bottom.collapsed = true;
        break;
    case PanelPreset::Review:
        next.left.collapsed = false;
        next.right.collapsed = true;
        next.bottom.collapsed = true;
        break;
    }
    return next;
}

PanelVisibility panelVisibility(ShellSlot slot, const PanelState &state, int windowWidth, int windowHeight)
{
    if (slot == ShellSlot::Main) {
        return PanelVisibility::Visible; // Main is not a panel; it never hides
    }
    if (state.collapsed) {
        return PanelVisibility::Collapsed;
    }
    const PanelSpec spec = panelSpec(slot);
    if (spec.autoHideBelow > 0) {
        const int dimension = slot == ShellSlot::Bottom ? windowHeight : windowWidth;
        if (dimension < spec.autoHideBelow) {
            return PanelVisibility::AutoHidden;
        }
    }
    return PanelVisibility::Visible;
}

int clampPanelSize(ShellSlot slot, int size)
{
    const PanelSpec spec = panelSpec(slot);
    const int upper = spec.maxSize > 0 ? spec.maxSize : size;
    return std::clamp(size, spec.minSize, std::max(spec.minSize, upper));
}

PalettePlacement clampPalettePlacement(
    PalettePlacement placement, int hostWidth, int hostHeight, int paletteWidth, int paletteHeight)
{
    // max(0, ...) second: when the palette is larger than the host the upper
    // bound goes negative, and the origin must win so the drag strip stays
    // reachable.
    placement.x = std::min(placement.x, hostWidth - paletteWidth);
    placement.x = std::max(0, placement.x);
    placement.y = std::min(placement.y, hostHeight - paletteHeight);
    placement.y = std::max(0, placement.y);
    return placement;
}

} // namespace edi::shell
