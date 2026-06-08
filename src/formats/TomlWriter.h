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
// Preserve later:
//   Emitted TOML should be stable, reviewable, and human-editable.
//
// Surface contract:
//   - Primary responsibility: declare contract-to-TOML write adapters.
//   - Allowed data: typed static config contracts, output options, source labels,
//     and FormatResult diagnostics.
//   - Call direction: settings/project save services call writer adapters.
//   - Mutation authority: translation only.
//   - Unit convention: write units explicitly where config needs them.
//   - Identity policy: typed IDs become stable text fields.
//   - Lifetime: returns owned output text/buffers.
//   - Composition boundary: writer does not choose settings behavior.
//   - Promotion path: stable formatting policy can split into a formatter.
