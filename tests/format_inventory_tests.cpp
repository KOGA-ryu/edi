#include "formats/FormatInventory.h"

#include <iostream>

namespace {

void require(bool condition, const char *message)
{
    if (!condition) {
        std::cerr << "failed: " << message << '\n';
        std::exit(1);
    }
}

void requireRow(
    const QString &path,
    const QString &category,
    const QString &family,
    const QString &target,
    const QString &priority)
{
    const edi::formats::InventoryRow row = edi::formats::classifyInventoryPath(path, 42);
    require(row.category == category, qPrintable(path + QStringLiteral(" category")));
    require(row.dataFamily == family, qPrintable(path + QStringLiteral(" family")));
    require(row.proposedTargetFormat == target, qPrintable(path + QStringLiteral(" target")));
    require(row.migrationPriority == priority, qPrintable(path + QStringLiteral(" priority")));
    require(row.sizeBytes == 42, "size is preserved");
}

edi::formats::InventorySourceControlIndex sampleSourceControlIndex()
{
    edi::formats::InventorySourceControlIndex index;
    index.available = true;
    index.trackedPaths.insert("data/ui_theme.json");
    index.trackedPaths.insert("tests/fixtures/drawing_tool_scripts/line_create_basic.json");
    index.ignoredPaths.insert("tests/artifacts/drawing_metrics/raw/click_run.jsonl");
    return index;
}

} // namespace

int main()
{
    require(edi::formats::normalizedInventoryPath("./data/ui_theme.json") == "data/ui_theme.json", "normalizes leading dot path");
    require(edi::formats::extensionForPath("events.JSONL") == ".jsonl", "extension is normalized");

    requireRow("data/ui_theme.json", "internal_authored_json", "theme_config", "TOML", "high");
    requireRow("data/project_profiles/draftsman_blank.json", "internal_authored_json", "project_profile_config", "TOML", "high");
    requireRow("data/shell_layout.json", "internal_authored_json", "shell_config", "TOML", "high");
    requireRow("data/design_principles.json", "internal_authored_json", "design_config", "TOML", "medium");
    requireRow("data/features/blender_recipe_lab/operation_chain.json", "internal_authored_json", "authored_recipe", "Lua", "medium");
    requireRow("data/features/drawing_tool/tool_registry.json", "internal_authored_json", "tool_config", "TOML", "medium");
    requireRow("data/features/blender_recipe_lab/face_mask_grid_v0.fixture.json", "internal_authored_json", "feature_fixture", "MessagePack", "medium");
    requireRow("data/maps/game_guy/starter_cells.jsonl", "internal_authored_json", "map_state", "MessagePack", "medium");
    requireRow("data/review_subjects/draftsman_ui_taxonomy.json", "internal_authored_json", "review_packet", "TOON", "medium");
    requireRow("data/text_editor/documents.json", "internal_authored_json", "text_document_manifest", "TOML", "medium");
    requireRow("tests/fixtures/drawing_tool_scripts/line_create_basic.json", "test_fixture_json", "drawing_replay_fixture", "MessagePack", "medium");
    requireRow("tests/artifacts/drawing_metrics/raw/click_run.jsonl", "internal_generated_json", "telemetry_stream", "MessagePack", "low");
    requireRow("third_party/vendor/manifest.json", "third_party_json", "third_party_payload", "quarantine", "ignore");
    requireRow("build/some/generated.json", "build_output_json", "build_output", "quarantine", "ignore");
    requireRow("imports/external_compat/sample.json", "external_compat_json", "external_compatibility_packet", "quarantine", "blocked");
    requireRow("misc/unowned.json", "unknown_json", "unknown", "quarantine", "blocked");

    const QString header = edi::formats::inventoryRowHeader();
    require(header.contains("proposed_target_format"), "header includes target field");
    require(header.contains("repository_state"), "header includes repository state");
    require(edi::formats::inventoryRowLine(edi::formats::classifyInventoryPath("data/ui_theme.json", 10)).contains("theme_config"), "row line includes family");

    const edi::formats::InventorySourceControlIndex index = sampleSourceControlIndex();
    const edi::formats::InventoryRow trackedConfig = edi::formats::withRepositoryState(edi::formats::classifyInventoryPath("data/ui_theme.json", 10), index);
    require(trackedConfig.repositoryState == "tracked", "tracked config is marked tracked");
    require(trackedConfig.migrationScope == "canonical_candidate", "tracked config is canonical candidate");
    const edi::formats::InventoryRow trackedFixture = edi::formats::withRepositoryState(edi::formats::classifyInventoryPath("tests/fixtures/drawing_tool_scripts/line_create_basic.json", 11), index);
    require(trackedFixture.repositoryState == "tracked", "tracked fixture is marked tracked");
    require(trackedFixture.migrationScope == "fixture_contract", "tracked fixture is fixture contract");
    const edi::formats::InventoryRow ignoredArtifact = edi::formats::withRepositoryState(edi::formats::classifyInventoryPath("tests/artifacts/drawing_metrics/raw/click_run.jsonl", 12), index);
    require(ignoredArtifact.repositoryState == "ignored", "ignored artifact is marked ignored");
    require(ignoredArtifact.migrationScope == "disposable_artifact", "ignored artifact is disposable");
    const edi::formats::InventoryRow untrackedLocal = edi::formats::withRepositoryState(edi::formats::classifyInventoryPath("misc/unowned.json", 13), index);
    require(untrackedLocal.repositoryState == "untracked", "untracked local file is marked untracked");
    require(untrackedLocal.migrationScope == "local_audit", "untracked local file is local audit");

    const QVector<edi::formats::InventoryRow> rows{
        trackedConfig,
        trackedFixture,
        untrackedLocal,
    };
    const QVector<edi::formats::InventoryRow> messagePackRows = edi::formats::filterInventoryRows(rows, {{}, {}, {"MessagePack"}, {}});
    require(messagePackRows.size() == 1, "target filter keeps matching row only");
    require(messagePackRows.first().path.contains("line_create_basic"), "target filter returns fixture row");
    const QVector<edi::formats::InventoryRow> blockedRows = edi::formats::filterInventoryRows(rows, {{}, {}, {}, {"blocked"}});
    require(blockedRows.size() == 1, "priority filter finds blocked rows");
    const QVector<edi::formats::InventoryRow> trackedRows = edi::formats::filterInventoryRows(rows, {{}, {}, {}, {}, {"tracked"}, {}});
    require(trackedRows.size() == 2, "repository state filter finds tracked rows");
    const QVector<edi::formats::InventoryRow> scopeRows = edi::formats::filterInventoryRows(rows, {{}, {}, {}, {}, {}, {"fixture_contract"}});
    require(scopeRows.size() == 1, "scope filter finds fixture contract rows");
    require(edi::formats::inventoryUnknownCount(rows) == 1, "unknown count detects unclassified rows");
    require(edi::formats::inventoryBlockedCount(rows) == 1, "blocked count detects blocked migration rows");

    const QVector<edi::formats::InventoryFamilySummary> summaries = edi::formats::inventoryFamilySummaries(rows, 1);
    require(summaries.size() == 3, "family summary groups by family and target");
    bool foundFixtureSummary = false;
    for (const edi::formats::InventoryFamilySummary &summary : summaries) {
        if (summary.dataFamily == "drawing_replay_fixture") {
            foundFixtureSummary = true;
            require(summary.fileCount == 1, "fixture summary counts files");
            require(summary.sizeBytes == 11, "fixture summary sums bytes");
            require(summary.samplePaths.size() == 1, "fixture summary caps samples");
            require(summary.samplePaths.first().contains("line_create_basic"), "fixture summary keeps sample path");
        }
    }
    require(foundFixtureSummary, "family summary includes fixture family");
    require(edi::formats::inventoryFamilySummaryHeader().contains("file_count"), "family header includes file count");
    require(edi::formats::inventoryFamilySummaryReport(rows, 1).contains("drawing_replay_fixture"), "family report includes family names");

    return 0;
}
