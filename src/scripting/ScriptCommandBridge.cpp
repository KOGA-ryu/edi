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
