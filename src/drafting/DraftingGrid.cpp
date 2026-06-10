#include "drafting/DraftingGrid.h"

#include <algorithm>
#include <cmath>

namespace edi::drafting {

namespace {

constexpr double kMinSize = 0.000001;

double finiteOr(double value, double fallback)
{
    return std::isfinite(value) ? value : fallback;
}

double clampMargin(double margin, double limit)
{
    return std::clamp(finiteOr(margin, 0.0), 0.0, std::max(0.0, limit));
}

void appendLines(std::vector<DraftingGridLine> &lines, DraftingGridLineAxis axis, double size, double step, int majorEvery)
{
    if (step <= 0.0 || size <= 0.0) {
        return;
    }

    const int count = static_cast<int>(std::floor(size / step));
    for (int i = 0; i <= count; ++i) {
        const double physical = static_cast<double>(i) * step;
        const double position = std::clamp(physical / size, 0.0, 1.0);
        lines.push_back({axis, position, majorEvery > 0 && i % majorEvery == 0});
    }
}

} // namespace

const char *draftingGridUnitName(DraftingGridUnit unit)
{
    switch (unit) {
    case DraftingGridUnit::CanvasUnit:
        return "canvas_unit";
    case DraftingGridUnit::Millimeter:
        return "millimeter";
    case DraftingGridUnit::Centimeter:
        return "centimeter";
    case DraftingGridUnit::Inch:
        return "inch";
    case DraftingGridUnit::Foot:
        return "foot";
    }
    return "canvas_unit";
}

const char *draftingGridUnitLabel(DraftingGridUnit unit)
{
    switch (unit) {
    case DraftingGridUnit::CanvasUnit:
        return "cu";
    case DraftingGridUnit::Millimeter:
        return "mm";
    case DraftingGridUnit::Centimeter:
        return "cm";
    case DraftingGridUnit::Inch:
        return "in";
    case DraftingGridUnit::Foot:
        return "ft";
    }
    return "cu";
}

const char *draftingGridPresetName(DraftingGridPreset preset)
{
    switch (preset) {
    case DraftingGridPreset::Custom:
        return "custom";
    case DraftingGridPreset::Letter:
        return "letter";
    case DraftingGridPreset::A4:
        return "a4";
    case DraftingGridPreset::SquareArtBoard:
        return "square_art_board";
    }
    return "custom";
}

const char *draftingGridPresetLabel(DraftingGridPreset preset)
{
    switch (preset) {
    case DraftingGridPreset::Custom:
        return "Custom";
    case DraftingGridPreset::Letter:
        return "Letter";
    case DraftingGridPreset::A4:
        return "A4";
    case DraftingGridPreset::SquareArtBoard:
        return "Square art board";
    }
    return "Custom";
}

DraftingGridUnit draftingGridUnitFromName(const std::string &name)
{
    if (name == "millimeter") {
        return DraftingGridUnit::Millimeter;
    }
    if (name == "centimeter") {
        return DraftingGridUnit::Centimeter;
    }
    if (name == "inch") {
        return DraftingGridUnit::Inch;
    }
    if (name == "foot") {
        return DraftingGridUnit::Foot;
    }
    return DraftingGridUnit::CanvasUnit;
}

double millimetersPerUnit(DraftingGridUnit unit)
{
    switch (unit) {
    case DraftingGridUnit::CanvasUnit:
        return 1.0; // device-dependent: treat one canvas unit as one millimetre
    case DraftingGridUnit::Millimeter:
        return 1.0;
    case DraftingGridUnit::Centimeter:
        return 10.0;
    case DraftingGridUnit::Inch:
        return 25.4;
    case DraftingGridUnit::Foot:
        return 304.8;
    }
    return 1.0;
}

DraftingGridPreset draftingGridPresetFromName(const std::string &name)
{
    if (name == "letter") {
        return DraftingGridPreset::Letter;
    }
    if (name == "a4") {
        return DraftingGridPreset::A4;
    }
    if (name == "square_art_board") {
        return DraftingGridPreset::SquareArtBoard;
    }
    return DraftingGridPreset::Custom;
}

DraftingGridSettings draftingGridPresetSettings(DraftingGridPreset preset)
{
    switch (preset) {
    case DraftingGridPreset::Letter:
        return {preset, DraftingGridUnit::Inch, 8.5, 11.0, 0.25, 0.25, 0.25, 0.25, 0.25, 4, true};
    case DraftingGridPreset::A4:
        return {preset, DraftingGridUnit::Millimeter, 210.0, 297.0, 5.0, 5.0, 5.0, 5.0, 5.0, 5, true};
    case DraftingGridPreset::SquareArtBoard:
        return {preset, DraftingGridUnit::Inch, 12.0, 12.0, 0.25, 0.25, 0.25, 0.25, 0.25, 4, true};
    case DraftingGridPreset::Custom:
        break;
    }
    return {DraftingGridPreset::Custom, DraftingGridUnit::CanvasUnit, 1.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0625, 4, true};
}

DraftingGridSettings defaultDraftingGridSettings()
{
    return draftingGridPresetSettings(DraftingGridPreset::SquareArtBoard);
}

DraftingGridSettings sanitizeDraftingGridSettings(DraftingGridSettings settings)
{
    settings.width = std::max(kMinSize, finiteOr(settings.width, 1.0));
    settings.height = std::max(kMinSize, finiteOr(settings.height, 1.0));
    settings.marginLeft = clampMargin(settings.marginLeft, settings.width);
    settings.marginRight = clampMargin(settings.marginRight, settings.width - settings.marginLeft);
    settings.marginTop = clampMargin(settings.marginTop, settings.height);
    settings.marginBottom = clampMargin(settings.marginBottom, settings.height - settings.marginTop);
    settings.minorStep = std::clamp(finiteOr(settings.minorStep, 0.0625), kMinSize, std::max(settings.width, settings.height));
    settings.majorLineEvery = std::max(1, settings.majorLineEvery);
    return settings;
}

DraftingGridProjection projectDraftingGrid(const DraftingGridSettings &settings)
{
    DraftingGridProjection projection;
    projection.settings = sanitizeDraftingGridSettings(settings);

    const DraftingGridSettings &safe = projection.settings;
    projection.drawableBounds = {
        safe.marginLeft / safe.width,
        safe.marginTop / safe.height,
        std::max(0.0, safe.width - safe.marginLeft - safe.marginRight) / safe.width,
        std::max(0.0, safe.height - safe.marginTop - safe.marginBottom) / safe.height,
    };
    projection.origin = {projection.drawableBounds.x, projection.drawableBounds.y};

    if (safe.visible) {
        appendLines(projection.lines, DraftingGridLineAxis::Vertical, safe.width, safe.minorStep, safe.majorLineEvery);
        appendLines(projection.lines, DraftingGridLineAxis::Horizontal, safe.height, safe.minorStep, safe.majorLineEvery);
    }

    return projection;
}

bool boundsOutsideDrawableArea(const Bounds2D &bounds, const DraftingGridProjection &grid)
{
    const Bounds2D drawable = grid.drawableBounds;
    return bounds.x < drawable.x
        || bounds.y < drawable.y
        || bounds.x + bounds.width > drawable.x + drawable.width
        || bounds.y + bounds.height > drawable.y + drawable.height;
}

} // namespace edi::drafting
