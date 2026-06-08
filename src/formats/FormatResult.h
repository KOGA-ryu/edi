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
// Preserve later:
//   Format adapters should return typed C++ contracts or diagnostics. Raw format
//   objects must not leak into app logic.
//
// Surface contract:
//   - Primary responsibility: define common adapter result shapes.
//   - Allowed data: typed value payloads, source labels, warning records, error
//     records, codes, and messages.
//   - Call direction: format adapters produce FormatResult; callers inspect
//     results before applying typed values.
//   - Mutation authority: none; result transport only.
//   - Unit convention: no domain units.
//   - Identity policy: source paths/labels identify input/output artifacts.
//   - Lifetime: owns diagnostic text and typed result values.
//   - Composition boundary: shared result type, not parser implementation.
//   - Promotion path: add severity/categories here if adapters need richer
//     diagnostics.
