#include "widgets/ShellHost.h"

#include <algorithm>

namespace edi::shell {

const FeatureDescriptor *findFeature(const FeatureRegistry &registry, const QString &id)
{
    const auto it = std::find_if(registry.features.begin(), registry.features.end(),
        [&id](const FeatureDescriptor &feature) { return feature.id == id; });
    return it == registry.features.end() ? nullptr : &*it;
}

std::vector<MountedSlot> mountWorkspaceLayout(
    const WorkspaceLayout &layout, const FeatureRegistry &registry, FeatureContext &context)
{
    std::vector<MountedSlot> mounted;
    for (const SlotBinding &binding : layout.bindings) {
        const FeatureDescriptor *feature = findFeature(registry, binding.featureId);
        if (feature == nullptr || !feature->buildPanel) {
            continue;
        }
        const bool fillsSlot = std::find(feature->supportedSlots.begin(), feature->supportedSlots.end(), binding.slot)
            != feature->supportedSlots.end();
        if (!fillsSlot) {
            continue;
        }
        QWidget *widget = feature->buildPanel(binding.slot, context);
        if (widget == nullptr) {
            continue;
        }
        mounted.push_back(MountedSlot{binding.slot, widget});
    }
    return mounted;
}

QWidget *mountedSlotWidget(const std::vector<MountedSlot> &mounted, ShellSlot slot)
{
    const auto it = std::find_if(mounted.begin(), mounted.end(),
        [slot](const MountedSlot &entry) { return entry.slot == slot; });
    return it == mounted.end() ? nullptr : it->widget;
}

} // namespace edi::shell
