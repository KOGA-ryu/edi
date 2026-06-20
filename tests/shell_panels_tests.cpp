#include "widgets/ShellPanels.h"

#include "EdiAssert.h"

using namespace edi::shell;

int main()
{
    // Spec §2 constants, pinned as goldens.
    EDI_CHECK(panelSpec(ShellSlot::Left).defaultSize == 260);
    EDI_CHECK(panelSpec(ShellSlot::Left).minSize == 180);
    EDI_CHECK(panelSpec(ShellSlot::Left).maxSize == 520);
    EDI_CHECK(panelSpec(ShellSlot::Left).autoHideBelow == 640);
    EDI_CHECK(panelSpec(ShellSlot::Left).openInitially);
    EDI_CHECK(panelSpec(ShellSlot::Right).defaultSize == 300);
    EDI_CHECK(panelSpec(ShellSlot::Right).minSize == 160);
    EDI_CHECK(panelSpec(ShellSlot::Right).maxSize == 0);       // unbounded
    EDI_CHECK(panelSpec(ShellSlot::Right).autoHideBelow == 0); // never auto-hides
    EDI_CHECK(!panelSpec(ShellSlot::Right).openInitially);
    EDI_CHECK(panelSpec(ShellSlot::Bottom).defaultSize == 132);
    EDI_CHECK(panelSpec(ShellSlot::Bottom).minSize == 96);
    EDI_CHECK(panelSpec(ShellSlot::Bottom).maxSize == 0); // unbounded: the terminal may become the main window
    EDI_CHECK(panelSpec(ShellSlot::Bottom).autoHideBelow == 520);
    EDI_CHECK(!panelSpec(ShellSlot::Bottom).openInitially);
    EDI_CHECK(panelSpec(ShellSlot::Main).defaultSize == 0); // Main is not a panel

    // Initial state: left open, right and bottom closed, sizes at defaults.
    const ShellPanelsState initial = defaultShellPanelsState();
    EDI_CHECK(!initial.left.collapsed && initial.left.size == 260);
    EDI_CHECK(initial.right.collapsed && initial.right.size == 300);
    EDI_CHECK(initial.bottom.collapsed && initial.bottom.size == 132);

    // Aggregate accessor maps slots to fields.
    ShellPanelsState s = initial;
    panelStateFor(s, ShellSlot::Right).collapsed = false;
    EDI_CHECK(!s.right.collapsed);
    EDI_CHECK(&panelStateFor(s, ShellSlot::Bottom) == &s.bottom);

    // Visibility: manual collapse outranks auto-hide; auto-hide kicks in below
    // the slot's threshold on its own axis only.
    const PanelState open{260, false};
    const PanelState closed{260, true};
    EDI_CHECK(panelVisibility(ShellSlot::Left, open, 1280, 820) == PanelVisibility::Visible);
    EDI_CHECK(panelVisibility(ShellSlot::Left, open, 639, 820) == PanelVisibility::AutoHidden);
    EDI_CHECK(panelVisibility(ShellSlot::Left, closed, 639, 820) == PanelVisibility::Collapsed);
    EDI_CHECK(panelVisibility(ShellSlot::Left, open, 640, 820) == PanelVisibility::Visible);
    // Bottom keys on height, not width.
    EDI_CHECK(panelVisibility(ShellSlot::Bottom, open, 1280, 519) == PanelVisibility::AutoHidden);
    EDI_CHECK(panelVisibility(ShellSlot::Bottom, open, 519, 820) == PanelVisibility::Visible);
    // Right never auto-hides, however small the window.
    EDI_CHECK(panelVisibility(ShellSlot::Right, open, 100, 100) == PanelVisibility::Visible);
    // Main never hides at all.
    EDI_CHECK(panelVisibility(ShellSlot::Main, closed, 100, 100) == PanelVisibility::Visible);

    // Presets are transforms over the state.
    ShellPanelsState custom = initial;
    custom.left.size = 400;
    custom.left.collapsed = true;
    custom.right.size = 350;

    const ShellPanelsState full = applyPanelPreset(custom, PanelPreset::Full);
    EDI_CHECK(!full.left.collapsed && !full.right.collapsed && !full.bottom.collapsed);
    EDI_CHECK(full.left.size == 260 && full.right.size == 300 && full.bottom.size == 132); // sizes reset

    const ShellPanelsState focus = applyPanelPreset(custom, PanelPreset::Focus);
    EDI_CHECK(focus.left.collapsed && focus.right.collapsed && focus.bottom.collapsed);
    EDI_CHECK(focus.left.size == 400); // collapse keeps sizes for reopening
    const ShellPanelsState review = applyPanelPreset(custom, PanelPreset::Review);
    EDI_CHECK(!review.left.collapsed && review.right.collapsed && review.bottom.collapsed);
    EDI_CHECK(review.right.size == 350); // review collapses; it does not resize

    // Size clamping per slot band; Right is unbounded above.
    EDI_CHECK(clampPanelSize(ShellSlot::Left, 100) == 180);
    EDI_CHECK(clampPanelSize(ShellSlot::Left, 9999) == 520);
    EDI_CHECK(clampPanelSize(ShellSlot::Left, 300) == 300);
    EDI_CHECK(clampPanelSize(ShellSlot::Right, 9999) == 9999);
    EDI_CHECK(clampPanelSize(ShellSlot::Right, 10) == 160);
    EDI_CHECK(clampPanelSize(ShellSlot::Bottom, 2000) == 2000); // unbounded above
    EDI_CHECK(clampPanelSize(ShellSlot::Bottom, 10) == 96);

    // F4 palette clamping: fully inside when it fits, origin-pinned when not.
    {
        // Fits: position kept verbatim.
        const PalettePlacement kept =
            clampPalettePlacement({QStringLiteral("p"), 40, 60}, 800, 600, 100, 200);
        EDI_CHECK(kept.x == 40 && kept.y == 60);
        // Off the right/bottom edge: pulled back to the last fully-visible px.
        const PalettePlacement pulled =
            clampPalettePlacement({QStringLiteral("p"), 750, 580}, 800, 600, 100, 200);
        EDI_CHECK(pulled.x == 700 && pulled.y == 400);
        // Negative coordinates snap to the origin.
        const PalettePlacement snapped =
            clampPalettePlacement({QStringLiteral("p"), -5, -9}, 800, 600, 100, 200);
        EDI_CHECK(snapped.x == 0 && snapped.y == 0);
        // Palette larger than the host: origin wins so the drag strip stays
        // reachable (the upper bound is negative here).
        const PalettePlacement pinned =
            clampPalettePlacement({QStringLiteral("p"), 50, 50}, 80, 60, 100, 200);
        EDI_CHECK(pinned.x == 0 && pinned.y == 0);
    }

    return 0;
}
