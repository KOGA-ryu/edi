#include "drafting/DraftingMetadata.h"

#include "EdiAssert.h"
#include <limits>
#include <string>

using namespace edi::drafting;

namespace {

ObjectMetadata validMetadata()
{
    ObjectMetadata metadata;
    metadata.author = "tester";
    metadata.source = "unit_test";
    metadata.createdAt = "2026-06-08T12:30:00Z";
    metadata.toolProvenance = "drafting_metadata_tests";
    metadata.measurementNote = "calibrated by test";
    metadata.measurement.unit = MeasurementUnit::Centimeter;
    metadata.measurement.canvasUnitsPerRealUnit = 2.0;
    metadata.measurement.label = "bench scale";
    return metadata;
}

} // namespace

int main()
{
    auto defaultValidation = validateObjectMetadata(ObjectMetadata{});
    EDI_CHECK(defaultValidation.ok);
    EDI_CHECK(defaultValidation.code == DraftingResultCode::None);

    auto validTimestampValidation = validateObjectMetadata(validMetadata());
    EDI_CHECK(validTimestampValidation.ok);
    EDI_CHECK(isValidMetadataTimestamp("2026-06-08T12:30:00Z"));
    EDI_CHECK(isValidMetadataTimestamp("2024-02-29T23:59:59Z"));
    EDI_CHECK(isValidMetadataTimestamp(""));
    EDI_CHECK(isValidMetadataText("metadata", kMetadataShortTextLimit));
    EDI_CHECK(isValidMeasurementMetadata(validMetadata().measurement));
    EDI_CHECK(isValidMeasurementMetadata(MeasurementMetadata{}));
    EDI_CHECK(isValidGuideVisualColor("#83aeca"));
    EDI_CHECK(!isValidGuideVisualColor("83aeca"));
    EDI_CHECK(!isValidGuideVisualColor("#83aecx"));
    EDI_CHECK(isValidGuideVisualDashStyle("solid"));
    EDI_CHECK(isValidGuideVisualDashStyle("dash"));
    EDI_CHECK(isValidGuideVisualDashStyle("dot"));
    EDI_CHECK(!isValidGuideVisualDashStyle("stripe"));
    EDI_CHECK(!isValidMetadataTimestamp("2026-00-08T12:30:00Z"));
    EDI_CHECK(!isValidMetadataTimestamp("2026-13-08T12:30:00Z"));
    EDI_CHECK(!isValidMetadataTimestamp("2026-04-31T12:30:00Z"));
    EDI_CHECK(!isValidMetadataTimestamp("2026-02-29T12:30:00Z"));
    EDI_CHECK(!isValidMetadataTimestamp("2026-06-08T24:30:00Z"));
    EDI_CHECK(!isValidMetadataTimestamp("2026-06-08T12:60:00Z"));
    EDI_CHECK(!isValidMetadataTimestamp("2026-06-08T12:30:60Z"));

    ObjectMetadata badVersion = validMetadata();
    badVersion.schemaVersion = 0;
    auto badVersionValidation = validateObjectMetadata(badVersion);
    EDI_CHECK(!badVersionValidation.ok);
    EDI_CHECK(badVersionValidation.code == DraftingResultCode::InvalidMetadata);

    ObjectMetadata badTimestamp = validMetadata();
    badTimestamp.createdAt = "June 8";
    EDI_CHECK(!isValidMetadataTimestamp(badTimestamp.createdAt));
    auto badTimestampValidation = validateObjectMetadata(badTimestamp);
    EDI_CHECK(!badTimestampValidation.ok);
    EDI_CHECK(badTimestampValidation.code == DraftingResultCode::InvalidMetadata);

    ObjectMetadata badAuthor = validMetadata();
    badAuthor.author = "bad\nauthor";
    EDI_CHECK(!isValidMetadataText(badAuthor.author, kMetadataShortTextLimit));
    auto badAuthorValidation = validateObjectMetadata(badAuthor);
    EDI_CHECK(!badAuthorValidation.ok);
    EDI_CHECK(badAuthorValidation.code == DraftingResultCode::InvalidMetadata);

    ObjectMetadata longAuthor = validMetadata();
    longAuthor.author = std::string(kMetadataShortTextLimit + 1, 'x');
    auto longAuthorValidation = validateObjectMetadata(longAuthor);
    EDI_CHECK(!longAuthorValidation.ok);
    EDI_CHECK(longAuthorValidation.code == DraftingResultCode::InvalidMetadata);

    ObjectMetadata longSource = validMetadata();
    longSource.source = std::string(kMetadataShortTextLimit + 1, 'x');
    auto longSourceValidation = validateObjectMetadata(longSource);
    EDI_CHECK(!longSourceValidation.ok);
    EDI_CHECK(longSourceValidation.code == DraftingResultCode::InvalidMetadata);

    ObjectMetadata longToolProvenance = validMetadata();
    longToolProvenance.toolProvenance = std::string(kMetadataShortTextLimit + 1, 'x');
    auto longToolProvenanceValidation = validateObjectMetadata(longToolProvenance);
    EDI_CHECK(!longToolProvenanceValidation.ok);
    EDI_CHECK(longToolProvenanceValidation.code == DraftingResultCode::InvalidMetadata);

    ObjectMetadata longMeasurementNote = validMetadata();
    longMeasurementNote.measurementNote = std::string(kMetadataMeasurementNoteLimit + 1, 'x');
    auto longMeasurementNoteValidation = validateObjectMetadata(longMeasurementNote);
    EDI_CHECK(!longMeasurementNoteValidation.ok);
    EDI_CHECK(longMeasurementNoteValidation.code == DraftingResultCode::InvalidMetadata);

    ObjectMetadata noUnitZeroScale = validMetadata();
    noUnitZeroScale.measurement.unit = MeasurementUnit::None;
    noUnitZeroScale.measurement.canvasUnitsPerRealUnit = 0.0;
    EDI_CHECK(isValidMeasurementMetadata(noUnitZeroScale.measurement));
    auto noUnitZeroScaleValidation = validateObjectMetadata(noUnitZeroScale);
    EDI_CHECK(noUnitZeroScaleValidation.ok);

    ObjectMetadata zeroRealScale = validMetadata();
    zeroRealScale.measurement.unit = MeasurementUnit::Centimeter;
    zeroRealScale.measurement.canvasUnitsPerRealUnit = 0.0;
    EDI_CHECK(!isValidMeasurementMetadata(zeroRealScale.measurement));
    auto zeroRealScaleValidation = validateObjectMetadata(zeroRealScale);
    EDI_CHECK(!zeroRealScaleValidation.ok);
    EDI_CHECK(zeroRealScaleValidation.code == DraftingResultCode::InvalidMetadata);

    ObjectMetadata nonFiniteScale = validMetadata();
    nonFiniteScale.measurement.canvasUnitsPerRealUnit = std::numeric_limits<double>::quiet_NaN();
    EDI_CHECK(!isValidMeasurementMetadata(nonFiniteScale.measurement));
    auto nonFiniteScaleValidation = validateObjectMetadata(nonFiniteScale);
    EDI_CHECK(!nonFiniteScaleValidation.ok);
    EDI_CHECK(nonFiniteScaleValidation.code == DraftingResultCode::InvalidMetadata);

    ObjectMetadata badMeasurementLabel = validMetadata();
    badMeasurementLabel.measurement.label = "bad\nlabel";
    EDI_CHECK(!isValidMeasurementMetadata(badMeasurementLabel.measurement));
    auto badMeasurementLabelValidation = validateObjectMetadata(badMeasurementLabel);
    EDI_CHECK(!badMeasurementLabelValidation.ok);
    EDI_CHECK(badMeasurementLabelValidation.code == DraftingResultCode::InvalidMetadata);

    ObjectMetadata longMeasurementLabel = validMetadata();
    longMeasurementLabel.measurement.label = std::string(kMetadataShortTextLimit + 1, 'x');
    EDI_CHECK(!isValidMeasurementMetadata(longMeasurementLabel.measurement));
    auto longMeasurementLabelValidation = validateObjectMetadata(longMeasurementLabel);
    EDI_CHECK(!longMeasurementLabelValidation.ok);
    EDI_CHECK(longMeasurementLabelValidation.code == DraftingResultCode::InvalidMetadata);

    ObjectMetadata guideVisualMetadata = validMetadata();
    guideVisualMetadata.guideVisual.label = "material edge";
    guideVisualMetadata.guideVisual.color = "#54d2c6";
    guideVisualMetadata.guideVisual.dashStyle = "solid";
    guideVisualMetadata.guideVisual.showLabel = false;
    EDI_CHECK(validateObjectMetadata(guideVisualMetadata).ok);

    ObjectMetadata badGuideLabel = validMetadata();
    badGuideLabel.guideVisual.label = "bad\nlabel";
    auto badGuideLabelValidation = validateObjectMetadata(badGuideLabel);
    EDI_CHECK(!badGuideLabelValidation.ok);
    EDI_CHECK(badGuideLabelValidation.code == DraftingResultCode::InvalidMetadata);

    ObjectMetadata badGuideColor = validMetadata();
    badGuideColor.guideVisual.color = "#xyzxyz";
    auto badGuideColorValidation = validateObjectMetadata(badGuideColor);
    EDI_CHECK(!badGuideColorValidation.ok);
    EDI_CHECK(badGuideColorValidation.code == DraftingResultCode::InvalidMetadata);

    ObjectMetadata badGuideDash = validMetadata();
    badGuideDash.guideVisual.dashStyle = "stripe";
    auto badGuideDashValidation = validateObjectMetadata(badGuideDash);
    EDI_CHECK(!badGuideDashValidation.ok);
    EDI_CHECK(badGuideDashValidation.code == DraftingResultCode::InvalidMetadata);

    ObjectMetadata guidePlanBase = validMetadata();
    auto labelPlan = planGuideVisualLabelUpdate(guidePlanBase, "cut line");
    EDI_CHECK(labelPlan.ok);
    EDI_CHECK(labelPlan.metadata.guideVisual.label == "cut line");
    EDI_CHECK(labelPlan.metadata.author == guidePlanBase.author);

    auto colorPlan = planGuideVisualColorUpdate(guidePlanBase, "#f6c65b");
    EDI_CHECK(colorPlan.ok);
    EDI_CHECK(colorPlan.metadata.guideVisual.color == "#f6c65b");

    auto dashPlan = planGuideVisualDashStyleUpdate(guidePlanBase, "dot");
    EDI_CHECK(dashPlan.ok);
    EDI_CHECK(dashPlan.metadata.guideVisual.dashStyle == "dot");

    auto visibilityPlan = planGuideVisualLabelVisibleUpdate(guidePlanBase, false);
    EDI_CHECK(visibilityPlan.ok);
    EDI_CHECK(!visibilityPlan.metadata.guideVisual.showLabel);

    auto badLabelPlan = planGuideVisualLabelUpdate(guidePlanBase, "bad\nlabel");
    EDI_CHECK(!badLabelPlan.ok);
    EDI_CHECK(badLabelPlan.code == DraftingResultCode::InvalidMetadata);

    auto badColorPlan = planGuideVisualColorUpdate(guidePlanBase, "blue");
    EDI_CHECK(!badColorPlan.ok);
    EDI_CHECK(badColorPlan.code == DraftingResultCode::InvalidMetadata);

    auto badDashPlan = planGuideVisualDashStyleUpdate(guidePlanBase, "stripe");
    EDI_CHECK(!badDashPlan.ok);
    EDI_CHECK(badDashPlan.code == DraftingResultCode::InvalidMetadata);

    auto dimensionVisibilityPlan = planDimensionVisualLabelVisibleUpdate(guidePlanBase, false);
    EDI_CHECK(dimensionVisibilityPlan.ok);
    EDI_CHECK(!dimensionVisibilityPlan.metadata.dimensionVisual.showLabel);
    EDI_CHECK(dimensionVisibilityPlan.metadata.guideVisual.color == guidePlanBase.guideVisual.color);

    ObjectMetadata badBaseForDimensionPlan = guidePlanBase;
    badBaseForDimensionPlan.guideVisual.color = "bad";
    auto rejectedDimensionVisibilityPlan = planDimensionVisualLabelVisibleUpdate(badBaseForDimensionPlan, false);
    EDI_CHECK(!rejectedDimensionVisibilityPlan.ok);
    EDI_CHECK(rejectedDimensionVisibilityPlan.code == DraftingResultCode::InvalidMetadata);

    auto measurementNotePlan = planMeasurementNoteUpdate(guidePlanBase, "measured on plotter");
    EDI_CHECK(measurementNotePlan.ok);
    EDI_CHECK(measurementNotePlan.metadata.measurementNote == "measured on plotter");
    EDI_CHECK(measurementNotePlan.metadata.author == guidePlanBase.author);

    auto badMeasurementNotePlan = planMeasurementNoteUpdate(guidePlanBase, std::string(kMetadataMeasurementNoteLimit + 1, 'x'));
    EDI_CHECK(!badMeasurementNotePlan.ok);
    EDI_CHECK(badMeasurementNotePlan.code == DraftingResultCode::InvalidMetadata);

    return 0;
}
