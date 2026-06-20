#include "drafting/DraftingGrid.h"

#include "EdiAssert.h"
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
    EDI_CHECK(std::string(draftingGridUnitName(DraftingGridUnit::Millimeter)) == "millimeter");
    EDI_CHECK(std::string(draftingGridUnitLabel(DraftingGridUnit::Inch)) == "in");
    EDI_CHECK(draftingGridUnitFromName("millimeter") == DraftingGridUnit::Millimeter);
    EDI_CHECK(draftingGridUnitFromName("centimeter") == DraftingGridUnit::Centimeter);
    EDI_CHECK(draftingGridUnitFromName("inch") == DraftingGridUnit::Inch);
    EDI_CHECK(draftingGridUnitFromName("foot") == DraftingGridUnit::Foot);
    EDI_CHECK(draftingGridUnitFromName("missing") == DraftingGridUnit::CanvasUnit);
    EDI_CHECK(std::string(draftingGridPresetName(DraftingGridPreset::SquareArtBoard)) == "square_art_board");
    EDI_CHECK(std::string(draftingGridPresetLabel(DraftingGridPreset::Letter)) == "Letter");
    EDI_CHECK(draftingGridPresetFromName("a4") == DraftingGridPreset::A4);
    EDI_CHECK(draftingGridPresetFromName("missing") == DraftingGridPreset::Custom);

    DraftingGridSettings letter = draftingGridPresetSettings(DraftingGridPreset::Letter);
    EDI_CHECK(letter.unit == DraftingGridUnit::Inch);
    EDI_CHECK(nearlyEqual(letter.width, 8.5));
    EDI_CHECK(nearlyEqual(letter.height, 11.0));

    DraftingGridProjection projectedLetter = projectDraftingGrid(letter);
    EDI_CHECK(nearlyEqual(projectedLetter.drawableBounds.x, 0.25 / 8.5));
    EDI_CHECK(nearlyEqual(projectedLetter.drawableBounds.y, 0.25 / 11.0));
    EDI_CHECK(!projectedLetter.lines.empty());
    EDI_CHECK(projectedLetter.lines.front().major);

    DraftingGridSettings unsafe = letter;
    unsafe.width = -1.0;
    unsafe.height = 0.0;
    unsafe.marginLeft = 20.0;
    unsafe.minorStep = -5.0;
    unsafe.majorLineEvery = 0;
    DraftingGridSettings safe = sanitizeDraftingGridSettings(unsafe);
    EDI_CHECK(safe.width > 0.0);
    EDI_CHECK(safe.height > 0.0);
    EDI_CHECK(safe.marginLeft <= safe.width);
    EDI_CHECK(safe.minorStep > 0.0);
    EDI_CHECK(safe.majorLineEvery == 1);

    DraftingGridProjection square = projectDraftingGrid(draftingGridPresetSettings(DraftingGridPreset::SquareArtBoard));
    EDI_CHECK(!boundsOutsideDrawableArea({0.1, 0.1, 0.2, 0.2}, square));
    EDI_CHECK(boundsOutsideDrawableArea({0.0, 0.1, 0.2, 0.2}, square));
    EDI_CHECK(boundsOutsideDrawableArea({0.9, 0.9, 0.2, 0.2}, square));

    return 0;
}
