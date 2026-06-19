#pragma once

#include "zoo/AssetZoo.h"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace edi::zoo {

// Pure ops over the AssetZoo catalog — FREE FUNCTIONS over the plain struct
// (data-oriented: no methods, no stateful "library" object). Mirrors the
// document's block/plug ops: an index-by-id read helper + validate-then-mutate
// writers, plus id minting that resumes above the catalog's highest serial.

// Format a serial as a zoo id ("asset_0001"). Kept local to the zoo so the module
// depends only on the format core, not the drafting core, for one small helper.
AssetId assetIdForSerial(int serial);

// Highest trailing-serial across existing ids (0 if empty), so a mint after a load
// resumes ABOVE every id already in the catalog — no id reuse, even after removes.
int highestAssetIdSerial(const AssetZoo &zoo);

// The next free id ("asset_<highest+1>").
AssetId mintAssetId(const AssetZoo &zoo);

// Factory: the identity fields set, the rest defaulted (uncurated, no refs).
AssetRecord makeAssetRecord(AssetId id, std::string name, std::string category);

// Read helpers.
std::optional<std::size_t> assetIndexById(const AssetZoo &zoo, const AssetId &id);
const AssetRecord *findAsset(const AssetZoo &zoo, const AssetId &id);

// The curation view: only greenlit assets are "in" the zoo.
std::vector<const AssetRecord *> curatedAssets(const AssetZoo &zoo);

// Mutations. addAsset rejects an empty or duplicate id (returns false); the caller
// mints the id with mintAssetId first. curate/remove return false when absent.
bool addAsset(AssetZoo &zoo, AssetRecord record);
bool curateAsset(AssetZoo &zoo, const AssetId &id, bool curated);
bool removeAsset(AssetZoo &zoo, const AssetId &id);

// Socket ops over a single record (uniqueness is WITHIN the record — two records
// may each carry a "door" socket). addSocket mirrors addAsset: it rejects an empty
// name or a name already on the record (returns false), else appends and returns true.
bool addSocket(AssetRecord &record, AssetSocket socket);

// Find a socket by name (nullptr if absent). NOTE: the returned pointer dangles if
// record.sockets is later mutated (push/erase) — same caveat as findAsset.
const AssetSocket *findSocket(const AssetRecord &record, const std::string &name);

} // namespace edi::zoo
