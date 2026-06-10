#include "io/ShellLayoutStore.h"

#include "io/SettingsStore.h"

#include <optional>
#include <string>

namespace edi::io {
namespace {

using edi::formats::StaticConfig;
using edi::shell::PanelState;
using edi::shell::ShellPanelsState;
using edi::shell::ShellSlot;
using edi::shell::SlotBinding;
using edi::shell::WorkspaceLayout;

// Slots travel as names, not enum integers: a hand-edited TOML must stay
// meaningful when the enum order changes, and "left" is editable by a human in
// a way "1" is not.
std::string shellSlotId(ShellSlot slot)
{
    switch (slot) {
    case ShellSlot::Main:
        return "main";
    case ShellSlot::Left:
        return "left";
    case ShellSlot::Right:
        return "right";
    case ShellSlot::Bottom:
        return "bottom";
    }
    return "main";
}

std::optional<ShellSlot> shellSlotFromId(const std::string &id)
{
    if (id == "main") {
        return ShellSlot::Main;
    }
    if (id == "left") {
        return ShellSlot::Left;
    }
    if (id == "right") {
        return ShellSlot::Right;
    }
    if (id == "bottom") {
        return ShellSlot::Bottom;
    }
    return std::nullopt;
}

void writePanel(StaticConfig &config, const std::string &key, const PanelState &panel)
{
    setSettingsInt(config, "panel." + key + ".size", panel.size);
    setSettingsBool(config, "panel." + key + ".collapsed", panel.collapsed);
}

PanelState readPanel(const StaticConfig &config, const std::string &key, ShellSlot slot, const PanelState &fallback)
{
    PanelState panel;
    // Sizes pass through the same clamp the splitters use, so a hand-edited
    // "size = 9000" degrades to the slot's legal band instead of importing a
    // broken geometry.
    panel.size = edi::shell::clampPanelSize(slot, settingsInt(config, "panel." + key + ".size", fallback.size));
    panel.collapsed = settingsBool(config, "panel." + key + ".collapsed", fallback.collapsed);
    return panel;
}

} // namespace

StaticConfig workspaceLayoutToConfig(const WorkspaceLayout &layout, const ShellPanelsState &panels)
{
    StaticConfig config;
    setSettingsString(config, "workspace.id", layout.id.toStdString());
    setSettingsString(config, "workspace.label", layout.label.toStdString());
    // Lists in StaticConfig are indexed keys (same idiom as recent files):
    // the flat map keeps the TOML reader/writer trivial, and binding order is
    // preserved by the index.
    for (std::size_t i = 0; i < layout.bindings.size(); ++i) {
        const std::string prefix = "binding." + std::to_string(i);
        setSettingsString(config, prefix + ".slot", shellSlotId(layout.bindings[i].slot));
        setSettingsString(config, prefix + ".feature", layout.bindings[i].featureId.toStdString());
    }
    writePanel(config, "left", panels.left);
    writePanel(config, "right", panels.right);
    writePanel(config, "bottom", panels.bottom);
    return config;
}

ShellLayoutData shellLayoutFromConfig(const StaticConfig &config)
{
    ShellLayoutData data;
    data.layout.id = QString::fromStdString(settingsString(config, "workspace.id", ""));
    data.layout.label = QString::fromStdString(settingsString(config, "workspace.label", ""));
    for (std::size_t i = 0;; ++i) {
        const std::string prefix = "binding." + std::to_string(i);
        const std::string slotId = settingsString(config, prefix + ".slot", "");
        const std::string featureId = settingsString(config, prefix + ".feature", "");
        if (slotId.empty() && featureId.empty()) {
            break;
        }
        const std::optional<ShellSlot> slot = shellSlotFromId(slotId);
        if (!slot || featureId.empty()) {
            continue; // a malformed row degrades to "that slot stays empty"
        }
        data.layout.bindings.push_back(SlotBinding{*slot, QString::fromStdString(featureId)});
    }

    const ShellPanelsState defaults = edi::shell::defaultShellPanelsState();
    data.panels.left = readPanel(config, "left", ShellSlot::Left, defaults.left);
    data.panels.right = readPanel(config, "right", ShellSlot::Right, defaults.right);
    data.panels.bottom = readPanel(config, "bottom", ShellSlot::Bottom, defaults.bottom);

    // A layout is usable when it names itself and binds at least one slot;
    // anything less and the caller should prefer its built-in default.
    data.ok = !data.layout.id.isEmpty() && !data.layout.bindings.empty();
    return data;
}

ShellLayoutData loadShellLayoutFromPath(const QString &path)
{
    return shellLayoutFromConfig(loadSettingsFromPath(path));
}

bool saveShellLayoutToPath(const QString &path, const WorkspaceLayout &layout, const ShellPanelsState &panels)
{
    return saveSettingsToPath(path, workspaceLayoutToConfig(layout, panels));
}

} // namespace edi::io
