# Handoff — <campaign-id>

> The per-campaign state. Each gate appends its result; the NEXT gate reads this
> first. Agents hand off THROUGH this file — they cannot message each other.
> Copy this template to `docs/handoffs/<campaign-id>.md` at campaign start.

- **Campaign**: <id>
- **Department**: edi-<dept>
- **Goal (one line)**:
- **Boundary (the question the reviewer gate must settle)**:

## Gate log

### Research gate — <date> — <agent/session>
- Question ("how do other repos solve this?"):
- Findings (cited — URL + what it supports):
- Verdict / what to carry into the reviewer gate:

### Reviewer gate — <date> — <agent/session>
- Where this belongs (ownership, naming):
- Repo fit / what is allowed — and NOT allowed:
- Duplication risk:
- Scope (the bounded slices for the builder):
- **Boundary settled? (yes/no)** — if no, what blocks it:

### Builder batch — <date> — <agent/session>
- Slices implemented (each with its commit):
- Green gate (build / ctest N-N / scan):
- Facts reported (what changed, what was verified):
- Noticed-but-didn't-do:

## Open questions / blockers
-

## Next
-
