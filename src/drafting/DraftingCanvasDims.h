#pragma once
namespace edi::drafting {
// Canvas presentation dimensions for the map/canvas authoring path. Each is DATA (named),
// per the no-hardcoded-dimensions rule. These are the 2D-CANVAS widths (the 3D realizer owns
// its own greybox dims). Values are the historical constants, now named — byte-identical.

// The default 2D corridor walkable width (canvas units), used where there is no room context
// to derive from (interactive corridors, and the degenerate roomless-map fallback).
inline constexpr double kDefaultCorridorWidth = 0.045;

// A generated corridor's width DERIVES from the dungeon: it is the narrowest room's short edge
// divided by this, so the corridor tracks room size (double the rooms -> double the corridor)
// and never overflows the wall it is carved into (it is at most 1/this of any edge). Stored as
// the DIVISOR (room short edge per corridor) so the derivation stays `edge / kRoomShortEdgePerCorridor`
// — a byte-identical division, not a multiply-by-fraction (which could drift the last bit).
inline constexpr double kRoomShortEdgePerCorridor = 3.0;

// A door leaf's band thickness as a fraction of the corridor width (so the leaf scales with the
// corridor at any size). Kept as the ratio expression so `corridorWidth * kDoorLeafToCorridorRatio`
// reproduces the historical 0.02 at the default 0.045 corridor exactly.
inline constexpr double kDoorLeafToCorridorRatio = 0.02 / 0.045;

// A corridor wall's thickness as a fraction of the corridor width (same scaling rationale).
inline constexpr double kCorridorWallToCorridorRatio = 0.015 / 0.045;

// The small offset a pasted/duplicated copy is nudged by (canvas units, both x and y) so it does not
// land exactly on its source and become invisible. A spacing dimension -> named data.
inline constexpr double kCopyNudgeOffset = 0.02;

// The ascii-map auto-fit fills this fraction of the unit board (~80% visual with margins), so any map
// renders centered with breathing room. A fit/padding dimension -> named data.
inline constexpr double kAsciiBoardFillFraction = 0.62;

// The default room wall thickness when a RoomSpec does not author one (canvas units). The historical
// default, now named.
inline constexpr double kDefaultRoomWallThickness = 0.1;

// The default thickness of a wall drawn with the wall tool (canvas units), when the user has not set one.
// Distinct from kDefaultRoomWallThickness (a RoomSpec's default) though they share today's value: the wall
// tool and the room spec are separate concepts, so keep separate data — if one default shifts later they
// must not be conflated.
inline constexpr double kDefaultWallToolThickness = 0.1;
} // namespace edi::drafting
