// FormatResult.h
//
// Purpose:
//   Defines shared result, warning, and error contracts for all format adapters.
//
// Expected contracts:
//   - FormatError with source label/path, code, and message.
//   - FormatWarning with source label/path, code, and message.
//   - FormatResult<T> carrying success state, typed value, warnings, and errors.
//
// Ownership rule:
//   FormatResult owns adapter diagnostics only. It does not own parsing policy,
//   app mutation, or domain object storage.
//
// Must not depend on:
//   - Drafting/text internals beyond public typed contracts.
//   - UI widgets.
//   - Lua runtime state.
//
// Preserve later:
//   Format adapters should return typed C++ contracts or diagnostics. Raw format
//   objects must not leak into app logic.
