#include "drafting/DraftingMetadata.h"

#include <cassert>
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

    ObjectMetadata badVersion = validMetadata();
    badVersion.schemaVersion = 0;
    auto badVersionValidation = validateObjectMetadata(badVersion);
    assert(!badVersionValidation.ok);
    assert(badVersionValidation.code == DraftingResultCode::InvalidMetadata);

    ObjectMetadata badTimestamp = validMetadata();
    badTimestamp.createdAt = "June 8";
    auto badTimestampValidation = validateObjectMetadata(badTimestamp);
    assert(!badTimestampValidation.ok);
    assert(badTimestampValidation.code == DraftingResultCode::InvalidMetadata);

    ObjectMetadata badAuthor = validMetadata();
    badAuthor.author = "bad\nauthor";
    auto badAuthorValidation = validateObjectMetadata(badAuthor);
    assert(!badAuthorValidation.ok);
    assert(badAuthorValidation.code == DraftingResultCode::InvalidMetadata);

    ObjectMetadata longAuthor = validMetadata();
    longAuthor.author = std::string(129, 'x');
    auto longAuthorValidation = validateObjectMetadata(longAuthor);
    assert(!longAuthorValidation.ok);
    assert(longAuthorValidation.code == DraftingResultCode::InvalidMetadata);

    ObjectMetadata longSource = validMetadata();
    longSource.source = std::string(129, 'x');
    auto longSourceValidation = validateObjectMetadata(longSource);
    assert(!longSourceValidation.ok);
    assert(longSourceValidation.code == DraftingResultCode::InvalidMetadata);

    ObjectMetadata longToolProvenance = validMetadata();
    longToolProvenance.toolProvenance = std::string(129, 'x');
    auto longToolProvenanceValidation = validateObjectMetadata(longToolProvenance);
    assert(!longToolProvenanceValidation.ok);
    assert(longToolProvenanceValidation.code == DraftingResultCode::InvalidMetadata);

    ObjectMetadata longMeasurementNote = validMetadata();
    longMeasurementNote.measurementNote = std::string(513, 'x');
    auto longMeasurementNoteValidation = validateObjectMetadata(longMeasurementNote);
    assert(!longMeasurementNoteValidation.ok);
    assert(longMeasurementNoteValidation.code == DraftingResultCode::InvalidMetadata);

    return 0;
}
