#pragma once

#include "formats/FormatResult.h"
#include "formats/MessagePackInspector.h" // ByteBuffer + the EDIM envelope constants
#include "formats/MessagePackValue.h"
#include "zoo/AssetZoo.h"

#include <string>

namespace edi::zoo {

// Pure AssetZoo <-> MessagePack conversion. No Qt, no file I/O — the catalog's
// on-disk STORE (a later slice) wraps these with the file layer.
// Schema: top-level map {schema:"edi.zoo", version:1, zoo:{assets:[...]}}.
// Reads are ADDITIVE + TOLERANT (a missing optional key => the field's default),
// so later fields (sockets, the metadata grid) load into older files with no
// version bump — the same discipline as the drawing document's wall_visual/blocks.
edi::formats::MsgPackValue assetZooToValue(const AssetZoo &zoo);
edi::formats::FormatResult<AssetZoo> assetZooFromValue(const edi::formats::MsgPackValue &value);

// Byte-level envelope: the SHARED EDIM magic + version byte + MessagePack(top),
// the exact wrapper encodeDraftingDocument uses, so every edi binary file reads the
// same way.
edi::formats::ByteBuffer encodeAssetZoo(const AssetZoo &zoo);
edi::formats::FormatResult<AssetZoo> decodeAssetZoo(const edi::formats::ByteBuffer &bytes,
                                                    const std::string &source = {});

} // namespace edi::zoo
