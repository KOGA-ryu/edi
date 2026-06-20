# Closeout — dungeon-map-20260619-cleanup-d15-d18 (Seam B/C lock-tag parity + determinism-test scope)

Freezes two cleanup-clipboard items so future work does not re-litigate them. Status: **COMPLETE ✅**
(S1–S5 on `dept/dungeon-map`; NOT pushed — hub owns the origin bridge).

## What shipped (SHAs on dept/dungeon-map, base master @8da3bdb)
- **S1 `b6a5b24`** — `locked`/`keyId` on `DraftingDeclaredConnection` (model + comment).
- **S2 `8fc0a25`** — conditional `locked`/`key_id` in the `.edidraw` connection codec.
- **S3 `0102b79`** — carry the lock author→document through `createMapFromSpec`.
- **S4 `5f14e37`** — Seam C (document) export lock columns + the NEW reference Seam C golden.
- **S5 `3d02f9e`** — scope the determinism-test header to in-process; defer the byte guarantee to the
  regression sha256.

## D15 — the frozen contract (Seam B / Seam C lock-tag PARITY)
**Decision: Option A (full parity).** The document is edi's single self-describing neutral source, so
a lock tag must live in the document model and survive BOTH export seams identically — not be
authoring-only. The lock is a TAG the engine interprets; **edi branches on it NOWHERE except record /
serialize / export** (verified: only parse-record + `createMapFromSpec` carry + codec + TOON emit).
edi simulates no gate.

End-to-end, the lock now survives every path:
- author `.map.toml` → `MapConnectionSpec.locked/keyId` → **Seam B** TOON (was already true).
- author → `createMapFromSpec` → `DraftingDeclaredConnection.locked/keyId` (S3) → `.edidraw` codec
  (S2) → reload → **Seam C** TOON (S4). The break was `createMapFromSpec` copying only `type`
  (`DrawingDocumentController.cpp:3461`); it now copies the lock.

**Guardrails honored (do not regress):**
- **Additive / byte-identity.** The codec uses the `flags`/`bounded_by` CONDITIONAL pattern (emit
  `locked` only when true, `key_id` only when non-empty; missing ⇒ false/empty) — NOT the always-write
  `level` pattern. An unlocked connection serializes BYTE-IDENTICAL to before (the keys are absent).
  **No version bump** (EDIM envelope untouched).
- **Goldens.** Seam B reference golden sha256 `6c632293…b0e3` UNCHANGED (re-run, not re-blessed). A
  NEW Seam C reference golden (`map_regression_lock_tests.cpp`) pins the no-columns shape
  `connections[12]{from,to,type}` (this golden did NOT exist before — the Seam C path was previously
  only self-diffed in the determinism test, an unpinned blind spot the reviewer caught). Both seams
  emit the lock columns conditionally — absent when no connection is locked.

## D18 — the frozen scope (determinism test)
`map_determinism_tests` runs both controllers in ONE process; it catches same-process ordering/creation
non-determinism (the `unordered_map`→`std::map` class it guards) but CANNOT catch cross-process/ASLR
variance. The build-independent byte guarantee is owned by `map_regression_lock_tests`' recorded
sha256. Header rescoped to say so (Option b — chosen over a separate-process re-exec harness because
this is a NIT and the sha256 already provides the cross-build guarantee).

## Verification (planner re-ran independently)
- **Debug 118/118 green.** **Release 111/118** — the only failures are the 7 pre-existing **D01**
  drafting-core segfaults (`assert()`-elision under NDEBUG; owned by edi-drafting + hub, NOT this
  campaign — confirmed identical to clean master @8da3bdb). **Zero new failures** in either build type;
  all map/serialize/regression/determinism tests pass in BOTH.
- Seam B golden unchanged; Seam C no-columns golden added; locked round-trip shows columns turn on and
  Seam B/C agree. Neutrality grep clean (no behavioral branch on the lock). Scan clean. Reference
  dungeon renders (152KB).

## Superseded
- The proving-ground campaign's **deferred** "lock on DraftingDeclaredConnection + conditional
  `.edidraw` codec" slice is now DONE — `docs/closeouts/dungeon-map-proving-ground.md`'s deferral is
  closed.

## Not in scope (left for owners)
- **D01/D02** (Release-build segfaults + the missing Release green-gate leg) — edi-drafting + hub.
- Other clipboard items belong to other departments.
</content>
