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

bool isHexDigit(char value)
{
    return (value >= '0' && value <= '9')
        || (value >= 'a' && value <= 'f')
        || (value >= 'A' && value <= 'F');
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

DraftingMetadataUpdatePlan DraftingMetadataUpdatePlan::accepted(ObjectMetadata metadata)
{
    DraftingMetadataUpdatePlan result;
    result.ok = true;
    result.code = DraftingResultCode::None;
    result.metadata = std::move(metadata);
    return result;
}

DraftingMetadataUpdatePlan DraftingMetadataUpdatePlan::rejected(DraftingResultCode code, std::string message)
{
    DraftingMetadataUpdatePlan result;
    result.ok = false;
    result.code = code;
    result.message = std::move(message);
    return result;
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

bool isValidGuideVisualColor(std::string_view value)
{
    if (value.size() != 7 || value.front() != '#') {
        return false;
    }
    return std::all_of(value.begin() + 1, value.end(), isHexDigit);
}

bool isValidGuideVisualDashStyle(std::string_view value)
{
    return value == "solid"
        || value == "dash"
        || value == "dot";
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
             std::tuple<std::string_view, std::string_view, std::size_t>{"material", metadata.material, kMetadataShortTextLimit},
             std::tuple<std::string_view, std::string_view, std::size_t>{"export group", metadata.exportGroup, kMetadataShortTextLimit},
         }) {
        auto textValidation = validateMetadataText(fieldName, value, limit);
        if (!textValidation.ok) {
            return textValidation;
        }
    }
    // Tags are free text but bounded the same way, each tag independently —
    // an open vocabulary still can't carry a megabyte or a control byte.
    for (const std::string &tag : metadata.tags) {
        auto tagValidation = validateMetadataText("tag", tag, kMetadataShortTextLimit);
        if (!tagValidation.ok) {
            return tagValidation;
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
    auto guideLabelValidation = validateMetadataText("guide label", metadata.guideVisual.label, kMetadataShortTextLimit);
    if (!guideLabelValidation.ok) {
        return guideLabelValidation;
    }
    if (!isValidGuideVisualColor(metadata.guideVisual.color)) {
        return DraftingMetadataValidationResult::rejected(
            DraftingResultCode::InvalidMetadata,
            "guide visual color must be #RRGGBB");
    }
    if (!isValidGuideVisualDashStyle(metadata.guideVisual.dashStyle)) {
        return DraftingMetadataValidationResult::rejected(
            DraftingResultCode::InvalidMetadata,
            "guide visual dash style is invalid");
    }

    return DraftingMetadataValidationResult::accepted();
}

namespace {

DraftingMetadataUpdatePlan validatedMetadataUpdate(ObjectMetadata metadata)
{
    const DraftingMetadataValidationResult validation = validateObjectMetadata(metadata);
    if (!validation.ok) {
        return DraftingMetadataUpdatePlan::rejected(validation.code, validation.message);
    }
    return DraftingMetadataUpdatePlan::accepted(std::move(metadata));
}

} // namespace

DraftingMetadataUpdatePlan planGuideVisualLabelUpdate(const ObjectMetadata &metadata, std::string label)
{
    ObjectMetadata next = metadata;
    next.guideVisual.label = std::move(label);
    return validatedMetadataUpdate(std::move(next));
}

DraftingMetadataUpdatePlan planGuideVisualColorUpdate(const ObjectMetadata &metadata, std::string color)
{
    ObjectMetadata next = metadata;
    next.guideVisual.color = std::move(color);
    return validatedMetadataUpdate(std::move(next));
}

DraftingMetadataUpdatePlan planGuideVisualDashStyleUpdate(const ObjectMetadata &metadata, std::string dashStyle)
{
    ObjectMetadata next = metadata;
    next.guideVisual.dashStyle = std::move(dashStyle);
    return validatedMetadataUpdate(std::move(next));
}

DraftingMetadataUpdatePlan planGuideVisualLabelVisibleUpdate(const ObjectMetadata &metadata, bool visible)
{
    ObjectMetadata next = metadata;
    next.guideVisual.showLabel = visible;
    return validatedMetadataUpdate(std::move(next));
}

DraftingMetadataUpdatePlan planDimensionVisualLabelVisibleUpdate(const ObjectMetadata &metadata, bool visible)
{
    ObjectMetadata next = metadata;
    next.dimensionVisual.showLabel = visible;
    return validatedMetadataUpdate(std::move(next));
}

DraftingMetadataUpdatePlan planMeasurementNoteUpdate(const ObjectMetadata &metadata, std::string note)
{
    ObjectMetadata next = metadata;
    next.measurementNote = std::move(note);
    return validatedMetadataUpdate(std::move(next));
}

DraftingMetadataUpdatePlan planObjectRoleUpdate(const ObjectMetadata &metadata, ObjectRole role)
{
    ObjectMetadata next = metadata;
    next.role = role; // enum: always representable, no text validation needed
    return validatedMetadataUpdate(std::move(next));
}

DraftingMetadataUpdatePlan planObjectMaterialUpdate(const ObjectMetadata &metadata, std::string material)
{
    ObjectMetadata next = metadata;
    next.material = std::move(material);
    return validatedMetadataUpdate(std::move(next));
}

DraftingMetadataUpdatePlan planObjectExportGroupUpdate(const ObjectMetadata &metadata, std::string exportGroup)
{
    ObjectMetadata next = metadata;
    next.exportGroup = std::move(exportGroup);
    return validatedMetadataUpdate(std::move(next));
}

DraftingMetadataUpdatePlan planObjectTagsUpdate(const ObjectMetadata &metadata, std::vector<std::string> tags)
{
    ObjectMetadata next = metadata;
    next.tags = std::move(tags);
    return validatedMetadataUpdate(std::move(next));
}

} // namespace edi::drafting
