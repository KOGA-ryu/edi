#pragma once

#include "formats/TomlReader.h"
#include "widgets/ShellTheme.h"

#include <QString>
#include <QStringList>

namespace edi::io {

// Profiles are named TOML bundles — today the theme inputs, later whatever
// else a "project profile" needs (default layouts, grid defaults). One file
// per profile under <AppConfigLocation>/profiles/, so a profile is diffable,
// hand-editable, and can be committed next to a project's drawings.

// Shared theme codec: edi.toml's live theme section and every profile file
// use the same keys, so they cannot drift apart.
void writeThemeInputsToConfig(edi::formats::StaticConfig &config, const edi::shell::ShellThemeInputs &inputs);
edi::shell::ShellThemeInputs readThemeInputsFromConfig(const edi::formats::StaticConfig &config);

// Plan-struct decode: a missing or unreadable profile means "keep what you
// have", never an error path.
struct ProfileData {
    bool ok = false;
    edi::shell::ShellThemeInputs inputs;
};

QString defaultProfilesDirPath(); // <AppConfigLocation>/profiles
QStringList listProfiles(const QString &dir);
bool saveProfile(const QString &dir, const QString &name, const edi::shell::ShellThemeInputs &inputs);
ProfileData loadProfile(const QString &dir, const QString &name);

} // namespace edi::io
