// ScriptCommandBridge.h
//
// Purpose:
//   Declares the boundary between authored scripts and validated C++ commands.
//
// Expected contracts:
//   - Script-visible command request descriptions.
//   - Dry-run command plans.
//   - Capability checks.
//   - Translation from script intent to drafting/text command requests.
//
// Ownership rule:
//   The bridge owns translation and validation before mutation. C++ domain
//   commands remain the only mutation authority.
//
// Must not depend on:
//   - UI widgets.
//   - Raw private document storage.
//   - Format parser internals.
//
// Preserve later:
//   Scripts may request commands, but they must not receive pointers/references
//   that let them mutate app state directly.
