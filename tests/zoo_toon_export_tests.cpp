// Asset zoo (pillar A) — realize bridge R1a: the curated-zoo → TOON manifest.
//
// BYTE-PINS the exporter against a fixture exercising every column form (multi-
// texture + typed socket, all-empty optionals, an outer-quoted name) and proves
// the curated-only filter (a non-curated record is absent). Also pins the
// reserved-char validator (zooManifestFieldProblem) on a `·`-textureRef, an `@`
// in a socket name, and a clean record. Pure int main()+assert, like the other
// zoo slices — links edi_zoo_core ONLY (no Qt, no drafting).
#include "zoo/AssetZoo.h"
#include "zoo/ZooToonExport.h"

#include <cassert>
#include <optional>
#include <string>

using namespace edi::zoo;

int main()
{
    // The fixture: four records in zoo order —
    //   (1) curated, multiple textures + a typed socket with coordinates,
    //   (2) curated, empty proxyRef / empty textures / empty sockets,
    //   (3) curated, name with a space (forces outer quoting),
    //   (4) NON-curated (must be excluded from the manifest).
    AssetZoo zoo;

    AssetRecord wall;
    wall.id = "asset_0001";
    wall.name = "crypt_wall";
    wall.category = "wall";
    wall.meshRef = "blender:crypt_wall.glb";
    wall.proxyRef = "block_0007";
    wall.curated = true;
    wall.textureRefs = {"stone_diffuse", "stone_normal"};
    wall.sockets = {AssetSocket{"north_door", "door", Anchor2D{1.5, -2.25}}};
    zoo.assets.push_back(wall);

    AssetRecord bare;
    bare.id = "asset_0002";
    bare.name = "stone_floor";
    bare.category = "floor";
    bare.meshRef = "blender:stone_floor.glb";
    bare.proxyRef = "";          // empty -> "" cell
    bare.curated = true;
    bare.textureRefs = {};       // empty -> "" cell
    bare.sockets = {};           // empty -> "" cell
    zoo.assets.push_back(bare);

    AssetRecord spaced;
    spaced.id = "asset_0003";
    spaced.name = "oak door";    // space forces outer quoting
    spaced.category = "door";
    spaced.meshRef = "blender:oak_door.glb";
    spaced.proxyRef = "block_0009";
    spaced.curated = true;
    spaced.textureRefs = {"oak_diffuse"};
    spaced.sockets = {AssetSocket{"hinge", "", Anchor2D{0.0, 3.0}}}; // empty type -> name:@x,y
    zoo.assets.push_back(spaced);

    AssetRecord draft;
    draft.id = "asset_0004";
    draft.name = "wip_pillar";
    draft.category = "column";
    draft.meshRef = "blender:wip_pillar.glb";
    draft.curated = false;       // NOT curated — excluded
    zoo.assets.push_back(draft);

    const std::string expected =
        "kind: zoo\n"
        "\n"
        "assets[3]{id,name,category,meshRef,proxyRef,textures,sockets}:\n"
        "  asset_0001,crypt_wall,wall,blender:crypt_wall.glb,block_0007,stone_diffuse·stone_normal,\"north_door:door@1.5,-2.25\"\n"
        "  asset_0002,stone_floor,floor,blender:stone_floor.glb,\"\",\"\",\"\"\n"
        "  asset_0003,\"oak door\",door,blender:oak_door.glb,block_0009,oak_diffuse,\"hinge:@0,3\"\n";

    const std::string actual = exportZooToToon(zoo);
    assert(actual == expected);

    // The validator flags a `·` in a textureRef.
    {
        AssetRecord bad;
        bad.textureRefs = {"stone·diffuse"};
        const std::optional<std::string> problem = zooManifestFieldProblem(bad);
        assert(problem.has_value());
    }

    // The validator flags an `@` in a socket name (a socket sub-grammar char).
    {
        AssetRecord bad;
        bad.sockets = {AssetSocket{"north@door", "door", Anchor2D{0.0, 0.0}}};
        const std::optional<std::string> problem = zooManifestFieldProblem(bad);
        assert(problem.has_value());
    }

    // A clean record (the fixture's wall) has no problem.
    {
        const std::optional<std::string> problem = zooManifestFieldProblem(wall);
        assert(!problem.has_value());
    }

    // A record carrying LEGITIMATE multibyte characters is accepted: `°` shares
    // its lead byte (0xC2) with `·` (0xC2 0xB7), so a naive byte-scan validator
    // falsely rejects it. The `·` must be matched as a 2-byte substring; the
    // ASCII `@ : ,` set stays byte-wise. No real reserved char is present here.
    {
        AssetRecord cleanMultibyte;
        cleanMultibyte.textureRefs = {"marble°"};
        cleanMultibyte.sockets = {AssetSocket{"slope", "30°", Anchor2D{0.0, 0.0}}};
        const std::optional<std::string> problem = zooManifestFieldProblem(cleanMultibyte);
        assert(!problem.has_value());
    }

    return 0;
}
