#include "io/ShellLayoutStore.h"

#include "io/SettingsStore.h"

#include <QDir>
#include <QStandardPaths>

#include <algorithm>
#include <optional>
#include <string>

namespace edi::io {
namespace {

using edi::formats::StaticConfig;
using edi::shell::BeltLayout;
using edi::shell::PanelState;
using edi::shell::ShellPanelsState;
using edi::shell::ShellSlot;
using edi::shell::SlotBinding;
using edi::shell::WorkspaceLayout;

// A hand-edited belt dimension outside this band degrades to the band edge,
// the same forgiving-decode policy as panel sizes. 16 is far above any
// usable belt (the cross renders rows+columns-1 cells) but keeps a typo like
// "rows = 600" from allocating a 600-row grid.
constexpr int kBeltDimensionMin = 1;
constexpr int kBeltDimensionMax = 16;

int clampBeltDimension(int value)
{
    return std::clamp(value, kBeltDimensionMin, kBeltDimensionMax);
}

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
    setSettingsInt(config, "belt.rows", layout.belt.rows);
    setSettingsInt(config, "belt.columns", layout.belt.columns);
    // Slots travel as indexed keys like bindings; empty slots are simply
    // absent keys, so a sparse belt stays a short file.
    for (std::size_t i = 0; i < layout.belt.itemIds.size(); ++i) {
        if (layout.belt.itemIds[i].isEmpty()) {
            continue;
        }
        setSettingsString(config, "belt.item." + std::to_string(i), layout.belt.itemIds[i].toStdString());
    }
    // Pinned quick-bars: indexed keys in pin order, same shape as bindings.
    for (std::size_t i = 0; i < layout.belt.pinnedRows.size(); ++i) {
        setSettingsInt(config, "belt.pin." + std::to_string(i), layout.belt.pinnedRows[i]);
    }
    // Palette positions: indexed rows like bindings, keyed by palette id.
    for (std::size_t i = 0; i < layout.palettes.size(); ++i) {
        const std::string prefix = "palette." + std::to_string(i);
        setSettingsString(config, prefix + ".id", layout.palettes[i].paletteId.toStdString());
        setSettingsInt(config, prefix + ".x", layout.palettes[i].x);
        setSettingsInt(config, prefix + ".y", layout.palettes[i].y);
    }
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

    const BeltLayout beltDefaults;
    data.layout.belt.rows = clampBeltDimension(settingsInt(config, "belt.rows", beltDefaults.rows));
    data.layout.belt.columns = clampBeltDimension(settingsInt(config, "belt.columns", beltDefaults.columns));
    // Read every slot the dimensions promise; absent keys are empty slots.
    // The item list is always exactly rows*columns long so the widget's
    // row-major contract never sees a ragged tail.
    const std::size_t slotCount =
        static_cast<std::size_t>(data.layout.belt.rows) * static_cast<std::size_t>(data.layout.belt.columns);
    data.layout.belt.itemIds.reserve(slotCount);
    for (std::size_t i = 0; i < slotCount; ++i) {
        data.layout.belt.itemIds.push_back(
            QString::fromStdString(settingsString(config, "belt.item." + std::to_string(i), "")));
    }
    // Pins: forgiving decode like everything else here — a hand-edited row
    // outside the belt or repeated is dropped, not an error. (Rows that hold
    // no items get pruned by the widget on apply; occupancy is the widget's
    // knowledge, not the file's.)
    for (std::size_t i = 0;; ++i) {
        const int row = settingsInt(config, "belt.pin." + std::to_string(i), -1);
        if (row < 0) {
            break;
        }
        const bool duplicate = std::find(data.layout.belt.pinnedRows.begin(),
                                         data.layout.belt.pinnedRows.end(), row)
            != data.layout.belt.pinnedRows.end();
        if (row < data.layout.belt.rows && !duplicate) {
            data.layout.belt.pinnedRows.push_back(row);
        }
    }

    for (std::size_t i = 0;; ++i) {
        const std::string prefix = "palette." + std::to_string(i);
        const std::string paletteId = settingsString(config, prefix + ".id", "");
        if (paletteId.empty()) {
            break;
        }
        edi::shell::PalettePlacement placement;
        placement.paletteId = QString::fromStdString(paletteId);
        // Raw load; the widget layer clamps against the live host size on
        // apply (the legal area depends on the window, not on the file).
        placement.x = settingsInt(config, prefix + ".x", placement.x);
        placement.y = settingsInt(config, prefix + ".y", placement.y);
        edi::shell::setPalettePlacement(data.layout, placement);
    }

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

QString defaultWorkspaceLayoutPath()
{
    return appConfigFilePath(QStringLiteral("workspace.toml"));
}

} // namespace edi::io
