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
// Preserve later:
//   Scripts may request commands, but they must not receive pointers/references
//   that let them mutate app state directly.
//
// Surface contract:
//   - Primary responsibility: declare script-intent to C++ command translation.
//   - Allowed data: validated recipe steps, capability grants, dry-run command
//     plans, public drafting/text command values, and diagnostics.
//   - Call direction: script runtime calls bridge; bridge produces commands for
//     domain command executors.
//   - Mutation authority: translation and dry-run planning; execution delegates
//     to domain command layers.
//   - Unit convention: script units must be translated into command units before
//     reaching domain mutation.
//   - Identity policy: scripts reference stable public IDs.
//   - Lifetime: bridge owns no document storage.
//   - Composition boundary: separates authored recipes from command authority.
//   - Promotion path: add permission/capability policy here before enabling Lua.
