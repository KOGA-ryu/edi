#include "zoo/ZooToonExport.h"

#include <cstdio>
#include <sstream>
#include <string>
#include <vector>

namespace edi::zoo {
namespace {

// --- TOON shape helpers, COPIED from src/io/MapToonExport.cpp -------------
// These three are character-for-character the map exporter's helpers. They are
// duplicated here ON PURPOSE: MapToonExport.cpp lives in a drafting-dependent TU,
// and edi_zoo_core links only edi_format_core. Including the shared file would
// pull drafting into the zoo and break the core boundary, so we pay the ~25-line
// duplication to keep the dependency graph honest. If the map grammar changes,
// these must be kept in lock-step by hand (both emit the SAME TOON wire).

// Minimal numeric form: "%g" prints 21, 47.5, 6 — never 21.000000 — matching how
// the realizer's _xy()/float() reads each coordinate.
std::string num(double v)
{
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "%g", v);
    return std::string(buffer);
}

// Quote a TOON cell only when it carries a delimiter/space (so simple tokens like
// `wall` stay bare, while a coordinate-bearing cell like `north:door@1.5,-2.25`
// is quoted). Escapes an embedded `"` as `\"`, mirroring _split_cells's unquote.
std::string cell(const std::string &value)
{
    bool quote = value.empty();
    for (const char c : value) {
        if (c == ',' || c == '"' || c == ' ' || c == '\t' || c == '\n') {
            quote = true;
            break;
        }
    }
    if (!quote) {
        return value;
    }
    std::string out = "\"";
    for (const char c : value) {
        if (c == '"') {
            out += '\\';
        }
        out += c;
    }
    out += '"';
    return out;
}

// Join neutral tokens into the single `·`-separated run a TOON list-column carries.
// Empty list -> empty string, which the caller passes through cell() like any cell.
// The middle dot is chosen so the run is ONE cell with no comma/space.
std::string joinFlags(const std::vector<std::string> &flags)
{
    std::string out;
    for (std::size_t i = 0; i < flags.size(); ++i) {
        if (i != 0) {
            out += "·"; // U+00B7 MIDDLE DOT
        }
        out += flags[i];
    }
    return out;
}

// --- Zoo-specific socket encoding ----------------------------------------
// Each socket encodes as the FIXED sub-grammar `name:type@x,y` — always, even
// when type is empty (`name:@x,y`) — and the sockets are `·`-joined into one
// run. The run carries commas (from x,y), so cell() will quote it, exactly like
// an origin/anchor pair elsewhere. The structural chars `·`/`@`/`:`/`,` are
// guarded by zooManifestFieldProblem at the curate boundary, not here.
std::string joinSockets(const std::vector<AssetSocket> &sockets)
{
    std::string out;
    for (std::size_t i = 0; i < sockets.size(); ++i) {
        if (i != 0) {
            out += "·"; // U+00B7 MIDDLE DOT — same separator as joinFlags
        }
        const AssetSocket &s = sockets[i];
        out += s.name;
        out += ':';
        out += s.type; // empty type -> `name:@x,y`, the fixed grammar still holds
        out += '@';
        out += num(s.anchor.x);
        out += ',';
        out += num(s.anchor.y);
    }
    return out;
}

bool containsAny(const std::string &value, const char *const reserved)
{
    for (const char ch : value) {
        for (const char *r = reserved; *r != '\0'; ++r) {
            if (ch == *r) {
                return true;
            }
        }
    }
    return false;
}

} // namespace

std::string exportZooToToon(const AssetZoo &zoo)
{
    // Count the curated records first — N must be the curated count, not the
    // total — so the section header's [N] matches the rows actually emitted.
    std::size_t curatedCount = 0;
    for (const AssetRecord &record : zoo.assets) {
        if (record.curated) {
            ++curatedCount;
        }
    }

    std::ostringstream out;
    // Preamble: `kind: zoo` then a blank section separator, mirroring
    // writeMapHeader's `kind: map` + blank line.
    out << "kind: zoo\n";
    out << "\n";

    out << "assets[" << curatedCount
        << "]{id,name,category,meshRef,proxyRef,textures,sockets}:\n";
    for (const AssetRecord &record : zoo.assets) {
        if (!record.curated) {
            continue; // curated-only: a non-greenlit record is not in the manifest
        }
        // ALWAYS emit all 7 columns — this is a brand-new section with no legacy
        // byte-identity to preserve, so there is no conditional-column logic.
        out << "  " << cell(record.id)
            << "," << cell(record.name)
            << "," << cell(record.category)
            << "," << cell(record.meshRef)
            << "," << cell(record.proxyRef)              // empty -> "" via cell()
            << "," << cell(joinFlags(record.textureRefs)) // ·-joined; empty -> ""
            << "," << cell(joinSockets(record.sockets))   // name:type@x,y run; empty -> ""
            << "\n";
    }
    return out.str();
}

std::optional<std::string> zooManifestFieldProblem(const AssetRecord &record)
{
    // The run separator `·` (U+00B7) is two UTF-8 bytes (0xC2 0xB7). It MUST be
    // matched as a SUBSTRING (find on the 2-byte sequence), never byte-scanned:
    // a byte-wise containsAny on "·" rejects ANY field carrying either byte alone
    // — so legitimate open-vocab values like `marble°` (°=0xC2 0xB0) or a socket
    // type `30°` would be falsely blocked. containsAny stays valid ONLY for the
    // single-byte ASCII chars `@ : ,`, which cannot appear as a UTF-8 continuation
    // byte and so carry no false-positive class.

    // A textureRef may not carry the run separator `·` — its only reserved char.
    for (const std::string &texture : record.textureRefs) {
        if (texture.find("·") != std::string::npos) {
            return "textureRef '" + texture + "' must not contain '·'";
        }
    }
    // A socket's name/type may not carry the `·` run separator (substring match)
    // nor any of the ASCII socket sub-grammar chars `@ : ,` (safe to byte-scan).
    for (const AssetSocket &socket : record.sockets) {
        if (socket.name.find("·") != std::string::npos
            || containsAny(socket.name, "@:,")) {
            return "socket name '" + socket.name
                + "' must not contain '·', '@', ':', or ','";
        }
        if (socket.type.find("·") != std::string::npos
            || containsAny(socket.type, "@:,")) {
            return "socket type '" + socket.type
                + "' must not contain '·', '@', ':', or ','";
        }
    }
    return std::nullopt;
}

} // namespace edi::zoo
