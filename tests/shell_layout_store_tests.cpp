#include "io/ShellLayoutStore.h"

#include "formats/TomlReader.h"
#include "formats/TomlWriter.h"

#include <QTemporaryDir>

#include <cassert>

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

    // Pure round trip: structs -> config -> structs.
    const auto config = workspaceLayoutToConfig(layout, panels);
    const ShellLayoutData decoded = shellLayoutFromConfig(config);
    assert(decoded.ok);
    assert(decoded.layout.id == layout.id);
    assert(decoded.layout.label == layout.label);
    assert(decoded.layout.bindings.size() == 4);
    assert(decoded.layout.bindings[0].slot == ShellSlot::Left);
    assert(decoded.layout.bindings[1].slot == ShellSlot::Main);
    assert(decoded.layout.bindings[3].featureId == QStringLiteral("drafting"));
    assert(decoded.panels.left.size == 320 && decoded.panels.left.collapsed);
    assert(decoded.panels.right.size == 410 && !decoded.panels.right.collapsed);
    assert(decoded.panels.bottom.size == 200);
    // The belt round-trips dense: empty slots come back as empty ids, so the
    // widget's row-major contract never sees a ragged list.
    assert(decoded.layout.belt == layout.belt);
    assert(decoded.layout.belt.itemIds.size() == 36);
    assert(decoded.layout.belt.itemIds[7] == QStringLiteral("circle_tool"));
    assert(decoded.layout.belt.itemIds[8].isEmpty());

    // Through the actual TOML format, not just the in-memory map: the keys
    // must survive write+parse, or the "TOML-serializable" claim is fiction.
    {
        const auto written = edi::formats::writeTomlStaticConfig(config);
        assert(written.ok && written.value);
        const auto parsed = edi::formats::readTomlStaticConfig(*written.value);
        assert(parsed.ok && parsed.value);
        const ShellLayoutData viaToml = shellLayoutFromConfig(*parsed.value);
        assert(viaToml.ok);
        assert(viaToml.layout.bindings.size() == 4);
        assert(viaToml.panels.left.size == 320 && viaToml.panels.left.collapsed);
        assert(viaToml.layout.belt == layout.belt);
    }

    // Empty config: not ok, panels fall back to spec defaults, and the belt
    // falls back to an empty 6x6 (the F3 default grid).
    {
        const ShellLayoutData empty = shellLayoutFromConfig({});
        assert(!empty.ok);
        assert(empty.panels.left.size == 260 && !empty.panels.left.collapsed);
        assert(empty.panels.right.collapsed);
        assert(empty.layout.belt.rows == 6 && empty.layout.belt.columns == 6);
        assert(empty.layout.belt.itemIds.size() == 36);
        for (const QString &id : empty.layout.belt.itemIds) {
            assert(id.isEmpty());
        }
    }

    // Hand-edited damage degrades instead of importing broken geometry:
    // an out-of-band size clamps, a nonsense slot row is skipped.
    {
        edi::formats::StaticConfig broken = config;
        broken["panel.left.size"] = "9000";
        broken["binding.0.slot"] = "middle";
        const ShellLayoutData repaired = shellLayoutFromConfig(broken);
        assert(repaired.ok);
        assert(repaired.panels.left.size == 520);          // clamped to the band
        assert(repaired.layout.bindings.size() == 3);      // bad row dropped, rest kept
        assert(repaired.layout.bindings[0].slot == ShellSlot::Main);
    }

    // Belt damage degrades the same way: dimensions clamp to the legal band,
    // and the item list resizes to what the dimensions promise.
    {
        edi::formats::StaticConfig broken = config;
        broken["belt.rows"] = "600";
        broken["belt.columns"] = "0";
        const ShellLayoutData repaired = shellLayoutFromConfig(broken);
        assert(repaired.layout.belt.rows == 16);    // clamped from 600
        assert(repaired.layout.belt.columns == 1);  // clamped from 0
        assert(repaired.layout.belt.itemIds.size() == 16);
        assert(repaired.layout.belt.itemIds[0] == QStringLiteral("select_move"));
    }

    // File seams: save/load round trip; a missing file is "use your default".
    {
        QTemporaryDir tempDir;
        assert(tempDir.isValid());
        const QString path = tempDir.filePath(QStringLiteral("workspace.toml"));
        assert(saveShellLayoutToPath(path, layout, panels));
        const ShellLayoutData loaded = loadShellLayoutFromPath(path);
        assert(loaded.ok);
        assert(loaded.layout.id == QStringLiteral("drafting"));
        assert(loaded.panels.bottom.size == 200);
        assert(!loadShellLayoutFromPath(tempDir.filePath(QStringLiteral("missing.toml"))).ok);
    }

    return 0;
}
