// TomlReader.h
//
// Purpose:
//   Declares TOML reader adapters for human-authored static configuration.
//
// Expected contracts:
//   - Read app settings.
//   - Read project/workspace settings.
//   - Read theme/layout/tool preset/export preset declarations when they are
//     static and declarative.
//
// Ownership rule:
//   TOML reader converts TOML input into typed C++ contracts. It does not own
//   app state or mutate runtime state.
//
// Must not depend on:
//   - UI widgets.
//   - Drafting/text mutation internals.
//   - Lua recipe execution.
//
// Preserve later:
//   TOML is for human-authored static config, not canonical drawing or text
//   document state.
