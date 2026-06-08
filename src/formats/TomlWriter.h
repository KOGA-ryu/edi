// TomlWriter.h
//
// Purpose:
//   Declares TOML writer adapters for human-authored static configuration.
//
// Expected contracts:
//   - Write app settings.
//   - Write project/workspace settings.
//   - Write static presets that are declarative enough for TOML.
//
// Ownership rule:
//   TOML writer projects typed C++ contracts into text. It does not own runtime
//   state and does not perform mutation.
//
// Must not depend on:
//   - UI widgets.
//   - Lua runtime.
//   - Raw domain storage internals beyond public contracts.
//
// Preserve later:
//   Emitted TOML should be stable, reviewable, and human-editable.
