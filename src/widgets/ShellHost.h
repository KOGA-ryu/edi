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

// A feature is a registered descriptor, NOT a subclass: the only variation
// point is the buildPanel callable, mirroring the controller's
// kind-and-callable helpers. One feature may fill several slots; buildPanel
// receives the slot so it can return a different panel per slot.
struct FeatureDescriptor {
    QString id;     // "drafting", "text_editor", ...
    QString label;
    // Every slot this feature can fill. Not named `slots`: Qt defines that as
    // a macro (it expands to nothing inside signal/slot declarations), which
    // would corrupt this header in any TU that also includes a Qt header.
    std::vector<ShellSlot> supportedSlots;
    std::function<QWidget *(ShellSlot, FeatureContext &)> buildPanel;
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

struct WorkspaceLayout {
    QString id;     // "drafting", "blender_recipe", ...
    QString label;
    std::vector<SlotBinding> bindings;
};

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
