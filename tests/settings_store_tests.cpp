#include "io/SettingsStore.h"

#include <QString>
#include <QTemporaryDir>

#include "EdiAssert.h"
#include <cmath>
#include <string>
#include <vector>

using namespace edi::io;
using edi::formats::StaticConfig;

namespace {

bool nearlyEqual(double a, double b)
{
    return std::abs(a - b) < 0.000001;
}

} // namespace

int main()
{
    // Typed accessors: present values parse; missing keys fall back.
    {
        StaticConfig config;
        setSettingsDouble(config, "grid.width", 11.0);
        setSettingsInt(config, "grid.major_line_every", 4);
        setSettingsBool(config, "snap.grid_enabled", true);
        setSettingsString(config, "grid.preset", "letter");

        EDI_CHECK(nearlyEqual(settingsDouble(config, "grid.width", 1.0), 11.0));
        EDI_CHECK(settingsInt(config, "grid.major_line_every", 1) == 4);
        EDI_CHECK(settingsBool(config, "snap.grid_enabled", false));
        EDI_CHECK(settingsString(config, "grid.preset", "x") == "letter");

        EDI_CHECK(nearlyEqual(settingsDouble(config, "absent", 2.5), 2.5));
        EDI_CHECK(settingsInt(config, "absent", 7) == 7);
        EDI_CHECK(settingsBool(config, "absent", true));
        EDI_CHECK(settingsString(config, "absent", "fallback") == "fallback");
    }

    // Garbage values fall back rather than throwing or returning junk.
    {
        StaticConfig config;
        config["grid.width"] = "not-a-number";
        config["grid.major_line_every"] = "12abc";
        config["snap.grid_enabled"] = "maybe";
        EDI_CHECK(nearlyEqual(settingsDouble(config, "grid.width", 3.0), 3.0));
        EDI_CHECK(settingsInt(config, "grid.major_line_every", 9) == 9);
        EDI_CHECK(settingsBool(config, "snap.grid_enabled", false) == false);
    }

    // Recent files: ordering (most recent first), de-dup, and cap.
    {
        std::vector<std::string> recent;
        recent = pushRecentFile(recent, "a.edidraw");
        recent = pushRecentFile(recent, "b.edidraw");
        recent = pushRecentFile(recent, "c.edidraw");
        EDI_CHECK((recent == std::vector<std::string>{"c.edidraw", "b.edidraw", "a.edidraw"}));
        // Re-adding an existing path moves it to the front without duplicating.
        recent = pushRecentFile(recent, "a.edidraw");
        EDI_CHECK((recent == std::vector<std::string>{"a.edidraw", "c.edidraw", "b.edidraw"}));

        // Cap at 3 drops the oldest.
        std::vector<std::string> capped;
        for (int i = 0; i < 5; ++i) {
            capped = pushRecentFile(capped, std::to_string(i), 3);
        }
        EDI_CHECK((capped == std::vector<std::string>{"4", "3", "2"}));

        // Round-trip through the config (recent.0..N), shorter list clears stragglers.
        StaticConfig config;
        writeRecentFilesToConfig(config, recent);
        EDI_CHECK(recentFilesFromConfig(config) == recent);
        writeRecentFilesToConfig(config, {"only.edidraw"});
        EDI_CHECK((recentFilesFromConfig(config) == std::vector<std::string>{"only.edidraw"}));
    }

    // File round-trip through a temporary edi.toml.
    {
        QTemporaryDir tempDir;
        EDI_CHECK(tempDir.isValid());
        const QString path = tempDir.filePath(QStringLiteral("edi.toml"));

        StaticConfig config;
        setSettingsDouble(config, "grid.width", 8.5);
        setSettingsBool(config, "snap.object_enabled", true);
        setSettingsString(config, "plot.order_mode", "nearest_next");
        writeRecentFilesToConfig(config, {"first.edidraw", "second.edidraw"});
        EDI_CHECK(saveSettingsToPath(path, config));

        const StaticConfig loaded = loadSettingsFromPath(path);
        EDI_CHECK(nearlyEqual(settingsDouble(loaded, "grid.width", 0.0), 8.5));
        EDI_CHECK(settingsBool(loaded, "snap.object_enabled", false));
        EDI_CHECK(settingsString(loaded, "plot.order_mode", "") == "nearest_next");
        EDI_CHECK((recentFilesFromConfig(loaded) == std::vector<std::string>{"first.edidraw", "second.edidraw"}));

        // A missing file yields an empty config (defaults apply downstream).
        const StaticConfig missing = loadSettingsFromPath(tempDir.filePath(QStringLiteral("nope.toml")));
        EDI_CHECK(missing.empty());
        EDI_CHECK(nearlyEqual(settingsDouble(missing, "grid.width", 12.0), 12.0));
    }

    return 0;
}
