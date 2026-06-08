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
    require(edi::formats::inventoryRowLine(edi::formats::classifyInventoryPath("data/ui_theme.json", 10)).contains("theme_config"), "row line includes family");

    const QVector<edi::formats::InventoryRow> rows{
        edi::formats::classifyInventoryPath("data/ui_theme.json", 10),
        edi::formats::classifyInventoryPath("tests/fixtures/drawing_tool_scripts/line_create_basic.json", 11),
        edi::formats::classifyInventoryPath("misc/unowned.json", 12),
    };
    const QVector<edi::formats::InventoryRow> messagePackRows = edi::formats::filterInventoryRows(rows, {{}, {}, {"MessagePack"}, {}});
    require(messagePackRows.size() == 1, "target filter keeps matching row only");
    require(messagePackRows.first().path.contains("line_create_basic"), "target filter returns fixture row");
    const QVector<edi::formats::InventoryRow> blockedRows = edi::formats::filterInventoryRows(rows, {{}, {}, {}, {"blocked"}});
    require(blockedRows.size() == 1, "priority filter finds blocked rows");
    require(edi::formats::inventoryUnknownCount(rows) == 1, "unknown count detects unclassified rows");
    require(edi::formats::inventoryBlockedCount(rows) == 1, "blocked count detects blocked migration rows");

    return 0;
}
