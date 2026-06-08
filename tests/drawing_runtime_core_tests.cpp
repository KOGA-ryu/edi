#include "runtime/DrawingRuntimeCore.h"

#include <iostream>

namespace {

void require(bool condition, const char *message)
{
    if (!condition) {
        std::cerr << "failed: " << message << '\n';
        std::exit(1);
    }
}

bool containsId(const QVariantList &rows, const QString &id)
{
    for (const QVariant &row : rows) {
        if (row.toMap().value("id").toString() == id) {
            return true;
        }
    }
    return false;
}

} // namespace

int main()
{
    DrawingToolCatalog catalog;
    const QVariantList toolModes = catalog.toolModes();
    require(toolModes.size() == 19, "tool catalog keeps all drawing modes");
    require(containsId(toolModes, "anchor_points"), "tool catalog includes anchor_points");
    require(containsId(toolModes, "regular_polygon"), "tool catalog includes regular_polygon");

    const QVariantMap settings = catalog.toolSettingsById();
    require(settings.contains("circle_arc"), "settings include circle_arc");
    require(settings.value("circle_arc").toList().size() == 4, "circle_arc has stable setting rows");

    const QVariantList sidebars = catalog.sidebarSections();
    require(sidebars.size() == 11, "sidebar sections count is stable");
    require(sidebars.first().toMap().value("id").toString() == "draw", "first sidebar section is draw");

    DrawingRuntimeRows rows;
    require(rows.editNumber(1.23456) == "1.235", "editNumber rounds to 3 decimals");
    require(rows.sidebarRowClickable({{"action", "tool"}}), "action-backed sidebar rows are clickable");
    require(!rows.sidebarRowClickable({{"action", ""}}), "empty-action sidebar rows are not clickable");

    return 0;
}
