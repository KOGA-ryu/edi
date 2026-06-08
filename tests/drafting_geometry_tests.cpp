// drafting_geometry_tests.cpp
//
// Contract family:
//   Pure drafting geometry math.
//
// Intended coverage:
//   - Bounds for point, line, rectangle, circle, polygon, and polyline.
//   - Translation and transform helpers.
//   - Distance, dimensions, and area primitives.
//   - Hit scoring and handle anchor calculations when implemented.
//
// Should not test here:
//   - Document storage mutation.
//   - Command dispatch.
//   - Rendering pixels.
//   - Format conversion.
//
// Later role:
//   These tests should become executable documentation proving geometry is
//   deterministic, finite, and independent from app state.
