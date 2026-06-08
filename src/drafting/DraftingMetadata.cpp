#include "drafting/DraftingMetadata.h"

#include <algorithm>
#include <cctype>
#include <string_view>
#include <tuple>
#include <utility>

namespace edi::drafting {

namespace {

constexpr std::size_t kMetadataShortTextLimit = 128;
constexpr std::size_t kMetadataMeasurementNoteLimit = 512;

bool isPrintableAscii(std::string_view value)
{
    return std::all_of(value.begin(), value.end(), [](unsigned char ch) {
        return ch >= 0x20 && ch <= 0x7e;
    });
}

DraftingMetadataValidationResult validateMetadataText(
    std::string_view fieldName,
    std::string_view value,
    std::size_t limit)
{
    if (value.size() > limit) {
        return DraftingMetadataValidationResult::rejected(
            DraftingResultCode::InvalidMetadata,
            std::string(fieldName) + " is too long");
    }
    if (!isPrintableAscii(value)) {
        return DraftingMetadataValidationResult::rejected(
            DraftingResultCode::InvalidMetadata,
            std::string(fieldName) + " must be printable ASCII");
    }
    return DraftingMetadataValidationResult::accepted();
}

bool isDigitAt(std::string_view value, std::size_t index)
{
    return index < value.size() && std::isdigit(static_cast<unsigned char>(value[index])) != 0;
}

bool hasCharAt(std::string_view value, std::size_t index, char expected)
{
    return index < value.size() && value[index] == expected;
}

bool isUtcTimestampShape(std::string_view value)
{
    if (value.empty()) {
        return true;
    }
    if (value.size() != 20) {
        return false;
    }

    return isDigitAt(value, 0)
        && isDigitAt(value, 1)
        && isDigitAt(value, 2)
        && isDigitAt(value, 3)
        && hasCharAt(value, 4, '-')
        && isDigitAt(value, 5)
        && isDigitAt(value, 6)
        && hasCharAt(value, 7, '-')
        && isDigitAt(value, 8)
        && isDigitAt(value, 9)
        && hasCharAt(value, 10, 'T')
        && isDigitAt(value, 11)
        && isDigitAt(value, 12)
        && hasCharAt(value, 13, ':')
        && isDigitAt(value, 14)
        && isDigitAt(value, 15)
        && hasCharAt(value, 16, ':')
        && isDigitAt(value, 17)
        && isDigitAt(value, 18)
        && hasCharAt(value, 19, 'Z');
}

} // namespace

DraftingMetadataValidationResult DraftingMetadataValidationResult::accepted()
{
    return {true, DraftingResultCode::None, {}};
}

DraftingMetadataValidationResult DraftingMetadataValidationResult::rejected(DraftingResultCode code, std::string message)
{
    return {false, code, std::move(message)};
}

DraftingMetadataValidationResult validateObjectMetadata(const ObjectMetadata &metadata)
{
    if (metadata.schemaVersion == 0) {
        return DraftingMetadataValidationResult::rejected(DraftingResultCode::InvalidMetadata, "metadata schema version is required");
    }

    for (const auto &[fieldName, value, limit] : {
             std::tuple<std::string_view, std::string_view, std::size_t>{"author", metadata.author, kMetadataShortTextLimit},
             std::tuple<std::string_view, std::string_view, std::size_t>{"source", metadata.source, kMetadataShortTextLimit},
             std::tuple<std::string_view, std::string_view, std::size_t>{"tool provenance", metadata.toolProvenance, kMetadataShortTextLimit},
             std::tuple<std::string_view, std::string_view, std::size_t>{"measurement note", metadata.measurementNote, kMetadataMeasurementNoteLimit},
         }) {
        auto textValidation = validateMetadataText(fieldName, value, limit);
        if (!textValidation.ok) {
            return textValidation;
        }
    }

    auto createdAtTextValidation = validateMetadataText("created at", metadata.createdAt, kMetadataShortTextLimit);
    if (!createdAtTextValidation.ok) {
        return createdAtTextValidation;
    }
    if (!isUtcTimestampShape(metadata.createdAt)) {
        return DraftingMetadataValidationResult::rejected(
            DraftingResultCode::InvalidMetadata,
            "created at must use YYYY-MM-DDTHH:MM:SSZ");
    }

    return DraftingMetadataValidationResult::accepted();
}

} // namespace edi::drafting
