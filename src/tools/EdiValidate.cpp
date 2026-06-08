#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QTextStream>

#include <algorithm>
#include <cmath>
#include <functional>
#include <map>
#include <set>

namespace {

QTextStream qout(stdout);
QTextStream qerr(stderr);

using Errors = QStringList;

bool isObject(const QJsonValue &value)
{
    return value.isObject();
}

bool isNonEmptyString(const QJsonValue &value)
{
    return value.isString() && !value.toString().trimmed().isEmpty();
}

bool isInteger(const QJsonValue &value)
{
    if (!value.isDouble()) {
        return false;
    }
    const double number = value.toDouble();
    return std::floor(number) == number;
}

bool isIntegerInRange(const QJsonValue &value, int min, int max)
{
    return isInteger(value) && value.toInt() >= min && value.toInt() <= max;
}

QString cleanPath(QString path)
{
    return QDir::cleanPath(path);
}

QString absolutePath(const QString &path)
{
    return QFileInfo(path).absoluteFilePath();
}

QString parentDirectory(const QString &path)
{
    return QFileInfo(path).absoluteDir().absolutePath();
}

QString repoRootForDataFile(const QString &file)
{
    return QFileInfo(parentDirectory(file)).absoluteDir().absolutePath();
}

bool fileExists(const QString &path)
{
    const QFileInfo info(path);
    return info.exists() && info.isFile();
}

bool directoryExists(const QString &path)
{
    const QFileInfo info(path);
    return info.exists() && info.isDir();
}

QJsonDocument readJsonDocument(const QString &file, QString *error)
{
    QFile input(file);
    if (!input.open(QIODevice::ReadOnly)) {
        *error = input.errorString();
        return {};
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(input.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        *error = parseError.errorString();
        return {};
    }
    return document;
}

bool readJsonObject(const QString &file, QJsonObject *object, QString *error)
{
    const QJsonDocument document = readJsonDocument(file, error);
    if (!document.isObject()) {
        if (error->isEmpty()) {
            *error = "document must be an object";
        }
        return false;
    }
    *object = document.object();
    return true;
}

QStringList argsOrDefault(const QStringList &args, const QString &fallback)
{
    if (!args.isEmpty()) {
        return args;
    }
    return {fallback};
}

void requireString(Errors &errors, const QJsonObject &object, const QString &key, const QString &context)
{
    if (!isNonEmptyString(object.value(key))) {
        errors.push_back(QString("%1: missing string %2").arg(context, key));
    }
}

void requireArray(Errors &errors, const QJsonObject &object, const QString &key, const QString &context)
{
    if (!object.value(key).isArray()) {
        errors.push_back(QString("%1: missing array %2").arg(context, key));
    }
}

void requireNonEmptyArray(Errors &errors, const QJsonObject &object, const QString &key, const QString &context)
{
    if (!object.value(key).isArray() || object.value(key).toArray().isEmpty()) {
        errors.push_back(QString("%1: missing non-empty array %2").arg(context, key));
    }
}

void requireObject(Errors &errors, const QJsonObject &object, const QString &key, const QString &context)
{
    if (!object.value(key).isObject()) {
        errors.push_back(QString("%1: missing object %2").arg(context, key));
    }
}

void requireBoolean(Errors &errors, const QJsonObject &object, const QString &key, const QString &context)
{
    if (!object.value(key).isBool()) {
        errors.push_back(QString("%1: missing boolean %2").arg(context, key));
    }
}

void optionalString(Errors &errors, const QJsonObject &object, const QString &key, const QString &context)
{
    const QJsonValue value = object.value(key);
    if (!value.isUndefined() && !value.isString()) {
        errors.push_back(QString("%1: %2 must be a string when present").arg(context, key));
    }
}

void printErrors(const QString &file, const Errors &errors)
{
    qerr << file << ": failed\n";
    for (const QString &error : errors) {
        qerr << "  - " << error << '\n';
    }
}

int finishFileResult(const QString &file, const Errors &errors, const QString &okMessage)
{
    if (!errors.isEmpty()) {
        printErrors(file, errors);
        return 1;
    }
    qout << okMessage << '\n';
    return 0;
}

std::set<QString> uniqueIds(Errors &errors, const QJsonArray &items, const QString &key, const QString &context)
{
    std::set<QString> ids;
    for (const QJsonValue &value : items) {
        if (!value.isObject()) {
            errors.push_back(QString("%1: entries must be objects").arg(context));
            continue;
        }
        const QJsonObject item = value.toObject();
        const QJsonValue id = item.value(key);
        if (!isNonEmptyString(id)) {
            errors.push_back(QString("%1: entry missing %2").arg(context, key));
            continue;
        }
        const QString idText = id.toString();
        if (ids.contains(idText)) {
            errors.push_back(QString("%1: duplicate %2 %3").arg(context, key, idText));
        }
        ids.insert(idText);
    }
    return ids;
}

QString sha256(const QString &file)
{
    QFile input(file);
    if (!input.open(QIODevice::ReadOnly)) {
        return {};
    }
    return QString::fromLatin1(QCryptographicHash::hash(input.readAll(), QCryptographicHash::Sha256).toHex());
}

QString packetPath(const QString &packetDir, const QString &relativePath)
{
    const QString clean = cleanPath(relativePath);
    if (clean.isEmpty() || QFileInfo(clean).isAbsolute() || clean == ".." || clean.startsWith("../")) {
        return {};
    }
    return QDir(packetDir).absoluteFilePath(clean);
}

QString requirePacketFile(Errors &errors, const QString &packetDir, const QString &relativePath, const QString &context)
{
    const QString fullPath = packetPath(packetDir, relativePath);
    if (fullPath.isEmpty()) {
        errors.push_back(QString("%1: invalid relative path %2").arg(context, relativePath));
        return {};
    }
    if (!fileExists(fullPath)) {
        errors.push_back(QString("%1: file not found %2").arg(context, relativePath));
        return {};
    }
    return fullPath;
}

void validateHash(Errors &errors, const QString &packetDir, const QString &relativePath, const QJsonValue &expectedHash, const QString &context)
{
    const QString fullPath = requirePacketFile(errors, packetDir, relativePath, context);
    if (fullPath.isEmpty()) {
        return;
    }
    static const QRegularExpression hashPattern("^[a-f0-9]{64}$");
    if (!expectedHash.isString() || !hashPattern.match(expectedHash.toString()).hasMatch()) {
        errors.push_back(QString("%1: sha256 must be a lowercase 64-character hex string").arg(context));
        return;
    }
    if (sha256(fullPath) != expectedHash.toString()) {
        errors.push_back(QString("%1: sha256 mismatch for %2").arg(context, relativePath));
    }
}

void parseIndex(Errors &errors, const QString &packetDir)
{
    const QString indexPath = requirePacketFile(errors, packetDir, "index.txt", "index");
    if (indexPath.isEmpty()) {
        return;
    }
    QFile input(indexPath);
    if (!input.open(QIODevice::ReadOnly | QIODevice::Text)) {
        errors.push_back(QString("index: unable to read index.txt: %1").arg(input.errorString()));
        return;
    }
    const QStringList lines = QString::fromUtf8(input.readAll()).split(QRegularExpression("\\r?\\n"));
    static const QRegularExpression recordPattern("^([a-f0-9]{64})\\s+(.+)$");
    int hashLines = 0;
    for (const QString &line : lines) {
        const QRegularExpressionMatch match = recordPattern.match(line);
        if (!match.hasMatch()) {
            continue;
        }
        ++hashLines;
        validateHash(errors, packetDir, match.captured(2), match.captured(1), QString("index %1").arg(match.captured(2)));
    }
    if (hashLines == 0) {
        errors.push_back("index: no sha256 file records found");
    }
}

int validateUiTheme(const QStringList &args)
{
    const QString file = args.value(0, "data/ui_theme.json");
    QJsonObject theme;
    QString error;
    if (!readJsonObject(file, &theme, &error)) {
        qerr << file << ": invalid JSON: " << error << '\n';
        return 1;
    }

    Errors errors;
    static const QRegularExpression hex("^#[0-9a-fA-F]{6}$");
    const std::set<QString> supportedFields = {
        "theme_mode", "base", "surface", "accent", "text",
        "ui_font", "code_font", "ui_font_size", "code_font_size"
    };

    const QString mode = theme.value("theme_mode").toString();
    if (mode != "light" && mode != "dark" && mode != "system") {
        errors.push_back("theme_mode must be light, dark, or system");
    }
    for (const QString &key : {"base", "surface", "accent", "text"}) {
        if (!theme.value(key).isString() || !hex.match(theme.value(key).toString()).hasMatch()) {
            errors.push_back(QString("%1 must be #RRGGBB").arg(key));
        }
    }
    for (const QString &key : {"ui_font", "code_font"}) {
        if (!theme.value(key).isString()) {
            errors.push_back(QString("%1 must be a string").arg(key));
        }
    }
    for (const QString &key : {"ui_font_size", "code_font_size"}) {
        if (!isIntegerInRange(theme.value(key), 9, 28)) {
            errors.push_back(QString("%1 must be an integer from 9 to 28").arg(key));
        }
    }
    for (const QString &key : theme.keys()) {
        if (!supportedFields.contains(key)) {
            errors.push_back(QString("%1 is not a supported theme field").arg(key));
        }
    }

    return finishFileResult(file, errors, QString("%1: ok").arg(file));
}

int validateReviewSubjects(const QStringList &args)
{
    int failures = 0;
    for (const QString &file : argsOrDefault(args, "data/review_subjects/draftsman_ui_taxonomy.json")) {
        QJsonObject document;
        QString error;
        if (!readJsonObject(file, &document, &error)) {
            qerr << file << ": invalid JSON: " << error << '\n';
            ++failures;
            continue;
        }

        Errors errors;
        const QJsonObject subject = document.value("subject").toObject();
        if (!document.value("subject").isObject()) {
            errors.push_back("document: missing subject object");
        } else {
            requireString(errors, subject, "subject_id", "subject");
            requireString(errors, subject, "label", "subject");
            requireString(errors, subject, "root_route_id", "subject");
        }
        requireArray(errors, document, "routes", "document");

        const QJsonArray routes = document.value("routes").toArray();
        std::set<QString> ids;
        for (const QJsonValue &value : routes) {
            const QJsonObject route = value.toObject();
            const QString routeId = route.value("route_id").toString();
            const QString context = QString("route %1").arg(routeId.isEmpty() ? "<unknown>" : routeId);
            requireString(errors, route, "route_id", context);
            requireString(errors, route, "label", context);
            requireString(errors, route, "status", context);
            requireArray(errors, route, "children", context);
            requireArray(errors, route, "objects", context);
            requireArray(errors, route, "code_refs", context);
            requireArray(errors, route, "prompts", context);
            if (!routeId.isEmpty()) {
                if (ids.contains(routeId)) {
                    errors.push_back(QString("%1: duplicate route_id").arg(context));
                }
                ids.insert(routeId);
            }
        }

        const QString rootRouteId = subject.value("root_route_id").toString();
        if (!rootRouteId.isEmpty() && !ids.contains(rootRouteId)) {
            errors.push_back(QString("subject: root_route_id %1 does not exist").arg(rootRouteId));
        }

        for (const QJsonValue &value : routes) {
            const QJsonObject route = value.toObject();
            const QString routeId = route.value("route_id").toString();
            const QString parent = route.value("parent").toString();
            if (!parent.isEmpty() && !ids.contains(parent)) {
                errors.push_back(QString("route %1: parent %2 does not exist").arg(routeId, parent));
            }
            for (const QJsonValue &child : route.value("children").toArray()) {
                const QString childId = child.toString();
                if (!ids.contains(childId)) {
                    errors.push_back(QString("route %1: child %2 does not exist").arg(routeId, childId));
                }
            }
        }

        failures += finishFileResult(file, errors, QString("%1: ok (%2 routes)").arg(file).arg(routes.size()));
    }
    return failures == 0 ? 0 : 1;
}

int validateProjectProfiles(const QStringList &args)
{
    int failures = 0;
    for (const QString &file : argsOrDefault(args, "data/project_profiles/draftsman_blank.json")) {
        QJsonObject document;
        QString error;
        if (!readJsonObject(file, &document, &error)) {
            qerr << file << ": invalid JSON: " << error << '\n';
            ++failures;
            continue;
        }

        Errors errors;
        if (document.value("schema_version").toInt() != 1) {
            errors.push_back("document: schema_version must be 1");
        }
        for (const QString &key : {"profile", "left_panel", "main_workspace", "right_inspector", "bottom_panel", "data_sources", "write_policy"}) {
            requireObject(errors, document, key, "document");
        }
        requireArray(errors, document, "activity_modes", "document");

        const QJsonObject profile = document.value("profile").toObject();
        if (document.value("profile").isObject()) {
            requireString(errors, profile, "profile_id", "profile");
            requireString(errors, profile, "label", "profile");
            requireString(errors, profile, "type", "profile");
            requireString(errors, profile, "default_activity", "profile");
        }

        std::set<QString> modeIds;
        for (const QJsonValue &value : document.value("activity_modes").toArray()) {
            const QJsonObject mode = value.toObject();
            const QString id = mode.value("id").toString();
            const QString context = QString("activity mode %1").arg(id.isEmpty() ? "<unknown>" : id);
            requireString(errors, mode, "id", context);
            requireString(errors, mode, "label", context);
            requireString(errors, mode, "icon", context);
            requireString(errors, mode, "tooltip", context);
            requireBoolean(errors, mode, "enabled", context);
            optionalString(errors, mode, "exclusive_group", context);
            const QString group = mode.value("exclusive_group").toString();
            if (!group.isEmpty() && group != "tool_type" && group != "system") {
                errors.push_back(QString("%1: exclusive_group must be tool_type or system when present").arg(context));
            }
            if (!id.isEmpty()) {
                if (modeIds.contains(id)) {
                    errors.push_back(QString("%1: duplicate id").arg(context));
                }
                modeIds.insert(id);
            }
        }
        const QString defaultActivity = profile.value("default_activity").toString();
        if (!defaultActivity.isEmpty() && !modeIds.contains(defaultActivity)) {
            errors.push_back(QString("profile: default_activity %1 is not declared").arg(defaultActivity));
        }

        const QJsonObject leftPanel = document.value("left_panel").toObject();
        if (document.value("left_panel").isObject()) {
            requireArray(errors, leftPanel, "project_rows", "left_panel");
            for (const QJsonValue &value : leftPanel.value("project_rows").toArray()) {
                const QJsonObject row = value.toObject();
                requireString(errors, row, "label", "left_panel.project_rows row");
                requireString(errors, row, "meta", "left_panel.project_rows row");
            }
        }

        const QJsonObject mainWorkspace = document.value("main_workspace").toObject();
        if (document.value("main_workspace").isObject()) {
            requireString(errors, mainWorkspace, "feature", "main_workspace");
        }

        const QJsonObject rightInspector = document.value("right_inspector").toObject();
        if (document.value("right_inspector").isObject()) {
            requireString(errors, rightInspector, "source", "right_inspector");
            requireObject(errors, rightInspector, "sections", "right_inspector");
            const QJsonObject sections = rightInspector.value("sections").toObject();
            for (const QString &section : {"facts", "selection", "code_refs", "notes", "receipts", "actions"}) {
                if (!sections.value(section).isBool()) {
                    errors.push_back(QString("right_inspector.sections.%1 must be boolean").arg(section));
                }
            }
        }

        const QJsonObject panelDefaults = document.value("panel_defaults").toObject();
        if (document.value("panel_defaults").isObject()) {
            for (const QString &key : {"left_collapsed", "right_collapsed", "bottom_collapsed"}) {
                if (!panelDefaults.value(key).isUndefined() && !panelDefaults.value(key).isBool()) {
                    errors.push_back(QString("panel_defaults.%1 must be boolean when present").arg(key));
                }
            }
        }

        const QJsonObject bottomPanel = document.value("bottom_panel").toObject();
        if (document.value("bottom_panel").isObject()) {
            requireArray(errors, bottomPanel, "tabs", "bottom_panel");
            for (const QJsonValue &tab : bottomPanel.value("tabs").toArray()) {
                if (!isNonEmptyString(tab)) {
                    errors.push_back("bottom_panel.tabs entries must be non-empty strings");
                }
            }
        }

        if (!document.value("custom_actions").isUndefined()) {
            requireArray(errors, document, "custom_actions", "document");
            std::set<QString> actionIds;
            for (const QJsonValue &value : document.value("custom_actions").toArray()) {
                const QJsonObject action = value.toObject();
                const QString id = action.value("id").toString();
                const QString context = QString("custom action %1").arg(id.isEmpty() ? "<unknown>" : id);
                requireString(errors, action, "id", context);
                requireString(errors, action, "label", context);
                requireString(errors, action, "menu", context);
                optionalString(errors, action, "activity", context);
                requireString(errors, action, "handler", context);
                requireBoolean(errors, action, "enabled", context);
                if (!id.isEmpty()) {
                    if (actionIds.contains(id)) {
                        errors.push_back(QString("%1: duplicate id").arg(context));
                    }
                    actionIds.insert(id);
                }
                if (!action.value("args").isUndefined() && !action.value("args").isObject()) {
                    errors.push_back(QString("%1: args must be an object when present").arg(context));
                }
            }
        }

        const QJsonObject dataSources = document.value("data_sources").toObject();
        if (document.value("data_sources").isObject()) {
            const QString feature = mainWorkspace.value("feature").toString();
            const bool doesNotRequireReviewSubject = feature == "blank_canvas" || feature == "csv_map_editor"
                || feature == "drawing_tool_blank" || feature == "text_editor";
            if (doesNotRequireReviewSubject) {
                optionalString(errors, dataSources, "review_subject", "data_sources");
            } else {
                requireString(errors, dataSources, "review_subject", "data_sources");
            }
            for (const QString &key : {"review_notes", "map_csv", "cell_metadata", "text_documents", "drawing_metadata_presets"}) {
                optionalString(errors, dataSources, key, "data_sources");
            }
            if (!doesNotRequireReviewSubject && dataSources.value("review_notes").isString() && dataSources.value("review_notes").toString().trimmed().isEmpty()) {
                errors.push_back("data_sources: review_notes is required unless main_workspace.feature is blank_canvas, csv_map_editor, drawing_tool_blank, or text_editor");
            }
            if (!doesNotRequireReviewSubject && dataSources.value("review_subject").isString() && dataSources.value("review_subject").toString().trimmed().isEmpty()) {
                errors.push_back("data_sources: review_subject is required unless main_workspace.feature is blank_canvas, csv_map_editor, drawing_tool_blank, or text_editor");
            }
            if (feature == "csv_map_editor") {
                requireString(errors, dataSources, "map_csv", "data_sources");
                requireString(errors, dataSources, "cell_metadata", "data_sources");
            }
            if (feature == "text_editor") {
                requireString(errors, dataSources, "text_documents", "data_sources");
            }
        }

        const QJsonObject writePolicy = document.value("write_policy").toObject();
        if (document.value("write_policy").isObject()) {
            requireBoolean(errors, writePolicy, "writes_enabled", "write_policy");
            requireString(errors, writePolicy, "note_storage", "write_policy");
        }

        failures += finishFileResult(file, errors, QString("%1: ok (%2)").arg(file, profile.value("profile_id").toString()));
    }
    return failures == 0 ? 0 : 1;
}

int validateShellLayout(const QStringList &args)
{
    const QString file = args.value(0, "data/shell_layout.json");
    QJsonObject document;
    QString error;
    if (!readJsonObject(file, &document, &error)) {
        qerr << file << ": invalid JSON: " << error << '\n';
        return 1;
    }

    auto assertInteger = [](Errors &errors, const QString &path, const QJsonValue &value, int min, int max) {
        if (!isIntegerInRange(value, min, max)) {
            errors.push_back(QString("%1 must be integer %2-%3").arg(path).arg(min).arg(max));
        }
    };
    auto panelPolicy = [&document, &assertInteger](Errors &errors, const QString &name, const QString &sizeStem, const QString &thresholdKey, int minLimit, int maxLimit, int thresholdMax) {
        const QJsonObject policy = document.value("policy").toObject();
        const QJsonObject item = policy.value(name).toObject();
        if (!policy.value(name).isObject()) {
            errors.push_back(QString("policy.%1 must be an object").arg(name));
            return item;
        }
        const QString minKey = QString("min_%1").arg(sizeStem);
        const QString maxKey = QString("max_%1").arg(sizeStem);
        assertInteger(errors, QString("policy.%1.%2").arg(name, minKey), item.value(minKey), minLimit, maxLimit);
        const int minValue = item.value(minKey).toInt();
        assertInteger(errors, QString("policy.%1.%2").arg(name, maxKey), item.value(maxKey), minValue, maxLimit);
        assertInteger(errors, QString("policy.%1.%2").arg(name, thresholdKey), item.value(thresholdKey), 0, thresholdMax);
        return item;
    };
    auto panel = [&document, &assertInteger](Errors &errors, const QString &name, const QString &sizeKey, int min, int max) {
        const QJsonObject panels = document.value("panels").toObject();
        const QJsonObject item = panels.value(name).toObject();
        if (!panels.value(name).isObject()) {
            errors.push_back(QString("panels.%1 must be an object").arg(name));
            return;
        }
        if (!item.value("collapsed").isBool()) {
            errors.push_back(QString("panels.%1.collapsed must be boolean").arg(name));
        }
        assertInteger(errors, QString("panels.%1.%2").arg(name, sizeKey), item.value(sizeKey), min, max);
    };

    Errors errors;
    const QJsonObject window = document.value("window").toObject();
    if (!document.value("window").isObject()) {
        errors.push_back("window must be an object");
    }
    assertInteger(errors, "window.width", window.value("width"), 520, 2400);
    assertInteger(errors, "window.height", window.value("height"), 420, 1800);
    const QJsonObject leftPolicy = panelPolicy(errors, "left", "width", "auto_hide_below_width", 120, 1200, 2400);
    const QJsonObject rightPolicy = panelPolicy(errors, "right", "width", "auto_hide_below_width", 120, 2400, 2400);
    const QJsonObject bottomPolicy = panelPolicy(errors, "bottom", "height", "auto_hide_below_height", 60, 1800, 1800);
    panel(errors, "left", "width", leftPolicy.value("min_width").toInt(), leftPolicy.value("max_width").toInt());
    panel(errors, "right", "width", rightPolicy.value("min_width").toInt(), rightPolicy.value("max_width").toInt());
    panel(errors, "bottom", "height", bottomPolicy.value("min_height").toInt(), bottomPolicy.value("max_height").toInt());
    const QJsonObject rightPanel = document.value("right_panel").toObject();
    if (!document.value("right_panel").isObject()) {
        errors.push_back("right_panel must be an object");
    }
    if (!rightPanel.value("sections").isObject()) {
        errors.push_back("right_panel.sections must be an object");
    }
    const QJsonObject sections = rightPanel.value("sections").toObject();
    for (const QString &section : {"facts", "selection", "code_refs", "notes", "receipts", "actions"}) {
        if (!sections.value(section).isBool()) {
            errors.push_back(QString("right_panel.sections.%1 must be boolean").arg(section));
        }
    }
    return finishFileResult(file, errors, QString("%1: ok").arg(file));
}

int validateShellSurfaceMap(const QStringList &args)
{
    int failures = 0;
    for (const QString &file : argsOrDefault(args, "data/shell_surface_map.json")) {
        QJsonObject document;
        QString error;
        const QString fullPath = absolutePath(file);
        const QString repoRoot = repoRootForDataFile(fullPath);
        if (!readJsonObject(file, &document, &error)) {
            qerr << file << ": invalid JSON: " << error << '\n';
            ++failures;
            continue;
        }

        auto relativeExists = [&repoRoot](const QString &path) {
            return fileExists(QDir(repoRoot).absoluteFilePath(path));
        };

        Errors errors;
        if (document.value("schema_version").toInt() != 1) {
            errors.push_back("document: schema_version must be 1");
        }
        requireString(errors, document, "map_id", "document");
        requireString(errors, document, "default_profile", "document");
        for (const QString &key : {"entry_documents", "profiles", "surfaces", "integration_workflow"}) {
            requireArray(errors, document, key, "document");
        }
        if (isNonEmptyString(document.value("default_profile")) && !relativeExists(document.value("default_profile").toString())) {
            errors.push_back(QString("document: default_profile not found: %1").arg(document.value("default_profile").toString()));
        }
        for (const QJsonValue &entry : document.value("entry_documents").toArray()) {
            if (!isNonEmptyString(entry)) {
                errors.push_back("entry_documents: entries must be non-empty strings");
            } else if (!relativeExists(entry.toString())) {
                errors.push_back(QString("entry_documents: file not found: %1").arg(entry.toString()));
            }
        }

        std::set<QString> profileIds;
        for (const QJsonValue &value : document.value("profiles").toArray()) {
            const QJsonObject profile = value.toObject();
            const QString id = profile.value("profile_id").toString();
            const QString context = QString("profile %1").arg(id.isEmpty() ? "<unknown>" : id);
            if (!value.isObject()) {
                errors.push_back(QString("%1: must be object").arg(context));
                continue;
            }
            requireString(errors, profile, "profile_id", context);
            requireString(errors, profile, "path", context);
            requireString(errors, profile, "purpose", context);
            requireString(errors, profile, "default_activity", context);
            requireArray(errors, profile, "enabled_features", context);
            if (!id.isEmpty()) {
                if (profileIds.contains(id)) {
                    errors.push_back(QString("%1: duplicate profile_id").arg(context));
                }
                profileIds.insert(id);
            }
            if (isNonEmptyString(profile.value("path")) && !relativeExists(profile.value("path").toString())) {
                errors.push_back(QString("%1: path not found: %2").arg(context, profile.value("path").toString()));
            }
        }

        std::set<QString> surfaceIds;
        for (const QJsonValue &value : document.value("surfaces").toArray()) {
            const QJsonObject surface = value.toObject();
            const QString id = surface.value("surface_id").toString();
            const QString context = QString("surface %1").arg(id.isEmpty() ? "<unknown>" : id);
            if (!value.isObject()) {
                errors.push_back(QString("%1: must be object").arg(context));
                continue;
            }
            requireString(errors, surface, "surface_id", context);
            requireString(errors, surface, "kind", context);
            requireString(errors, surface, "owner_file", context);
            for (const QString &key : {"activity_modes", "inputs", "may_edit", "must_not_edit", "proof"}) {
                requireArray(errors, surface, key, context);
            }
            if (!id.isEmpty()) {
                if (surfaceIds.contains(id)) {
                    errors.push_back(QString("%1: duplicate surface_id").arg(context));
                }
                surfaceIds.insert(id);
            }
            if (isNonEmptyString(surface.value("owner_file")) && !relativeExists(surface.value("owner_file").toString())) {
                errors.push_back(QString("%1: owner_file not found: %2").arg(context, surface.value("owner_file").toString()));
            }
            for (const QJsonValue &proof : surface.value("proof").toArray()) {
                if (!isNonEmptyString(proof)) {
                    errors.push_back(QString("%1: proof entries must be non-empty strings").arg(context));
                } else if (!relativeExists(proof.toString())) {
                    errors.push_back(QString("%1: proof not found: %2").arg(context, proof.toString()));
                }
            }
        }
        if (!surfaceIds.contains("blank_canvas")) {
            errors.push_back("surfaces: missing blank_canvas");
        }
        if (!surfaceIds.contains("main_workspace")) {
            errors.push_back("surfaces: missing main_workspace");
        }
        failures += finishFileResult(file, errors, QString("%1: ok (%2 surfaces)").arg(file).arg(document.value("surfaces").toArray().size()));
    }
    return failures == 0 ? 0 : 1;
}

int validateDesignPrinciples(const QStringList &args)
{
    int failures = 0;
    for (const QString &file : argsOrDefault(args, "data/design_principles.json")) {
        QJsonObject document;
        QString error;
        const QString fullPath = absolutePath(file);
        const QString repoRoot = repoRootForDataFile(fullPath);
        if (!readJsonObject(file, &document, &error)) {
            qerr << file << ": invalid JSON: " << error << '\n';
            ++failures;
            continue;
        }

        Errors errors;
        if (document.value("schema_version").toInt() != 1) {
            errors.push_back("document: schema_version must be 1");
        }
        requireString(errors, document, "principles_id", "document");
        requireString(errors, document, "source_document", "document");
        for (const QString &key : {"core_philosophy", "dex_roles", "design_profiles", "review_checks"}) {
            requireNonEmptyArray(errors, document, key, "document");
        }
        if (isNonEmptyString(document.value("source_document"))
            && !fileExists(QDir(repoRoot).absoluteFilePath(document.value("source_document").toString()))) {
            errors.push_back(QString("document: source_document not found: %1").arg(document.value("source_document").toString()));
        }

        const std::set<QString> principleIds = uniqueIds(errors, document.value("core_philosophy").toArray(), "id", "core_philosophy");
        for (const QJsonValue &value : document.value("core_philosophy").toArray()) {
            if (!value.isObject()) {
                continue;
            }
            const QJsonObject principle = value.toObject();
            const QString context = QString("principle %1").arg(principle.value("id").toString("<unknown>"));
            requireString(errors, principle, "rule", context);
            requireNonEmptyArray(errors, principle, "avoid", context);
        }

        uniqueIds(errors, document.value("dex_roles").toArray(), "role_id", "dex_roles");
        for (const QJsonValue &value : document.value("dex_roles").toArray()) {
            if (!value.isObject()) {
                continue;
            }
            const QJsonObject role = value.toObject();
            const QString context = QString("role %1").arg(role.value("role_id").toString("<unknown>"));
            requireString(errors, role, "purpose", context);
            requireNonEmptyArray(errors, role, "must_read", context);
            requireNonEmptyArray(errors, role, "must_produce", context);
            requireNonEmptyArray(errors, role, "must_not_do", context);
        }

        uniqueIds(errors, document.value("design_profiles").toArray(), "profile_id", "design_profiles");
        for (const QJsonValue &value : document.value("design_profiles").toArray()) {
            if (!value.isObject()) {
                continue;
            }
            const QJsonObject profile = value.toObject();
            const QString context = QString("design profile %1").arg(profile.value("profile_id").toString("<unknown>"));
            requireString(errors, profile, "purpose", context);
            requireNonEmptyArray(errors, profile, "inherits", context);
            requireObject(errors, profile, "defaults", context);
            for (const QJsonValue &inherited : profile.value("inherits").toArray()) {
                if (!principleIds.contains(inherited.toString())) {
                    errors.push_back(QString("%1: unknown inherited principle %2").arg(context, inherited.toString()));
                }
            }
        }

        uniqueIds(errors, document.value("review_checks").toArray(), "check_id", "review_checks");
        for (const QJsonValue &value : document.value("review_checks").toArray()) {
            if (!value.isObject()) {
                continue;
            }
            const QJsonObject check = value.toObject();
            const QString context = QString("review check %1").arg(check.value("check_id").toString("<unknown>"));
            requireString(errors, check, "question", context);
            requireString(errors, check, "fail_if", context);
        }

        failures += finishFileResult(file, errors, QString("%1: ok (%2 principles, %3 profiles)")
            .arg(file).arg(document.value("core_philosophy").toArray().size()).arg(document.value("design_profiles").toArray().size()));
    }
    return failures == 0 ? 0 : 1;
}

int validateCsvMapEditor(const QStringList &args)
{
    const QString profilePath = args.value(0, "data/project_profiles/draftsman_game_guy_map_editor.json");
    const QString repoRoot = QDir::currentPath();
    int failures = 0;
    auto fail = [&failures](const QString &message) {
        qerr << message << '\n';
        ++failures;
    };

    QJsonObject profile;
    QString error;
    if (!readJsonObject(QDir(repoRoot).absoluteFilePath(profilePath), &profile, &error)) {
        fail(QString("%1: invalid profile JSON: %2").arg(profilePath, error));
    }

    if (!profile.isEmpty()) {
        const QJsonObject sources = profile.value("data_sources").toObject();
        const QString csvPath = sources.value("map_csv").toString();
        const QString metadataPath = sources.value("cell_metadata").toString();
        if (csvPath.isEmpty()) {
            fail(QString("%1: missing data_sources.map_csv").arg(profilePath));
        }
        if (metadataPath.isEmpty()) {
            fail(QString("%1: missing data_sources.cell_metadata").arg(profilePath));
        }

        if (!csvPath.isEmpty()) {
            const QString fullCsvPath = QDir(repoRoot).absoluteFilePath(csvPath);
            if (!fileExists(fullCsvPath)) {
                fail(QString("%1: file not found").arg(csvPath));
            } else {
                QFile csv(fullCsvPath);
                if (!csv.open(QIODevice::ReadOnly | QIODevice::Text)) {
                    fail(QString("%1: unable to read: %2").arg(csvPath, csv.errorString()));
                } else {
                    QStringList lines = QString::fromUtf8(csv.readAll()).split(QRegularExpression("\\r?\\n"));
                    QList<QStringList> rows;
                    for (const QString &line : lines) {
                        const QString trimmed = line.trimmed();
                        if (trimmed.isEmpty()) {
                            continue;
                        }
                        QStringList cells;
                        for (const QString &cell : trimmed.split(',')) {
                            cells.push_back(cell.trimmed());
                        }
                        rows.push_back(cells);
                    }
                    if (rows.isEmpty()) {
                        fail(QString("%1: grid is empty").arg(csvPath));
                    }
                    const int width = rows.isEmpty() ? 0 : rows.first().size();
                    if (width == 0) {
                        fail(QString("%1: first row is empty").arg(csvPath));
                    }
                    static const QRegularExpression tokenPattern("^[a-z_]+$");
                    for (int rowIndex = 0; rowIndex < rows.size(); ++rowIndex) {
                        if (rows[rowIndex].size() != width) {
                            fail(QString("%1: row %2 has %3 cells, expected %4").arg(csvPath).arg(rowIndex).arg(rows[rowIndex].size()).arg(width));
                        }
                        for (int col = 0; col < rows[rowIndex].size(); ++col) {
                            if (!tokenPattern.match(rows[rowIndex][col]).hasMatch()) {
                                fail(QString("%1: invalid token at %2,%3: %4").arg(csvPath).arg(rowIndex).arg(col).arg(rows[rowIndex][col]));
                            }
                        }
                    }
                }
            }
        }

        if (!metadataPath.isEmpty()) {
            const QString fullMetadataPath = QDir(repoRoot).absoluteFilePath(metadataPath);
            if (!fileExists(fullMetadataPath)) {
                fail(QString("%1: file not found").arg(metadataPath));
            } else {
                QFile metadata(fullMetadataPath);
                if (!metadata.open(QIODevice::ReadOnly | QIODevice::Text)) {
                    fail(QString("%1: unable to read: %2").arg(metadataPath, metadata.errorString()));
                } else {
                    const QStringList lines = QString::fromUtf8(metadata.readAll()).split(QRegularExpression("\\r?\\n"));
                    for (int i = 0; i < lines.size(); ++i) {
                        const QString line = lines[i].trimmed();
                        if (line.isEmpty()) {
                            continue;
                        }
                        QJsonParseError parseError;
                        const QJsonDocument itemDoc = QJsonDocument::fromJson(line.toUtf8(), &parseError);
                        if (parseError.error != QJsonParseError::NoError || !itemDoc.isObject()) {
                            fail(QString("%1: line %2 invalid JSON: %3").arg(metadataPath).arg(i + 1, 0, 10).arg(parseError.errorString()));
                            continue;
                        }
                        const QJsonObject item = itemDoc.object();
                        if (!isInteger(item.value("row")) || !isInteger(item.value("col"))) {
                            fail(QString("%1: line %2 row/col must be integers").arg(metadataPath).arg(i + 1));
                        }
                        if (!isNonEmptyString(item.value("intent"))) {
                            fail(QString("%1: line %2 missing intent").arg(metadataPath).arg(i + 1));
                        }
                        if (!item.value("tags").isArray()) {
                            fail(QString("%1: line %2 tags must be array").arg(metadataPath).arg(i + 1));
                        }
                        if (!item.value("code_refs").isUndefined() && !item.value("code_refs").isArray()) {
                            fail(QString("%1: line %2 code_refs must be array when present").arg(metadataPath).arg(i + 1));
                        }
                    }
                }
            }
        }
    }

    if (failures == 0) {
        qout << profilePath << ": ok (csv_map_editor)\n";
    }
    return failures == 0 ? 0 : 1;
}

int validateTextEditorDocuments(const QStringList &args)
{
    const QString file = args.value(0, "data/text_editor/documents.json");
    const QString fullPath = absolutePath(file);
    const QString manifestDir = parentDirectory(fullPath);
    const std::set<QString> allowedRoles = {"prompt", "context", "reference", "scratch", "output"};
    QJsonObject document;
    QString error;
    if (!readJsonObject(fullPath, &document, &error)) {
        qerr << file << ": invalid JSON: " << error << '\n';
        return 1;
    }

    Errors errors;
    if (document.value("schema_version").toInt() != 1) {
        errors.push_back("document: schema_version must be 1");
    }
    if (!document.value("documents").isArray()) {
        errors.push_back("document: documents must be an array");
    }
    std::set<QString> seenIds;
    for (const QJsonValue &value : document.value("documents").toArray()) {
        const QJsonObject item = value.toObject();
        const QString id = item.value("id").toString();
        const QString context = QString("document %1").arg(id.isEmpty() ? "<unknown>" : id);
        requireString(errors, item, "id", context);
        requireString(errors, item, "name", context);
        requireString(errors, item, "language", context);
        requireString(errors, item, "path", context);
        const QString role = item.value("role").toString();
        if (!item.value("role").isUndefined() && (!item.value("role").isString() || !allowedRoles.contains(role))) {
            errors.push_back(QString("%1: role must be one of context, output, prompt, reference, scratch").arg(context));
        }
        if (!id.isEmpty()) {
            if (seenIds.contains(id)) {
                errors.push_back(QString("%1: duplicate id").arg(context));
            }
            seenIds.insert(id);
        }
        if (item.value("path").isString()) {
            const QString path = item.value("path").toString();
            const QString clean = cleanPath(path);
            if (QFileInfo(path).isAbsolute() || clean == ".." || clean.startsWith("../")) {
                errors.push_back(QString("%1: path must stay relative to the manifest").arg(context));
            } else if (!fileExists(QDir(manifestDir).absoluteFilePath(clean))) {
                errors.push_back(QString("%1: text file does not exist").arg(context));
            }
        }
    }
    return finishFileResult(file, errors, QString("%1: ok (%2 documents)").arg(file).arg(seenIds.size()));
}

int validateExportPacket(const QStringList &packetDirs)
{
    if (packetDirs.isEmpty()) {
        qerr << "usage: edi_validate export-packet <packet-dir> [...]\n";
        return 1;
    }

    const std::set<QString> allowedRoles = {"prompt", "context", "reference", "scratch", "output"};
    int failures = 0;
    for (const QString &packetDirInput : packetDirs) {
        const QString packetDir = absolutePath(packetDirInput);
        if (!directoryExists(packetDir)) {
            qerr << packetDirInput << ": failed\n";
            qerr << "  - packet: directory not found\n";
            ++failures;
            continue;
        }

        QJsonObject manifest;
        QString error;
        if (!readJsonObject(QDir(packetDir).absoluteFilePath("manifest.json"), &manifest, &error)) {
            qerr << packetDirInput << ": failed\n";
            qerr << "  - manifest: invalid or missing manifest.json: " << error << '\n';
            ++failures;
            continue;
        }

        Errors errors;
        if (manifest.value("schema_version").toInt() != 1) {
            errors.push_back("manifest: schema_version must be 1");
        }
        if (!isNonEmptyString(manifest.value("packet_type"))) {
            errors.push_back("manifest: missing packet_type");
        }
        if (!manifest.value("documents").isArray()) {
            errors.push_back("manifest: documents must be an array");
        }
        if (!manifest.value("files").isArray()) {
            errors.push_back("manifest: files must be an array");
        }

        std::set<QString> seenDocumentIds;
        for (const QJsonValue &value : manifest.value("documents").toArray()) {
            const QJsonObject document = value.toObject();
            const QString id = document.value("id").toString();
            const QString context = QString("document %1").arg(id.isEmpty() ? "<unknown>" : id);
            if (!value.isObject()) {
                errors.push_back(QString("%1: must be object").arg(context));
                continue;
            }
            if (id.trimmed().isEmpty()) {
                errors.push_back(QString("%1: missing id").arg(context));
            } else if (seenDocumentIds.contains(id)) {
                errors.push_back(QString("%1: duplicate id").arg(context));
            } else {
                seenDocumentIds.insert(id);
            }
            if (!isNonEmptyString(document.value("exported_path"))) {
                errors.push_back(QString("%1: missing exported_path").arg(context));
            } else {
                validateHash(errors, packetDir, document.value("exported_path").toString(), document.value("sha256"), context);
            }
            const QString role = document.value("role").toString();
            if (!document.value("role").isUndefined() && (!document.value("role").isString() || !allowedRoles.contains(role))) {
                errors.push_back(QString("%1: role must be one of context, output, prompt, reference, scratch").arg(context));
            }
        }

        std::set<QString> seenFilePaths;
        for (const QJsonValue &value : manifest.value("files").toArray()) {
            const QJsonObject file = value.toObject();
            const QString path = file.value("path").toString();
            const QString context = QString("file %1").arg(path.isEmpty() ? "<unknown>" : path);
            if (!value.isObject()) {
                errors.push_back(QString("%1: must be object").arg(context));
                continue;
            }
            if (path.trimmed().isEmpty()) {
                errors.push_back(QString("%1: missing path").arg(context));
                continue;
            }
            if (seenFilePaths.contains(path)) {
                errors.push_back(QString("%1: duplicate path").arg(context));
            }
            seenFilePaths.insert(path);
            validateHash(errors, packetDir, path, file.value("sha256"), context);
        }

        parseIndex(errors, packetDir);
        if (manifest.value("packet_type").toString() == "dex_handoff") {
            for (const QString &required : {"AGENT_README.txt", "prompt.txt", "context.txt", "all_documents.txt"}) {
                requirePacketFile(errors, packetDir, required, "dex_handoff");
            }
            if (!manifest.value("handoff_files").isObject()) {
                errors.push_back("dex_handoff: missing handoff_files object");
            }
        }

        failures += finishFileResult(packetDirInput, errors, QString("%1: ok (%2, %3 documents)")
            .arg(packetDirInput, manifest.value("packet_type").toString()).arg(seenDocumentIds.size()));
    }
    return failures == 0 ? 0 : 1;
}

int usage()
{
    qerr << "usage: edi_validate <subcommand> [file ...]\n";
    qerr << "subcommands: ui-theme, review-subjects, project-profiles, shell-layout, shell-surface-map, "
            "design-principles, csv-map-editor, text-editor-documents, export-packet\n";
    return 1;
}

} // namespace

int main(int argc, char **argv)
{
    QStringList args;
    for (int i = 1; i < argc; ++i) {
        args.push_back(QString::fromLocal8Bit(argv[i]));
    }
    if (args.isEmpty()) {
        return usage();
    }

    const QString subcommand = args.takeFirst();
    const std::map<QString, std::function<int(const QStringList &)>> handlers = {
        {"ui-theme", validateUiTheme},
        {"review-subjects", validateReviewSubjects},
        {"project-profiles", validateProjectProfiles},
        {"shell-layout", validateShellLayout},
        {"shell-surface-map", validateShellSurfaceMap},
        {"design-principles", validateDesignPrinciples},
        {"csv-map-editor", validateCsvMapEditor},
        {"text-editor-documents", validateTextEditorDocuments},
        {"export-packet", validateExportPacket},
    };

    const auto it = handlers.find(subcommand);
    if (it == handlers.end()) {
        return usage();
    }
    return it->second(args);
}
