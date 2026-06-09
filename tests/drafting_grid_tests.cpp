#include "drafting/DraftingGrid.h"

#include <cassert>
#include <cmath>
#include <string>

using namespace edi::drafting;

namespace {

bool nearlyEqual(double a, double b)
{
    return std::abs(a - b) < 0.000001;
}

} // namespace

int main()
{
    assert(std::string(draftingGridUnitName(DraftingGridUnit::Millimeter)) == "millimeter");
    assert(std::string(draftingGridUnitLabel(DraftingGridUnit::Inch)) == "in");
    assert(draftingGridUnitFromName("millimeter") == DraftingGridUnit::Millimeter);
    assert(draftingGridUnitFromName("centimeter") == DraftingGridUnit::Centimeter);
    assert(draftingGridUnitFromName("inch") == DraftingGridUnit::Inch);
    assert(draftingGridUnitFromName("foot") == DraftingGridUnit::Foot);
    assert(draftingGridUnitFromName("missing") == DraftingGridUnit::CanvasUnit);
    assert(std::string(draftingGridPresetName(DraftingGridPreset::SquareArtBoard)) == "square_art_board");
    assert(std::string(draftingGridPresetLabel(DraftingGridPreset::Letter)) == "Letter");
    assert(draftingGridPresetFromName("a4") == DraftingGridPreset::A4);
    assert(draftingGridPresetFromName("missing") == DraftingGridPreset::Custom);

    DraftingGridSettings letter = draftingGridPresetSettings(DraftingGridPreset::Letter);
    assert(letter.unit == DraftingGridUnit::Inch);
    assert(nearlyEqual(letter.width, 8.5));
    assert(nearlyEqual(letter.height, 11.0));

    DraftingGridProjection projectedLetter = projectDraftingGrid(letter);
    assert(nearlyEqual(projectedLetter.drawableBounds.x, 0.25 / 8.5));
    assert(nearlyEqual(projectedLetter.drawableBounds.y, 0.25 / 11.0));
    assert(!projectedLetter.lines.empty());
    assert(projectedLetter.lines.front().major);

    DraftingGridSettings unsafe = letter;
    unsafe.width = -1.0;
    unsafe.height = 0.0;
    unsafe.marginLeft = 20.0;
    unsafe.minorStep = -5.0;
    unsafe.majorLineEvery = 0;
    DraftingGridSettings safe = sanitizeDraftingGridSettings(unsafe);
    assert(safe.width > 0.0);
    assert(safe.height > 0.0);
    assert(safe.marginLeft <= safe.width);
    assert(safe.minorStep > 0.0);
    assert(safe.majorLineEvery == 1);

    DraftingGridProjection square = projectDraftingGrid(draftingGridPresetSettings(DraftingGridPreset::SquareArtBoard));
    assert(!boundsOutsideDrawableArea({0.1, 0.1, 0.2, 0.2}, square));
    assert(boundsOutsideDrawableArea({0.0, 0.1, 0.2, 0.2}, square));
    assert(boundsOutsideDrawableArea({0.9, 0.9, 0.2, 0.2}, square));

    return 0;
}
