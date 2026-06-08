// ScriptCommandBridge.cpp
//
// Implementation responsibility:
//   Converts validated script intent into command plans or command requests.
//
// Belongs here:
//   - Dry-run planning.
//   - Capability enforcement.
//   - Mapping script-level recipe steps to public drafting/text commands.
//   - Returning diagnostics without mutating when validation fails.
//
// Must be delegated elsewhere:
//   - Actual document mutation belongs in C++ command layers.
//   - Lua parsing/execution belongs in a later Lua runtime adapter.
//   - Format IO belongs in format adapters.
//
// Boundary note:
//   This file is a firewall. It should make script power explicit, reviewable,
//   and revocable.
//
// Surface contract:
//   - Primary responsibility: implement command planning from script requests.
//   - Allowed data: validated recipe steps, capability state, public command
//     requests, dry-run output, and diagnostics.
//   - Call direction: called by a future Lua runtime adapter.
//   - Mutation authority: dry-run and translation by default; actual mutation
//     only through domain command executors.
//   - Unit convention: converts recipe units to explicit command payload units.
//   - Identity policy: resolves public IDs only through command-facing handles.
//   - Lifetime: no retained references to mutable stores.
//   - Composition boundary: bridge does not parse Lua source.
//   - Promotion path: audit logging can be added here for script-driven changes.
