#pragma once

#include "drafting/DraftingTypes.h"

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace edi::drafting {

constexpr std::size_t kMetadataShortTextLimit = 128;
constexpr std::size_t kMetadataMeasurementNoteLimit = 512;

struct DraftingMetadataValidationResult {
    bool ok = false;
    DraftingResultCode code = DraftingResultCode::None;
    std::string message;

    static DraftingMetadataValidationResult accepted();
    static DraftingMetadataValidationResult rejected(DraftingResultCode code, std::string message);
};

struct DraftingMetadataUpdatePlan {
    bool ok = false;
    DraftingResultCode code = DraftingResultCode::None;
    std::string message;
    ObjectMetadata metadata;

    static DraftingMetadataUpdatePlan accepted(ObjectMetadata metadata);
    static DraftingMetadataUpdatePlan rejected(DraftingResultCode code, std::string message);
};

bool isValidMetadataText(std::string_view value, std::size_t limit);
bool isValidMetadataTimestamp(std::string_view value);
bool isValidMeasurementMetadata(const MeasurementMetadata &measurement);
bool isValidGuideVisualColor(std::string_view value);
bool isValidGuideVisualDashStyle(std::string_view value);
DraftingMetadataValidationResult validateObjectMetadata(const ObjectMetadata &metadata);
DraftingMetadataUpdatePlan planGuideVisualLabelUpdate(const ObjectMetadata &metadata, std::string label);
DraftingMetadataUpdatePlan planGuideVisualColorUpdate(const ObjectMetadata &metadata, std::string color);
DraftingMetadataUpdatePlan planGuideVisualDashStyleUpdate(const ObjectMetadata &metadata, std::string dashStyle);
DraftingMetadataUpdatePlan planGuideVisualLabelVisibleUpdate(const ObjectMetadata &metadata, bool visible);
DraftingMetadataUpdatePlan planDimensionVisualLabelVisibleUpdate(const ObjectMetadata &metadata, bool visible);
DraftingMetadataUpdatePlan planMeasurementNoteUpdate(const ObjectMetadata &metadata, std::string note);
// N3: the semantic/export fields. role is an enum (always valid); the three
// text fields validate as bounded printable text like the others.
DraftingMetadataUpdatePlan planObjectRoleUpdate(const ObjectMetadata &metadata, ObjectRole role);
DraftingMetadataUpdatePlan planObjectMaterialUpdate(const ObjectMetadata &metadata, std::string material);
DraftingMetadataUpdatePlan planObjectExportGroupUpdate(const ObjectMetadata &metadata, std::string exportGroup);
DraftingMetadataUpdatePlan planObjectTagsUpdate(const ObjectMetadata &metadata, std::vector<std::string> tags);

} // namespace edi::drafting
