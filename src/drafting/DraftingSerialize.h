#pragma once

#include "drafting/DraftingDocument.h"
#include "formats/FormatResult.h"
#include "formats/MessagePackInspector.h"
#include "formats/MessagePackValue.h"

namespace edi::drafting {

// Pure document <-> MessagePack value conversion. No Qt, no file I/O.
// Schema: top-level map {schema:"edi.drawing", version:1, document:{...}}.
// Object bounds are recomputed on load, never stored. Unknown fields are
// ignored on read (forward compatibility); missing required fields are
// rejected.
inline constexpr const char *kDraftingDocumentSchema = "edi.drawing";
inline constexpr int kDraftingDocumentVersion = 1;

edi::formats::MsgPackValue draftingDocumentToValue(const DraftingDocument &document);
edi::formats::FormatResult<DraftingDocument> draftingDocumentFromValue(const edi::formats::MsgPackValue &value);

// Byte-level envelope: EDIM magic + version byte + MessagePack(top value).
// These are what the store reads and writes.
edi::formats::ByteBuffer encodeDraftingDocument(const DraftingDocument &document);
edi::formats::FormatResult<DraftingDocument> decodeDraftingDocument(const edi::formats::ByteBuffer &bytes,
                                                                    const std::string &source = {});

} // namespace edi::drafting
