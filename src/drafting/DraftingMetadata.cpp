#include "drafting/DraftingMetadata.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <string_view>
#include <tuple>
#include <utility>

namespace edi::drafting {

namespace {

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
    if (!isValidMetadataText(value, limit)) {
        if (value.size() > limit) {
            return DraftingMetadataValidationResult::rejected(
                DraftingResultCode::InvalidMetadata,
                std::string(fieldName) + " is too long");
        }
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

int twoDigitNumberAt(std::string_view value, std::size_t index)
{
    return ((value[index] - '0') * 10) + (value[index + 1] - '0');
}

int fourDigitNumberAt(std::string_view value, std::size_t index)
{
    return ((value[index] - '0') * 1000)
        + ((value[index + 1] - '0') * 100)
        + ((value[index + 2] - '0') * 10)
        + (value[index + 3] - '0');
}

bool isLeapYear(int year)
{
    return (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
}

int daysInMonth(int year, int month)
{
    switch (month) {
    case 1:
    case 3:
    case 5:
    case 7:
    case 8:
    case 10:
    case 12:
        return 31;
    case 4:
    case 6:
    case 9:
    case 11:
        return 30;
    case 2:
        return isLeapYear(year) ? 29 : 28;
    default:
        return 0;
    }
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

bool isValidMetadataText(std::string_view value, std::size_t limit)
{
    return value.size() <= limit && isPrintableAscii(value);
}

bool isValidMetadataTimestamp(std::string_view value)
{
    if (value.empty()) {
        return true;
    }
    if (value.size() != 20) {
        return false;
    }

    const bool shapeMatches = isDigitAt(value, 0)
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
    if (!shapeMatches) {
        return false;
    }

    const int year = fourDigitNumberAt(value, 0);
    const int month = twoDigitNumberAt(value, 5);
    const int day = twoDigitNumberAt(value, 8);
    const int hour = twoDigitNumberAt(value, 11);
    const int minute = twoDigitNumberAt(value, 14);
    const int second = twoDigitNumberAt(value, 17);

    return month >= 1
        && month <= 12
        && day >= 1
        && day <= daysInMonth(year, month)
        && hour >= 0
        && hour <= 23
        && minute >= 0
        && minute <= 59
        && second >= 0
        && second <= 59;
}

bool isValidMeasurementMetadata(const MeasurementMetadata &measurement)
{
    if (!isValidMetadataText(measurement.label, kMetadataShortTextLimit)) {
        return false;
    }
    if (!std::isfinite(measurement.canvasUnitsPerRealUnit)) {
        return false;
    }
    if (measurement.unit == MeasurementUnit::None) {
        return true;
    }
    return measurement.canvasUnitsPerRealUnit > 0.0;
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
    if (!isValidMetadataTimestamp(metadata.createdAt)) {
        return DraftingMetadataValidationResult::rejected(
            DraftingResultCode::InvalidMetadata,
            "created at must use YYYY-MM-DDTHH:MM:SSZ");
    }
    if (!isValidMetadataText(metadata.measurement.label, kMetadataShortTextLimit)) {
        if (metadata.measurement.label.size() > kMetadataShortTextLimit) {
            return DraftingMetadataValidationResult::rejected(DraftingResultCode::InvalidMetadata, "measurement label is too long");
        }
        return DraftingMetadataValidationResult::rejected(DraftingResultCode::InvalidMetadata, "measurement label must be printable ASCII");
    }
    if (!std::isfinite(metadata.measurement.canvasUnitsPerRealUnit)) {
        return DraftingMetadataValidationResult::rejected(
            DraftingResultCode::InvalidMetadata,
            "measurement canvas units per real unit must be finite");
    }
    if (metadata.measurement.unit != MeasurementUnit::None && metadata.measurement.canvasUnitsPerRealUnit <= 0.0) {
        return DraftingMetadataValidationResult::rejected(
            DraftingResultCode::InvalidMetadata,
            "measurement canvas units per real unit must be positive");
    }

    return DraftingMetadataValidationResult::accepted();
}

} // namespace edi::drafting
