#include "io/SettingsStore.h"

#include "formats/TomlWriter.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QStandardPaths>

#include <algorithm>
#include <sstream>

namespace edi::io {

double settingsDouble(const StaticConfig &config, const std::string &key, double fallback)
{
    const auto it = config.find(key);
    if (it == config.end()) {
        return fallback;
    }
    try {
        std::size_t consumed = 0;
        const double value = std::stod(it->second, &consumed);
        if (consumed != it->second.size()) {
            return fallback; // trailing garbage
        }
        return value;
    } catch (...) {
        return fallback;
    }
}

int settingsInt(const StaticConfig &config, const std::string &key, int fallback)
{
    const auto it = config.find(key);
    if (it == config.end()) {
        return fallback;
    }
    try {
        std::size_t consumed = 0;
        const int value = std::stoi(it->second, &consumed);
        if (consumed != it->second.size()) {
            return fallback;
        }
        return value;
    } catch (...) {
        return fallback;
    }
}

bool settingsBool(const StaticConfig &config, const std::string &key, bool fallback)
{
    const auto it = config.find(key);
    if (it == config.end()) {
        return fallback;
    }
    if (it->second == "true") {
        return true;
    }
    if (it->second == "false") {
        return false;
    }
    return fallback;
}

std::string settingsString(const StaticConfig &config, const std::string &key, const std::string &fallback)
{
    const auto it = config.find(key);
    return it == config.end() ? fallback : it->second;
}

void setSettingsDouble(StaticConfig &config, const std::string &key, double value)
{
    std::ostringstream stream;
    stream << value;
    config[key] = stream.str();
}

void setSettingsInt(StaticConfig &config, const std::string &key, int value)
{
    config[key] = std::to_string(value);
}

void setSettingsBool(StaticConfig &config, const std::string &key, bool value)
{
    config[key] = value ? "true" : "false";
}

void setSettingsString(StaticConfig &config, const std::string &key, const std::string &value)
{
    config[key] = value;
}

std::vector<std::string> recentFilesFromConfig(const StaticConfig &config)
{
    std::vector<std::string> recent;
    for (std::size_t i = 0;; ++i) {
        const auto it = config.find("recent." + std::to_string(i));
        if (it == config.end()) {
            break;
        }
        recent.push_back(it->second);
    }
    return recent;
}

void writeRecentFilesToConfig(StaticConfig &config, const std::vector<std::string> &recent, std::size_t cap)
{
    // Clear any existing recent.* keys so a shorter list cannot leave stragglers.
    for (auto it = config.begin(); it != config.end();) {
        if (it->first.rfind("recent.", 0) == 0) {
            it = config.erase(it);
        } else {
            ++it;
        }
    }
    const std::size_t count = std::min(recent.size(), cap);
    for (std::size_t i = 0; i < count; ++i) {
        config["recent." + std::to_string(i)] = recent[i];
    }
}

std::vector<std::string> pushRecentFile(std::vector<std::string> recent, const std::string &path, std::size_t cap)
{
    if (path.empty()) {
        return recent;
    }
    recent.erase(std::remove(recent.begin(), recent.end(), path), recent.end());
    recent.insert(recent.begin(), path);
    if (recent.size() > cap) {
        recent.resize(cap);
    }
    return recent;
}

QString appConfigFilePath(const QString &name)
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    return QDir(dir).filePath(name);
}

QString defaultSettingsPath()
{
    return appConfigFilePath(QStringLiteral("edi.toml"));
}

StaticConfig loadSettingsFromPath(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return {};
    }
    const std::string text = QString::fromUtf8(file.readAll()).toStdString();
    auto result = edi::formats::readTomlStaticConfig(text, path.toStdString());
    if (!result.ok || !result.value) {
        return {};
    }
    return *result.value;
}

bool saveSettingsToPath(const QString &path, const StaticConfig &config)
{
    auto written = edi::formats::writeTomlStaticConfig(config, path.toStdString());
    if (!written.ok || !written.value) {
        return false;
    }
    const QFileInfo info(path);
    if (!info.absoluteDir().exists() && !QDir().mkpath(info.absolutePath())) {
        return false;
    }
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        return false;
    }
    file.write(QByteArray::fromStdString(*written.value));
    return file.commit();
}

} // namespace edi::io
