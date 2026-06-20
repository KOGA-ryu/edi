#include "io/ShellLayoutStore.h"

#include "formats/TomlReader.h"
#include "formats/TomlWriter.h"
#include "io/SettingsStore.h"

#include <QTemporaryDir>

#include "EdiAssert.h"

using namespace edi::io;
using namespace edi::shell;

int main()
{
    // A representative job: drafting across all four slots, customized panels.
    WorkspaceLayout layout;
    layout.id = QStringLiteral("drafting");
    layout.label = QStringLiteral("Drafting");
    layout.bindings = {
        {ShellSlot::Left, QStringLiteral("drafting")},
        {ShellSlot::Main, QStringLiteral("drafting")},
        {ShellSlot::Right, QStringLiteral("drafting")},
        {ShellSlot::Bottom, QStringLiteral("drafting")},
    };
    ShellPanelsState panels = defaultShellPanelsState();
    panels.left.size = 320;
    panels.left.collapsed = true;
    panels.right.size = 410;
    panels.right.collapsed = false;
    panels.bottom.size = 200;
    // A sparse belt: tools in the first row, one in the second, rest empty.
    layout.belt.rows = 6;
    layout.belt.columns = 6;
    layout.belt.itemIds.assign(36, QString());
    layout.belt.itemIds[0] = QStringLiteral("select_move");
    layout.belt.itemIds[1] = QStringLiteral("line_tool");
    layout.belt.itemIds[7] = QStringLiteral("circle_tool");
    // Two frozen quick-bars, pin order preserved (1 was pinned before 0).
    layout.belt.pinnedRows = {1, 0};
    // Modular panels: the user moved layers left and hid calibration.
    layout.panelContent = {{QStringLiteral("layers_document"), QStringLiteral("left")},
                           {QStringLiteral("calibration_document"), QStringLiteral("hidden")}};
    // Two floating palettes with stored positions.
    setPalettePlacement(layout, {QStringLiteral("tool_belt"), 40, 60});
    setPalettePlacement(layout, {QStringLiteral("snap_box"), 300, 12});

    // Pure round trip: structs -> config -> structs.
    const auto config = workspaceLayoutToConfig(layout, panels);
    const ShellLayoutData decoded = shellLayoutFromConfig(config);
    EDI_CHECK(decoded.ok);
    EDI_CHECK(decoded.layout.id == layout.id);
    EDI_CHECK(decoded.layout.label == layout.label);
    EDI_CHECK(decoded.layout.bindings.size() == 4);
    EDI_CHECK(decoded.layout.bindings[0].slot == ShellSlot::Left);
    EDI_CHECK(decoded.layout.bindings[1].slot == ShellSlot::Main);
    EDI_CHECK(decoded.layout.bindings[3].featureId == QStringLiteral("drafting"));
    EDI_CHECK(decoded.panels.left.size == 320 && decoded.panels.left.collapsed);
    EDI_CHECK(decoded.panels.right.size == 410 && !decoded.panels.right.collapsed);
    EDI_CHECK(decoded.panels.bottom.size == 200);
    // The belt round-trips dense: empty slots come back as empty ids, so the
    // widget's row-major contract never sees a ragged list.
    EDI_CHECK(decoded.layout.belt == layout.belt);
    EDI_CHECK(decoded.layout.belt.itemIds.size() == 36);
    EDI_CHECK(decoded.layout.belt.itemIds[7] == QStringLiteral("circle_tool"));
    EDI_CHECK(decoded.layout.belt.itemIds[8].isEmpty());
    // Pins round-trip in pin order, not row order.
    EDI_CHECK((decoded.layout.belt.pinnedRows == std::vector<int>{1, 0}));
    // Panel assignments round-trip; a vandalized slot word drops its row.
    EDI_CHECK(decoded.layout.panelContent == layout.panelContent);
    {
        auto vandalized = config;
        edi::io::setSettingsString(vandalized, "panel_content.2.group", "guides_document");
        edi::io::setSettingsString(vandalized, "panel_content.2.slot", "ceiling");
        const ShellLayoutData survived = shellLayoutFromConfig(vandalized);
        EDI_CHECK(survived.layout.panelContent == layout.panelContent);
    }
    // Forgiving decode: a hand-edited pin outside the belt or repeated is
    // dropped, not an error.
    {
        auto vandalized = config;
        edi::io::setSettingsInt(vandalized, "belt.pin.2", 99); // off the grid
        edi::io::setSettingsInt(vandalized, "belt.pin.3", 1);  // duplicate
        const ShellLayoutData survived = shellLayoutFromConfig(vandalized);
        EDI_CHECK((survived.layout.belt.pinnedRows == std::vector<int>{1, 0}));
    }
    // Palette placements round-trip as a keyed set.
    EDI_CHECK(decoded.layout.palettes == layout.palettes);
    EDI_CHECK(palettePlacement(decoded.layout, QStringLiteral("tool_belt")).x == 40);
    EDI_CHECK(palettePlacement(decoded.layout, QStringLiteral("snap_box")).y == 12);
    // An unknown id answers with the default placement, not a sentinel.
    EDI_CHECK(palettePlacement(decoded.layout, QStringLiteral("missing")).x == 12);
    // setPalettePlacement updates in place rather than appending duplicates.
    {
        WorkspaceLayout updated = decoded.layout;
        setPalettePlacement(updated, {QStringLiteral("tool_belt"), 99, 98});
        EDI_CHECK(updated.palettes.size() == decoded.layout.palettes.size());
        EDI_CHECK(palettePlacement(updated, QStringLiteral("tool_belt")).y == 98);
    }

    // Through the actual TOML format, not just the in-memory map: the keys
    // must survive write+parse, or the "TOML-serializable" claim is fiction.
    {
        const auto written = edi::formats::writeTomlStaticConfig(config);
        EDI_CHECK(written.ok && written.value);
        const auto parsed = edi::formats::readTomlStaticConfig(*written.value);
        EDI_CHECK(parsed.ok && parsed.value);
        const ShellLayoutData viaToml = shellLayoutFromConfig(*parsed.value);
        EDI_CHECK(viaToml.ok);
        EDI_CHECK(viaToml.layout.bindings.size() == 4);
        EDI_CHECK(viaToml.panels.left.size == 320 && viaToml.panels.left.collapsed);
        EDI_CHECK(viaToml.layout.belt == layout.belt);
    }

    // Empty config: not ok, panels fall back to spec defaults, and the belt
    // falls back to an empty 6x6 (the F3 default grid).
    {
        const ShellLayoutData empty = shellLayoutFromConfig({});
        EDI_CHECK(!empty.ok);
        EDI_CHECK(empty.panels.left.size == 260 && !empty.panels.left.collapsed);
        EDI_CHECK(empty.panels.right.collapsed);
        EDI_CHECK(empty.layout.belt.rows == 6 && empty.layout.belt.columns == 6);
        EDI_CHECK(empty.layout.belt.itemIds.size() == 36);
        for (const QString &id : empty.layout.belt.itemIds) {
            EDI_CHECK(id.isEmpty());
        }
    }

    // Hand-edited damage degrades instead of importing broken geometry:
    // an out-of-band size clamps, a nonsense slot row is skipped.
    {
        edi::formats::StaticConfig broken = config;
        broken["panel.left.size"] = "9000";
        broken["binding.0.slot"] = "middle";
        const ShellLayoutData repaired = shellLayoutFromConfig(broken);
        EDI_CHECK(repaired.ok);
        EDI_CHECK(repaired.panels.left.size == 520);          // clamped to the band
        EDI_CHECK(repaired.layout.bindings.size() == 3);      // bad row dropped, rest kept
        EDI_CHECK(repaired.layout.bindings[0].slot == ShellSlot::Main);
    }

    // Belt damage degrades the same way: dimensions clamp to the legal band,
    // and the item list resizes to what the dimensions promise.
    {
        edi::formats::StaticConfig broken = config;
        broken["belt.rows"] = "600";
        broken["belt.columns"] = "0";
        const ShellLayoutData repaired = shellLayoutFromConfig(broken);
        EDI_CHECK(repaired.layout.belt.rows == 16);    // clamped from 600
        EDI_CHECK(repaired.layout.belt.columns == 1);  // clamped from 0
        EDI_CHECK(repaired.layout.belt.itemIds.size() == 16);
        EDI_CHECK(repaired.layout.belt.itemIds[0] == QStringLiteral("select_move"));
    }

    // File seams: save/load round trip; a missing file is "use your default".
    {
        QTemporaryDir tempDir;
        EDI_CHECK(tempDir.isValid());
        const QString path = tempDir.filePath(QStringLiteral("workspace.toml"));
        EDI_CHECK(saveShellLayoutToPath(path, layout, panels));
        const ShellLayoutData loaded = loadShellLayoutFromPath(path);
        EDI_CHECK(loaded.ok);
        EDI_CHECK(loaded.layout.id == QStringLiteral("drafting"));
        EDI_CHECK(loaded.panels.bottom.size == 200);
        EDI_CHECK(!loadShellLayoutFromPath(tempDir.filePath(QStringLiteral("missing.toml"))).ok);
    }

    return 0;
}
