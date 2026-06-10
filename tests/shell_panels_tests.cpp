#include "widgets/ShellPanels.h"

#include <cassert>

using namespace edi::shell;

int main()
{
    // Spec §2 constants, pinned as goldens.
    assert(panelSpec(ShellSlot::Left).defaultSize == 260);
    assert(panelSpec(ShellSlot::Left).minSize == 180);
    assert(panelSpec(ShellSlot::Left).maxSize == 520);
    assert(panelSpec(ShellSlot::Left).autoHideBelow == 640);
    assert(panelSpec(ShellSlot::Left).openInitially);
    assert(panelSpec(ShellSlot::Right).defaultSize == 300);
    assert(panelSpec(ShellSlot::Right).minSize == 160);
    assert(panelSpec(ShellSlot::Right).maxSize == 0);       // unbounded
    assert(panelSpec(ShellSlot::Right).autoHideBelow == 0); // never auto-hides
    assert(!panelSpec(ShellSlot::Right).openInitially);
    assert(panelSpec(ShellSlot::Bottom).defaultSize == 132);
    assert(panelSpec(ShellSlot::Bottom).minSize == 96);
    assert(panelSpec(ShellSlot::Bottom).maxSize == 0); // unbounded: the terminal may become the main window
    assert(panelSpec(ShellSlot::Bottom).autoHideBelow == 520);
    assert(!panelSpec(ShellSlot::Bottom).openInitially);
    assert(panelSpec(ShellSlot::Main).defaultSize == 0); // Main is not a panel

    // Initial state: left open, right and bottom closed, sizes at defaults.
    const ShellPanelsState initial = defaultShellPanelsState();
    assert(!initial.left.collapsed && initial.left.size == 260);
    assert(initial.right.collapsed && initial.right.size == 300);
    assert(initial.bottom.collapsed && initial.bottom.size == 132);

    // Aggregate accessor maps slots to fields.
    ShellPanelsState s = initial;
    panelStateFor(s, ShellSlot::Right).collapsed = false;
    assert(!s.right.collapsed);
    assert(&panelStateFor(s, ShellSlot::Bottom) == &s.bottom);

    // Visibility: manual collapse outranks auto-hide; auto-hide kicks in below
    // the slot's threshold on its own axis only.
    const PanelState open{260, false};
    const PanelState closed{260, true};
    assert(panelVisibility(ShellSlot::Left, open, 1280, 820) == PanelVisibility::Visible);
    assert(panelVisibility(ShellSlot::Left, open, 639, 820) == PanelVisibility::AutoHidden);
    assert(panelVisibility(ShellSlot::Left, closed, 639, 820) == PanelVisibility::Collapsed);
    assert(panelVisibility(ShellSlot::Left, open, 640, 820) == PanelVisibility::Visible);
    // Bottom keys on height, not width.
    assert(panelVisibility(ShellSlot::Bottom, open, 1280, 519) == PanelVisibility::AutoHidden);
    assert(panelVisibility(ShellSlot::Bottom, open, 519, 820) == PanelVisibility::Visible);
    // Right never auto-hides, however small the window.
    assert(panelVisibility(ShellSlot::Right, open, 100, 100) == PanelVisibility::Visible);
    // Main never hides at all.
    assert(panelVisibility(ShellSlot::Main, closed, 100, 100) == PanelVisibility::Visible);

    // Presets are transforms over the state.
    ShellPanelsState custom = initial;
    custom.left.size = 400;
    custom.left.collapsed = true;
    custom.right.size = 350;

    const ShellPanelsState full = applyPanelPreset(custom, PanelPreset::Full);
    assert(!full.left.collapsed && !full.right.collapsed && !full.bottom.collapsed);
    assert(full.left.size == 260 && full.right.size == 300 && full.bottom.size == 132); // sizes reset

    const ShellPanelsState focus = applyPanelPreset(custom, PanelPreset::Focus);
    assert(focus.left.collapsed && focus.right.collapsed && focus.bottom.collapsed);
    assert(focus.left.size == 400); // collapse keeps sizes for reopening
    const ShellPanelsState tiny = applyPanelPreset(custom, PanelPreset::Tiny);
    assert(tiny.left.collapsed && tiny.right.collapsed && tiny.bottom.collapsed);

    const ShellPanelsState review = applyPanelPreset(custom, PanelPreset::Review);
    assert(!review.left.collapsed && review.right.collapsed && review.bottom.collapsed);
    assert(review.right.size == 350); // review collapses; it does not resize

    // Size clamping per slot band; Right is unbounded above.
    assert(clampPanelSize(ShellSlot::Left, 100) == 180);
    assert(clampPanelSize(ShellSlot::Left, 9999) == 520);
    assert(clampPanelSize(ShellSlot::Left, 300) == 300);
    assert(clampPanelSize(ShellSlot::Right, 9999) == 9999);
    assert(clampPanelSize(ShellSlot::Right, 10) == 160);
    assert(clampPanelSize(ShellSlot::Bottom, 2000) == 2000); // unbounded above
    assert(clampPanelSize(ShellSlot::Bottom, 10) == 96);

    return 0;
}
