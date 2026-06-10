#pragma once

#include "formats/TomlReader.h"

#include <QString>

#include <cstddef>
#include <string>
#include <vector>

namespace edi::io {

using edi::formats::StaticConfig;

// Typed accessors over the flat string config. All tolerate a missing key or an
// unparseable value by returning the fallback (same spirit as finiteNumber).
double settingsDouble(const StaticConfig &config, const std::string &key, double fallback);
int settingsInt(const StaticConfig &config, const std::string &key, int fallback);
bool settingsBool(const StaticConfig &config, const std::string &key, bool fallback);
std::string settingsString(const StaticConfig &config, const std::string &key, const std::string &fallback);

void setSettingsDouble(StaticConfig &config, const std::string &key, double value);
void setSettingsInt(StaticConfig &config, const std::string &key, int value);
void setSettingsBool(StaticConfig &config, const std::string &key, bool value);
void setSettingsString(StaticConfig &config, const std::string &key, const std::string &value);

// Recent files are stored as recent.0, recent.1, ... most-recent-first.
inline constexpr std::size_t kRecentFilesCap = 10;
std::vector<std::string> recentFilesFromConfig(const StaticConfig &config);
void writeRecentFilesToConfig(StaticConfig &config, const std::vector<std::string> &recent, std::size_t cap = kRecentFilesCap);
// Returns a new list with `path` moved to the front, de-duplicated, capped.
std::vector<std::string> pushRecentFile(std::vector<std::string> recent, const std::string &path, std::size_t cap = kRecentFilesCap);

// File I/O (Qt). loadSettingsFromPath returns an empty config when the file is
// missing or unparseable. defaultSettingsPath is <AppConfigLocation>/edi.toml.
QString defaultSettingsPath();

// <AppConfigLocation>/<name> — the one place the config root is decided;
// every store derives its path through here.
QString appConfigFilePath(const QString &name);
StaticConfig loadSettingsFromPath(const QString &path);
bool saveSettingsToPath(const QString &path, const StaticConfig &config);

} // namespace edi::io
