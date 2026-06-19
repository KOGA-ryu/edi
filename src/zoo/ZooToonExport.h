#pragma once

#include "zoo/AssetZoo.h"

#include <optional>
#include <string>

namespace edi::zoo {

// The curated-zoo → TOON manifest bridge (realize chain R1a).
//
// The zoo is C++/MessagePack; the realizer is Python/TOON. Reading MessagePack
// in Python was REJECTED (a reviewer boundary gate) — Python stays TOON-only —
// so the bridge is a NEUTRAL TOON manifest emitted on the C++ side. This module
// lives in edi_zoo_core (which links ONLY edi_format_core, no Qt, no drafting):
// it touches nothing but AssetRecord/AssetSocket/Anchor2D/std::string, so it
// belongs here and must NOT pull in drafting/recipe/io/Qt — that isolation is
// the LAW of the zoo core.
//
// The TOON grammar mirrors src/io/MapToonExport.cpp; its num()/cell()/joinFlags
// helpers are COPIED (not shared) into ZooToonExport.cpp because MapToonExport
// lives in a drafting-dependent TU — including it would drag drafting into the
// zoo and break the boundary above. The ~25-line duplication is the deliberate
// cost of holding that line.

// Project the zoo to a TOON document with a single `assets[N]{...}:` section
// listing ONLY the curated records (N = curated count), in zoo order. Total:
// always returns a string. PRECONDITION: every curated record passes
// zooManifestFieldProblem (the future curate/persist path = R1c enforces it);
// exportZooToToon assumes valid input and does NOT re-check.
std::string exportZooToToon(const AssetZoo &zoo);

// Reserved-char guard for the manifest's structural separators. The `·`/`@`/`:`
// /`,` characters are separators inside the `·`-joined texture run and the
// `name:type@x,y` socket sub-grammar; AssetRecord fields are open-vocab free-form
// strings, so a value carrying one would SILENTLY corrupt the round-trip. Returns
// a human-readable problem string for the first violation, else std::nullopt:
//   - any textureRef contains `·`; or
//   - any socket name or type contains `·`, `@`, `:`, or `,`.
// (category/name/meshRef/proxyRef are single cells protected by cell() quoting,
// so they need no guard — only the ·-joined run and socket inner-fields do.)
//
// PRECONDITION callers (R1c, before curating) must check this. The precedent is
// recipeScriptParamKeyProblem — a problem-returning validator for a cross-language
// key/field constraint, checked at the write boundary rather than swallowed here.
std::optional<std::string> zooManifestFieldProblem(const AssetRecord &record);

} // namespace edi::zoo
