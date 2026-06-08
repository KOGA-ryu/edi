#include "drafting/DraftingMetadata.h"

#include <cassert>
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
    metadata.measurement.scale = 2.0;
    metadata.measurement.label = "bench scale";
    return metadata;
}

} // namespace

int main()
{
    auto defaultValidation = validateObjectMetadata(ObjectMetadata{});
    assert(defaultValidation.ok);
    assert(defaultValidation.code == DraftingResultCode::None);

    auto validTimestampValidation = validateObjectMetadata(validMetadata());
    assert(validTimestampValidation.ok);
    assert(isValidMetadataTimestamp("2026-06-08T12:30:00Z"));
    assert(isValidMetadataTimestamp("2024-02-29T23:59:59Z"));
    assert(isValidMetadataTimestamp(""));
    assert(isValidMetadataText("metadata", kMetadataShortTextLimit));
    assert(isValidMeasurementMetadata(validMetadata().measurement));
    assert(isValidMeasurementMetadata(MeasurementMetadata{}));
    assert(!isValidMetadataTimestamp("2026-00-08T12:30:00Z"));
    assert(!isValidMetadataTimestamp("2026-13-08T12:30:00Z"));
    assert(!isValidMetadataTimestamp("2026-04-31T12:30:00Z"));
    assert(!isValidMetadataTimestamp("2026-02-29T12:30:00Z"));
    assert(!isValidMetadataTimestamp("2026-06-08T24:30:00Z"));
    assert(!isValidMetadataTimestamp("2026-06-08T12:60:00Z"));
    assert(!isValidMetadataTimestamp("2026-06-08T12:30:60Z"));

    ObjectMetadata badVersion = validMetadata();
    badVersion.schemaVersion = 0;
    auto badVersionValidation = validateObjectMetadata(badVersion);
    assert(!badVersionValidation.ok);
    assert(badVersionValidation.code == DraftingResultCode::InvalidMetadata);

    ObjectMetadata badTimestamp = validMetadata();
    badTimestamp.createdAt = "June 8";
    assert(!isValidMetadataTimestamp(badTimestamp.createdAt));
    auto badTimestampValidation = validateObjectMetadata(badTimestamp);
    assert(!badTimestampValidation.ok);
    assert(badTimestampValidation.code == DraftingResultCode::InvalidMetadata);

    ObjectMetadata badAuthor = validMetadata();
    badAuthor.author = "bad\nauthor";
    assert(!isValidMetadataText(badAuthor.author, kMetadataShortTextLimit));
    auto badAuthorValidation = validateObjectMetadata(badAuthor);
    assert(!badAuthorValidation.ok);
    assert(badAuthorValidation.code == DraftingResultCode::InvalidMetadata);

    ObjectMetadata longAuthor = validMetadata();
    longAuthor.author = std::string(kMetadataShortTextLimit + 1, 'x');
    auto longAuthorValidation = validateObjectMetadata(longAuthor);
    assert(!longAuthorValidation.ok);
    assert(longAuthorValidation.code == DraftingResultCode::InvalidMetadata);

    ObjectMetadata longSource = validMetadata();
    longSource.source = std::string(kMetadataShortTextLimit + 1, 'x');
    auto longSourceValidation = validateObjectMetadata(longSource);
    assert(!longSourceValidation.ok);
    assert(longSourceValidation.code == DraftingResultCode::InvalidMetadata);

    ObjectMetadata longToolProvenance = validMetadata();
    longToolProvenance.toolProvenance = std::string(kMetadataShortTextLimit + 1, 'x');
    auto longToolProvenanceValidation = validateObjectMetadata(longToolProvenance);
    assert(!longToolProvenanceValidation.ok);
    assert(longToolProvenanceValidation.code == DraftingResultCode::InvalidMetadata);

    ObjectMetadata longMeasurementNote = validMetadata();
    longMeasurementNote.measurementNote = std::string(kMetadataMeasurementNoteLimit + 1, 'x');
    auto longMeasurementNoteValidation = validateObjectMetadata(longMeasurementNote);
    assert(!longMeasurementNoteValidation.ok);
    assert(longMeasurementNoteValidation.code == DraftingResultCode::InvalidMetadata);

    ObjectMetadata noUnitZeroScale = validMetadata();
    noUnitZeroScale.measurement.unit = MeasurementUnit::None;
    noUnitZeroScale.measurement.scale = 0.0;
    assert(isValidMeasurementMetadata(noUnitZeroScale.measurement));
    auto noUnitZeroScaleValidation = validateObjectMetadata(noUnitZeroScale);
    assert(noUnitZeroScaleValidation.ok);

    ObjectMetadata zeroRealScale = validMetadata();
    zeroRealScale.measurement.unit = MeasurementUnit::Centimeter;
    zeroRealScale.measurement.scale = 0.0;
    assert(!isValidMeasurementMetadata(zeroRealScale.measurement));
    auto zeroRealScaleValidation = validateObjectMetadata(zeroRealScale);
    assert(!zeroRealScaleValidation.ok);
    assert(zeroRealScaleValidation.code == DraftingResultCode::InvalidMetadata);

    ObjectMetadata nonFiniteScale = validMetadata();
    nonFiniteScale.measurement.scale = std::numeric_limits<double>::quiet_NaN();
    assert(!isValidMeasurementMetadata(nonFiniteScale.measurement));
    auto nonFiniteScaleValidation = validateObjectMetadata(nonFiniteScale);
    assert(!nonFiniteScaleValidation.ok);
    assert(nonFiniteScaleValidation.code == DraftingResultCode::InvalidMetadata);

    ObjectMetadata badMeasurementLabel = validMetadata();
    badMeasurementLabel.measurement.label = "bad\nlabel";
    assert(!isValidMeasurementMetadata(badMeasurementLabel.measurement));
    auto badMeasurementLabelValidation = validateObjectMetadata(badMeasurementLabel);
    assert(!badMeasurementLabelValidation.ok);
    assert(badMeasurementLabelValidation.code == DraftingResultCode::InvalidMetadata);

    ObjectMetadata longMeasurementLabel = validMetadata();
    longMeasurementLabel.measurement.label = std::string(kMetadataShortTextLimit + 1, 'x');
    assert(!isValidMeasurementMetadata(longMeasurementLabel.measurement));
    auto longMeasurementLabelValidation = validateObjectMetadata(longMeasurementLabel);
    assert(!longMeasurementLabelValidation.ok);
    assert(longMeasurementLabelValidation.code == DraftingResultCode::InvalidMetadata);

    return 0;
}
