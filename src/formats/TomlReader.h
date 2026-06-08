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
// Preserve later:
//   TOML is for human-authored static config, not canonical drawing or text
//   document state.
//
// Surface contract:
//   - Primary responsibility: declare TOML-to-contract read adapters.
//   - Allowed data: TOML text/bytes/path labels and typed config output
//     contracts.
//   - Call direction: persistence/settings code calls reader adapters.
//   - Mutation authority: translation only.
//   - Unit convention: config values should declare their own units in typed
//     contracts.
//   - Identity policy: config keys map to typed fields; project IDs remain typed
//     values after conversion.
//   - Lifetime: parsed TOML exists only during conversion.
//   - Composition boundary: adapter translates; app applies settings later.
//   - Promotion path: multiple TOML schemas can split into focused readers.
