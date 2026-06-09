#pragma once

#include "drafting/DraftingTypes.h"

#include <string>
#include <vector>

namespace edi::drafting {

enum class DraftingGridUnit {
    CanvasUnit,
    Millimeter,
    Centimeter,
    Inch,
    Foot
};

enum class DraftingGridPreset {
    Custom,
    Letter,
    A4,
    SquareArtBoard
};

enum class DraftingGridLineAxis {
    Vertical,
    Horizontal
};

struct DraftingGridSettings {
    DraftingGridPreset preset = DraftingGridPreset::SquareArtBoard;
    DraftingGridUnit unit = DraftingGridUnit::Inch;
    double width = 12.0;
    double height = 12.0;
    double marginLeft = 0.25;
    double marginTop = 0.25;
    double marginRight = 0.25;
    double marginBottom = 0.25;
    double minorStep = 0.25;
    int majorLineEvery = 4;
    bool visible = true;
};

struct DraftingGridLine {
    DraftingGridLineAxis axis = DraftingGridLineAxis::Vertical;
    double position = 0.0;
    bool major = false;
};

struct DraftingGridProjection {
    DraftingGridSettings settings;
    Bounds2D pageBounds {0.0, 0.0, 1.0, 1.0};
    Bounds2D drawableBounds;
    Point2D origin {0.0, 0.0};
    std::vector<DraftingGridLine> lines;
};

const char *draftingGridUnitName(DraftingGridUnit unit);
const char *draftingGridUnitLabel(DraftingGridUnit unit);
const char *draftingGridPresetName(DraftingGridPreset preset);
const char *draftingGridPresetLabel(DraftingGridPreset preset);
DraftingGridUnit draftingGridUnitFromName(const std::string &name);
DraftingGridPreset draftingGridPresetFromName(const std::string &name);
DraftingGridSettings draftingGridPresetSettings(DraftingGridPreset preset);
DraftingGridSettings defaultDraftingGridSettings();
DraftingGridSettings sanitizeDraftingGridSettings(DraftingGridSettings settings);
DraftingGridProjection projectDraftingGrid(const DraftingGridSettings &settings);
bool boundsOutsideDrawableArea(const Bounds2D &bounds, const DraftingGridProjection &grid);

} // namespace edi::drafting
