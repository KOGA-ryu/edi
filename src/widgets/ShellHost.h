#pragma once

#include <QString>

#include <functional>
#include <vector>

class QWidget;
class DrawingDocumentController;

namespace edi::shell {

// A region the shell offers. (Activity rail / status bar are chrome, not
// slots — they belong to the window itself; see docs/shell_architecture.md.)
enum class ShellSlot { Main, Left, Right, Bottom };

// The thin Qt bus features share. With one feature it is just the drawing
// controller; it earns typed document accessors only when feature #2 needs to
// read what feature #1 produced — building the full bus before then would be
// abstraction without a client.
struct FeatureContext {
    DrawingDocumentController *drawingController = nullptr;
};

// A feature is a registered descriptor, NOT a subclass: the variation points
// are callables, mirroring the controller's kind-and-callable helpers. One
// feature may fill several slots; buildPanel receives the slot so it can
// return a different panel per slot.
// A floating palette a feature offers: id (stable, keys the stored
// placement), title (drag-strip text), and the content widget. The shell
// wraps the content in its FloatingPalette frame; the feature never sees
// the frame.
struct FeaturePaletteSpec {
    QString id;
    QString title;
    QWidget *content = nullptr;
};

struct FeatureDescriptor {
    QString id;     // "drafting", "text_editor", ...
    QString label;
    // Every slot this feature can fill. Not named `slots`: Qt defines that as
    // a macro (it expands to nothing inside signal/slot declarations), which
    // would corrupt this header in any TU that also includes a Qt header.
    std::vector<ShellSlot> supportedSlots;
    std::function<QWidget *(ShellSlot, FeatureContext &)> buildPanel;
    // Palettes the feature wants floated over the main area (F4). Called on
    // every mount, after recreateInstance; fresh widgets each time. Optional
    // like the lifecycle hooks — most features have none.
    std::function<std::vector<FeaturePaletteSpec>()> buildPalettes;
    // Instance lifecycle, driven by the shell around every workspace mount:
    // recreateInstance runs BEFORE mounting (a fresh instance whose
    // widget-pointer members die with the old one — nothing can dangle), and
    // instanceMounted runs AFTER (re-feed shell-owned state to the fresh
    // instance). Optional: a stateless feature registers neither. Adding a
    // feature is appending a registry row — the shell has no per-feature code.
    std::function<void()> recreateInstance;
    std::function<void()> instanceMounted;
};

// A table, not a service: adding a feature is appending a row.
struct FeatureRegistry {
    std::vector<FeatureDescriptor> features;
};

// The descriptor registered under `id`, or nullptr.
const FeatureDescriptor *findFeature(const FeatureRegistry &registry, const QString &id);

// Plain data — this is a "job". TOML-serializable when H5 adds persistence.
struct SlotBinding {
    ShellSlot slot;
    QString featureId;
};

// Value semantics: a loaded layout can be compared against the mounted one to
// decide whether switching is actually needed.
inline bool operator==(const SlotBinding &a, const SlotBinding &b)
{
    return a.slot == b.slot && a.featureId == b.featureId;
}

// The tool belt as layout data (user decision 2026-06-10): which tool sits
// in which of the rows x columns slots is part of the *job*, persisted in
// workspace.toml beside panel geometry — not application code. Ids are
// row-major; an empty id is an empty slot. The belt widget renders this; the
// shell never interprets the ids.
struct BeltLayout {
    int rows = 6;
    int columns = 6;
    std::vector<QString> itemIds;
};

inline bool operator==(const BeltLayout &a, const BeltLayout &b)
{
    return a.rows == b.rows && a.columns == b.columns && a.itemIds == b.itemIds;
}

// Where a floating palette sits over the main area (F4). Palettes are
// workspace data like the belt: which palettes exist comes from the mounted
// features; where they sit is the job's geometry, persisted in
// workspace.toml. Coordinates are px from the main area's top-left, clamped
// on apply — a stale position from a larger window degrades to visible.
struct PalettePlacement {
    QString paletteId;
    int x = 12;
    int y = 12;
};

inline bool operator==(const PalettePlacement &a, const PalettePlacement &b)
{
    return a.paletteId == b.paletteId && a.x == b.x && a.y == b.y;
}

struct WorkspaceLayout {
    QString id;     // "drafting", "blender_recipe", ...
    QString label;
    std::vector<SlotBinding> bindings;
    BeltLayout belt;
    std::vector<PalettePlacement> palettes;
};

// The stored placement for `paletteId`, or the default placement when the
// workspace has no opinion yet.
PalettePlacement palettePlacement(const WorkspaceLayout &layout, const QString &paletteId);
// Insert-or-update by palette id (placements are a keyed set, not a list).
void setPalettePlacement(WorkspaceLayout &layout, const PalettePlacement &placement);

struct MountedSlot {
    ShellSlot slot = ShellSlot::Main;
    QWidget *widget = nullptr;
};

// Instantiate each bound feature's panel via its factory. Bindings that name a
// missing feature, a slot the feature cannot fill, or a factory that declines
// (null function or null result) are skipped: a bad layout degrades to an
// emptier shell, never a crash. Order of the result follows binding order.
std::vector<MountedSlot> mountWorkspaceLayout(
    const WorkspaceLayout &layout, const FeatureRegistry &registry, FeatureContext &context);

// The widget mounted into `slot`, or nullptr if the layout left it empty.
QWidget *mountedSlotWidget(const std::vector<MountedSlot> &mounted, ShellSlot slot);

} // namespace edi::shell
