#include "io/ProfileStore.h"

#include "io/SettingsStore.h"

#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>

namespace edi::io {
namespace {

using edi::formats::StaticConfig;
using edi::shell::ShellThemeInputs;

// A profile name becomes a file name; strip anything that could escape the
// profiles directory or break the filesystem. Degrade, don't reject: a messy
// name saves as its cleaned form.
QString sanitizedProfileName(const QString &name)
{
    QString cleaned;
    for (const QChar character : name.trimmed()) {
        if (character.isLetterOrNumber() || character == QLatin1Char('-') || character == QLatin1Char('_')
            || character == QLatin1Char(' ')) {
            cleaned.append(character);
        }
    }
    return cleaned;
}

} // namespace

void writeThemeInputsToConfig(StaticConfig &config, const ShellThemeInputs &inputs)
{
    setSettingsString(config, "theme.base", inputs.base.toStdString());
    setSettingsString(config, "theme.surface", inputs.surface.toStdString());
    setSettingsString(config, "theme.accent", inputs.accent.toStdString());
    setSettingsString(config, "theme.text", inputs.text.toStdString());
    setSettingsString(config, "theme.ui_font", inputs.uiFont.toStdString());
    setSettingsString(config, "theme.code_font", inputs.codeFont.toStdString());
    setSettingsInt(config, "theme.ui_font_size", inputs.uiFontSize);
    setSettingsInt(config, "theme.code_font_size", inputs.codeFontSize);
}

ShellThemeInputs readThemeInputsFromConfig(const StaticConfig &config)
{
    // Fallbacks are the struct's own defaults: an absent or partial theme
    // section degrades to the stock look, never to black-on-black.
    const ShellThemeInputs defaults;
    ShellThemeInputs inputs;
    inputs.base = QString::fromStdString(settingsString(config, "theme.base", defaults.base.toStdString()));
    inputs.surface = QString::fromStdString(settingsString(config, "theme.surface", defaults.surface.toStdString()));
    inputs.accent = QString::fromStdString(settingsString(config, "theme.accent", defaults.accent.toStdString()));
    inputs.text = QString::fromStdString(settingsString(config, "theme.text", defaults.text.toStdString()));
    inputs.uiFont = QString::fromStdString(settingsString(config, "theme.ui_font", defaults.uiFont.toStdString()));
    inputs.codeFont = QString::fromStdString(settingsString(config, "theme.code_font", defaults.codeFont.toStdString()));
    inputs.uiFontSize = settingsInt(config, "theme.ui_font_size", defaults.uiFontSize);
    inputs.codeFontSize = settingsInt(config, "theme.code_font_size", defaults.codeFontSize);
    return inputs;
}

QString defaultProfilesDirPath()
{
    return appConfigFilePath(QStringLiteral("profiles"));
}

QStringList listProfiles(const QString &dir)
{
    QStringList names;
    for (const QFileInfo &info : QDir(dir).entryInfoList({QStringLiteral("*.toml")}, QDir::Files, QDir::Name)) {
        names.push_back(info.completeBaseName());
    }
    return names;
}

bool saveProfile(const QString &dir, const QString &name, const ShellThemeInputs &inputs)
{
    const QString cleaned = sanitizedProfileName(name);
    if (cleaned.isEmpty()) {
        return false;
    }
    StaticConfig config;
    writeThemeInputsToConfig(config, inputs);
    return saveSettingsToPath(QDir(dir).filePath(cleaned + QStringLiteral(".toml")), config);
}

ProfileData loadProfile(const QString &dir, const QString &name)
{
    const QString cleaned = sanitizedProfileName(name);
    ProfileData data;
    if (cleaned.isEmpty()) {
        return data;
    }
    const QString path = QDir(dir).filePath(cleaned + QStringLiteral(".toml"));
    if (!QFileInfo::exists(path)) {
        return data;
    }
    data.inputs = readThemeInputsFromConfig(loadSettingsFromPath(path));
    data.ok = true;
    return data;
}

} // namespace edi::io
