#include "zoo/AssetZooOps.h"

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <sstream>
#include <utility>

namespace edi::zoo {

namespace {

// Recover the trailing numeric suffix of an "asset_NNNN" id (0 if none) — the same
// recovery the document's highestDocumentIdSerial uses, kept private to the module
// so the zoo need not link the drafting core for it.
int idTrailingSerial(const AssetId &id)
{
    std::size_t end = id.size();
    std::size_t begin = end;
    while (begin > 0 && std::isdigit(static_cast<unsigned char>(id[begin - 1]))) {
        --begin;
    }
    if (begin == end) {
        return 0;
    }
    try {
        return std::stoi(id.substr(begin, end - begin));
    } catch (...) {
        return 0;
    }
}

} // namespace

AssetId assetIdForSerial(int serial)
{
    std::ostringstream stream;
    stream << "asset_" << std::setfill('0') << std::setw(4) << serial;
    return stream.str();
}

int highestAssetIdSerial(const AssetZoo &zoo)
{
    int highest = 0;
    for (const AssetRecord &asset : zoo.assets) {
        highest = std::max(highest, idTrailingSerial(asset.id));
    }
    return highest;
}

AssetId mintAssetId(const AssetZoo &zoo)
{
    return assetIdForSerial(highestAssetIdSerial(zoo) + 1);
}

AssetRecord makeAssetRecord(AssetId id, std::string name, std::string category)
{
    AssetRecord record;
    record.id = std::move(id);
    record.name = std::move(name);
    record.category = std::move(category);
    return record;
}

std::optional<std::size_t> assetIndexById(const AssetZoo &zoo, const AssetId &id)
{
    for (std::size_t i = 0; i < zoo.assets.size(); ++i) {
        if (zoo.assets[i].id == id) {
            return i;
        }
    }
    return std::nullopt;
}

const AssetRecord *findAsset(const AssetZoo &zoo, const AssetId &id)
{
    const std::optional<std::size_t> index = assetIndexById(zoo, id);
    return index ? &zoo.assets[*index] : nullptr;
}

std::vector<const AssetRecord *> curatedAssets(const AssetZoo &zoo)
{
    std::vector<const AssetRecord *> result;
    for (const AssetRecord &asset : zoo.assets) {
        if (asset.curated) {
            result.push_back(&asset);
        }
    }
    return result;
}

bool addAsset(AssetZoo &zoo, AssetRecord record)
{
    // The zoo owns id uniqueness: an empty id (caller forgot to mint) or a
    // collision is rejected rather than silently shadowing an existing asset.
    if (record.id.empty() || assetIndexById(zoo, record.id)) {
        return false;
    }
    zoo.assets.push_back(std::move(record));
    return true;
}

bool curateAsset(AssetZoo &zoo, const AssetId &id, bool curated)
{
    const std::optional<std::size_t> index = assetIndexById(zoo, id);
    if (!index) {
        return false;
    }
    zoo.assets[*index].curated = curated;
    return true;
}

bool removeAsset(AssetZoo &zoo, const AssetId &id)
{
    const std::optional<std::size_t> index = assetIndexById(zoo, id);
    if (!index) {
        return false;
    }
    zoo.assets.erase(zoo.assets.begin() + static_cast<std::ptrdiff_t>(*index));
    return true;
}

} // namespace edi::zoo
