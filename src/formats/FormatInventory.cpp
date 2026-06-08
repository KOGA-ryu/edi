#include "formats/FormatInventory.h"

#include <QDir>
#include <QFileInfo>
#include <QMap>
#include <QStringList>

#include <algorithm>

namespace edi::formats {
namespace {

bool hasPrefix(const QString &path, const QString &prefix)
{
    return path == prefix || path.startsWith(prefix + QStringLiteral("/"));
}

bool containsPart(const QString &path, const QString &part)
{
    return path.contains(QStringLiteral("/") + part + QStringLiteral("/"))
        || path.startsWith(part + QStringLiteral("/"));
}

bool containsAny(const QString &path, const QStringList &needles)
{
    for (const QString &needle : needles) {
        if (path.contains(needle)) {
            return true;
        }
    }
    return false;
}

InventoryRow row(
    const QString &path,
    qint64 sizeBytes,
    const QString &category,
    const QString &dataFamily,
    const QString &target,
    const QString &priority,
    const QString &reason)
{
    return {
        normalizedInventoryPath(path),
        extensionForPath(path),
        sizeBytes,
        category,
        dataFamily,
        target,
        priority,
        reason,
    };
}

QString escapeCell(QString value)
{
    value.replace(QStringLiteral("\t"), QStringLiteral(" "));
    value.replace(QStringLiteral("\n"), QStringLiteral(" "));
    value.replace(QStringLiteral("\r"), QStringLiteral(" "));
    return value;
}

bool listEmptyOrContains(const QStringList &values, const QString &value)
{
    return values.isEmpty() || values.contains(value);
}

QString familyKey(const InventoryRow &row)
{
    return QStringList{
        row.category,
        row.dataFamily,
        row.proposedTargetFormat,
        row.migrationPriority,
    }.join(QStringLiteral("\t"));
}

void appendInventoryRows(const QDir &root, const QString &relativeDir, QVector<InventoryRow> *rows)
{
    const QDir dir(relativeDir.isEmpty() ? root.absolutePath() : root.absoluteFilePath(relativeDir));
    const QFileInfoList files = dir.entryInfoList({QStringLiteral("*.json"), QStringLiteral("*.jsonl")}, QDir::Files, QDir::Name);
    for (const QFileInfo &file : files) {
        rows->push_back(classifyInventoryPath(root.relativeFilePath(file.absoluteFilePath()), file.size()));
    }

    const QFileInfoList dirs = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    for (const QFileInfo &child : dirs) {
        const QString name = child.fileName();
        if (name == QStringLiteral(".git") || name == QStringLiteral("build") || name == QStringLiteral("third_party")) {
            continue;
        }
        appendInventoryRows(root, root.relativeFilePath(child.absoluteFilePath()), rows);
    }
}

} // namespace

QString normalizedInventoryPath(const QString &path)
{
    QString normalized = QDir::fromNativeSeparators(QDir::cleanPath(path.trimmed()));
    if (normalized == QStringLiteral(".")) {
        return QStringLiteral(".");
    }
    while (normalized.startsWith(QStringLiteral("./"))) {
        normalized = normalized.mid(2);
    }
    return normalized;
}

QString extensionForPath(const QString &path)
{
    const QString suffix = QFileInfo(path).suffix().toLower();
    return suffix.isEmpty() ? QString() : QStringLiteral(".") + suffix;
}

InventoryRow classifyInventoryPath(const QString &path, qint64 sizeBytes)
{
    const QString p = normalizedInventoryPath(path);
    const QString ext = extensionForPath(p);

    if (hasPrefix(p, QStringLiteral(".git")) || hasPrefix(p, QStringLiteral("build"))) {
        return row(p, sizeBytes, QStringLiteral("build_output_json"), QStringLiteral("build_output"), QStringLiteral("quarantine"), QStringLiteral("ignore"), QStringLiteral("build or repository metadata is excluded from canonical data migration"));
    }

    if (hasPrefix(p, QStringLiteral("third_party")) || containsPart(p, QStringLiteral("third_party"))) {
        return row(p, sizeBytes, QStringLiteral("third_party_json"), QStringLiteral("third_party_payload"), QStringLiteral("quarantine"), QStringLiteral("ignore"), QStringLiteral("third-party data stays outside app-owned format policy"));
    }

    if (containsAny(p, {QStringLiteral("external"), QStringLiteral("compat"), QStringLiteral("import_packet"), QStringLiteral("imported")})) {
        return row(p, sizeBytes, QStringLiteral("external_compat_json"), QStringLiteral("external_compatibility_packet"), QStringLiteral("quarantine"), QStringLiteral("blocked"), QStringLiteral("external compatibility data needs a replacement contract before migration"));
    }

    if (hasPrefix(p, QStringLiteral("tests/artifacts"))) {
        const QString family = ext == QStringLiteral(".jsonl")
            ? QStringLiteral("telemetry_stream")
            : QStringLiteral("generated_metric_report");
        return row(p, sizeBytes, QStringLiteral("internal_generated_json"), family, QStringLiteral("MessagePack"), QStringLiteral("low"), QStringLiteral("generated workflow metrics are machine state and should become compact inspected artifacts"));
    }

    if (hasPrefix(p, QStringLiteral("tests/fixtures"))) {
        return row(p, sizeBytes, QStringLiteral("test_fixture_json"), QStringLiteral("drawing_replay_fixture"), QStringLiteral("MessagePack"), QStringLiteral("medium"), QStringLiteral("drawing tests and golden replay fixtures are machine-readable contract fixtures"));
    }

    if (p == QStringLiteral("data/ui_theme.json")) {
        return row(p, sizeBytes, QStringLiteral("internal_authored_json"), QStringLiteral("theme_config"), QStringLiteral("TOML"), QStringLiteral("high"), QStringLiteral("human-authored static theme config maps cleanly to TOML"));
    }

    if (hasPrefix(p, QStringLiteral("data/project_profiles"))) {
        return row(p, sizeBytes, QStringLiteral("internal_authored_json"), QStringLiteral("project_profile_config"), QStringLiteral("TOML"), QStringLiteral("high"), QStringLiteral("human-authored project profile config should be readable and diffable"));
    }

    if (p == QStringLiteral("data/shell_layout.json") || p == QStringLiteral("data/shell_surface_map.json")) {
        return row(p, sizeBytes, QStringLiteral("internal_authored_json"), QStringLiteral("shell_config"), QStringLiteral("TOML"), QStringLiteral("high"), QStringLiteral("human-authored shell layout and surface config is static app configuration"));
    }

    if (p == QStringLiteral("data/design_principles.json")) {
        return row(p, sizeBytes, QStringLiteral("internal_authored_json"), QStringLiteral("design_config"), QStringLiteral("TOML"), QStringLiteral("medium"), QStringLiteral("design rules are human-authored static config"));
    }

    if (hasPrefix(p, QStringLiteral("data/review_subjects")) || hasPrefix(p, QStringLiteral("data/review_notes"))) {
        return row(p, sizeBytes, QStringLiteral("internal_authored_json"), QStringLiteral("review_packet"), QStringLiteral("TOON"), QStringLiteral("medium"), QStringLiteral("review packets and notes are AI/context-facing handoff data"));
    }

    if (hasPrefix(p, QStringLiteral("data/text_editor"))) {
        return row(p, sizeBytes, QStringLiteral("internal_authored_json"), QStringLiteral("text_document_manifest"), QStringLiteral("TOML"), QStringLiteral("medium"), QStringLiteral("text editor manifests are authored static configuration"));
    }

    if (hasPrefix(p, QStringLiteral("data/maps"))) {
        return row(p, sizeBytes, QStringLiteral("internal_authored_json"), QStringLiteral("map_state"), QStringLiteral("MessagePack"), QStringLiteral("medium"), QStringLiteral("map cells are structured machine state with potential compact storage needs"));
    }

    if (hasPrefix(p, QStringLiteral("data/features"))) {
        if (p.contains(QStringLiteral(".fixture.")) || p.contains(QStringLiteral("/docs/"))) {
            return row(p, sizeBytes, QStringLiteral("internal_authored_json"), QStringLiteral("feature_fixture"), QStringLiteral("MessagePack"), QStringLiteral("medium"), QStringLiteral("feature fixtures are structured machine snapshots, not long-term JSON config"));
        }
        const QString fileName = QFileInfo(p).fileName();
        if (containsAny(fileName, {QStringLiteral("script_"), QStringLiteral("operation_"), QStringLiteral("recipe")})) {
            return row(p, sizeBytes, QStringLiteral("internal_authored_json"), QStringLiteral("authored_recipe"), QStringLiteral("Lua"), QStringLiteral("medium"), QStringLiteral("authored composition and operation chains may need behavior-oriented recipe syntax"));
        }
        return row(p, sizeBytes, QStringLiteral("internal_authored_json"), QStringLiteral("tool_config"), QStringLiteral("TOML"), QStringLiteral("medium"), QStringLiteral("tool registries and presets are human-authored static configuration"));
    }

    if (hasPrefix(p, QStringLiteral("data"))) {
        return row(p, sizeBytes, QStringLiteral("internal_authored_json"), QStringLiteral("app_data"), QStringLiteral("TOML"), QStringLiteral("medium"), QStringLiteral("repo data defaults to authored app configuration until a narrower family is assigned"));
    }

    return row(p, sizeBytes, QStringLiteral("unknown_json"), QStringLiteral("unknown"), QStringLiteral("quarantine"), QStringLiteral("blocked"), QStringLiteral("no migration target assigned until ownership is classified"));
}

QVector<InventoryRow> inventoryRepoJsonFiles(const QString &repoRoot)
{
    QVector<InventoryRow> rows;
    const QDir root(repoRoot.isEmpty() ? QStringLiteral(".") : repoRoot);
    appendInventoryRows(root, QString(), &rows);
    std::sort(rows.begin(), rows.end(), [](const InventoryRow &a, const InventoryRow &b) {
        return a.path < b.path;
    });
    return rows;
}

bool inventoryRowMatchesFilter(const InventoryRow &row, const InventoryFilter &filter)
{
    return listEmptyOrContains(filter.categories, row.category)
        && listEmptyOrContains(filter.dataFamilies, row.dataFamily)
        && listEmptyOrContains(filter.targetFormats, row.proposedTargetFormat)
        && listEmptyOrContains(filter.priorities, row.migrationPriority);
}

QVector<InventoryRow> filterInventoryRows(const QVector<InventoryRow> &rows, const InventoryFilter &filter)
{
    QVector<InventoryRow> result;
    for (const InventoryRow &row : rows) {
        if (inventoryRowMatchesFilter(row, filter)) {
            result.push_back(row);
        }
    }
    return result;
}

QVector<InventoryFamilySummary> inventoryFamilySummaries(const QVector<InventoryRow> &rows, int sampleLimit)
{
    QMap<QString, InventoryFamilySummary> summaries;
    const int cappedSampleLimit = std::max(0, sampleLimit);
    for (const InventoryRow &row : rows) {
        InventoryFamilySummary &summary = summaries[familyKey(row)];
        if (summary.fileCount == 0) {
            summary.category = row.category;
            summary.dataFamily = row.dataFamily;
            summary.proposedTargetFormat = row.proposedTargetFormat;
            summary.migrationPriority = row.migrationPriority;
        }
        summary.fileCount += 1;
        summary.sizeBytes += row.sizeBytes;
        if (summary.samplePaths.size() < cappedSampleLimit) {
            summary.samplePaths.push_back(row.path);
        }
    }

    QVector<InventoryFamilySummary> result = summaries.values().toVector();
    std::sort(result.begin(), result.end(), [](const InventoryFamilySummary &a, const InventoryFamilySummary &b) {
        if (a.proposedTargetFormat != b.proposedTargetFormat) {
            return a.proposedTargetFormat < b.proposedTargetFormat;
        }
        if (a.category != b.category) {
            return a.category < b.category;
        }
        return a.dataFamily < b.dataFamily;
    });
    return result;
}

int inventoryUnknownCount(const QVector<InventoryRow> &rows)
{
    return std::count_if(rows.begin(), rows.end(), [](const InventoryRow &row) {
        return row.category == QStringLiteral("unknown_json") || row.dataFamily == QStringLiteral("unknown");
    });
}

int inventoryBlockedCount(const QVector<InventoryRow> &rows)
{
    return std::count_if(rows.begin(), rows.end(), [](const InventoryRow &row) {
        return row.migrationPriority == QStringLiteral("blocked");
    });
}

QString inventoryRowHeader()
{
    return QStringLiteral("path\textension\tsize_bytes\tcategory\tdata_family\tproposed_target_format\tmigration_priority\treason");
}

QString inventoryRowLine(const InventoryRow &row)
{
    return QStringList{
        escapeCell(row.path),
        escapeCell(row.extension),
        QString::number(row.sizeBytes),
        escapeCell(row.category),
        escapeCell(row.dataFamily),
        escapeCell(row.proposedTargetFormat),
        escapeCell(row.migrationPriority),
        escapeCell(row.reason),
    }.join(QStringLiteral("\t"));
}

QString inventoryFamilySummaryHeader()
{
    return QStringLiteral("category\tdata_family\tproposed_target_format\tmigration_priority\tfile_count\tsize_bytes\tsample_paths");
}

QString inventoryFamilySummaryLine(const InventoryFamilySummary &summary)
{
    QStringList escapedSamples;
    for (const QString &sample : summary.samplePaths) {
        escapedSamples.push_back(escapeCell(sample));
    }
    return QStringList{
        escapeCell(summary.category),
        escapeCell(summary.dataFamily),
        escapeCell(summary.proposedTargetFormat),
        escapeCell(summary.migrationPriority),
        QString::number(summary.fileCount),
        QString::number(summary.sizeBytes),
        escapedSamples.join(QStringLiteral(", ")),
    }.join(QStringLiteral("\t"));
}

QString inventoryFamilySummaryReport(const QVector<InventoryRow> &rows, int sampleLimit)
{
    QStringList lines{inventoryFamilySummaryHeader()};
    for (const InventoryFamilySummary &summary : inventoryFamilySummaries(rows, sampleLimit)) {
        lines.push_back(inventoryFamilySummaryLine(summary));
    }
    return lines.join(QStringLiteral("\n"));
}

QString inventorySummary(const QVector<InventoryRow> &rows)
{
    QMap<QString, int> categories;
    QMap<QString, int> targets;
    for (const InventoryRow &row : rows) {
        categories[row.category] += 1;
        targets[row.proposedTargetFormat] += 1;
    }

    QStringList lines;
    lines.push_back(QStringLiteral("total_files: %1").arg(rows.size()));
    lines.push_back(QStringLiteral("categories:"));
    for (auto it = categories.constBegin(); it != categories.constEnd(); ++it) {
        lines.push_back(QStringLiteral("  %1: %2").arg(it.key()).arg(it.value()));
    }
    lines.push_back(QStringLiteral("proposed_targets:"));
    for (auto it = targets.constBegin(); it != targets.constEnd(); ++it) {
        lines.push_back(QStringLiteral("  %1: %2").arg(it.key()).arg(it.value()));
    }
    return lines.join(QStringLiteral("\n"));
}

} // namespace edi::formats
