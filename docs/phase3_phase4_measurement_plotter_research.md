# EDI Phase 3-4 Measurement And Plotter Research

This document records user-supplied research patterns from CAD and plotting tools, including AutoCAD, LibreCAD, QCAD, Inkscape, LightBurn, and industrial vinyl-cutter workflows. The intent is to guide EDI Phase 3 and Phase 4 decisions around measurement, dimensions, plotter preview, stroke planning, hatch/fill behavior, and calibration.

This is planning input. It does not claim that the listed behaviors are already implemented in EDI.

## Phase 3-4 Focus

Phase 3 should make EDI useful for inspecting and annotating precise drawings:

- Quick measurement.
- Dedicated measurement commands.
- Dimension objects.
- Drafting rules for readable annotations.
- Numeric dimension editing.

Phase 4 should make EDI useful for physical output preparation:

- Plotter preview.
- Stroke and travel visualization.
- Plot job ordering.
- Hatch and fill generation.
- Calibration routines.
- Plotter safety warnings.

The shared rule is that measurement, dimensions, generated plot segments, and calibration should derive from typed C++ drafting state. They should not become parallel truth.

## Measurement Tools

### Quick Measurement

AutoCAD-style quick measurement provides live feedback under the cursor. It can show line length, circle radius, included angles, area, and perimeter without creating persistent geometry. Inkscape-style measurement also shows length and angle live as the measurement line passes over geometry.

EDI should support a non-mutating quick-measure mode:

- Hover over a line: show length.
- Hover over a circle: show radius and diameter.
- Hover near joined lines: show angle.
- Hover or click inside closed geometry later: show area and perimeter.
- Show measured values in the inspector/status surface.
- Do not create drawing objects unless the user explicitly commits a dimension.

### Dedicated Measurement Commands

QCAD separates measurement commands under an Info-style menu. Users choose the exact measurement type, then pick points or objects.

EDI should support explicit measurement commands:

- Distance between two points.
- Distance from object to point.
- Angle between two lines.
- Total selected path length.
- Area and perimeter for closed shapes.
- Selected object measurement summary.

The result can be displayed as transient inspector output first. Persistent measurement logs can come later.

### Measurement Output

Measurement output should be precise, unit-aware, and separate from geometry mutation:

- Display in current project units.
- Respect inspector precision and rounding settings.
- Include raw normalized value where useful for debugging.
- Identify source object ids.
- Indicate whether the result is exact, derived, approximate, or unsupported.

## Dimension Objects

### Dimension Types

LibreCAD documents common dimension types used for manufacturing and drafting. EDI should grow toward:

- Aligned dimension.
- Horizontal dimension.
- Vertical dimension.
- Linear/rotated dimension.
- Radial dimension.
- Diametric dimension.
- Angular dimension.
- Leader annotation.

The current dimension object work should remain command-routed and typed.

### Dimension Behavior

Dimension objects should be durable annotations derived from geometry:

- Start point.
- End point or target object.
- Offset.
- Label position.
- Displayed value.
- Unit and precision policy.
- Optional override text later.

The measured value should be recomputed from geometry unless the dimension is explicitly detached or overridden.

### Drafting Readability Rules

Dimensioning exists to remove ambiguity. EDI should eventually enforce or warn about common readability issues:

- Avoid crossing dimension lines.
- Avoid crossing extension lines.
- Keep dimension spacing consistent.
- Group related dimensions.
- Place dimensions outside the object by default.
- Use diameter for circles where appropriate.
- Use radius for arcs where appropriate.
- Warn about overlapping dimension labels.

First implementation can provide readable defaults and warnings before attempting automatic layout.

### Dimension Editing

Dimension objects need inspector and handle editing:

- Edit offset.
- Edit label position.
- Edit measured length when the dimension is allowed to drive geometry.
- Edit angle for angular/rotated dimensions.
- Choose radius versus diameter display.
- Lock dimension to prevent accidental movement.

Geometry-driving dimension edits must route through typed commands. Pure annotation edits should not mutate unrelated object geometry.

## Plotter Preview

### Preview Surface

LightBurn's preview model separates drawing moves from travel moves and lets users inspect the job before running it.

EDI should provide plot preview that shows:

- Drawing strokes.
- Non-drawing travel moves.
- Plotter bed border.
- Drawable bounds.
- Out-of-bounds warnings.
- Missing pen or layer mapping warnings.
- Stroke start and end markers.

This preview should derive from a plot job projection, not from widget paint state.

### Job Timeline

LightBurn-style preview offers a time slider and playback controls to scrub through cut order.

EDI should research:

- Step-through stroke order.
- Play/pause simulation.
- Time slider.
- Resume-from-stroke marker.
- Preview snapshot export later.

The first implementation can start with ordered stroke lists and distance statistics before adding animation.

### Plot Statistics

Useful preview stats include:

- Drawing distance.
- Travel distance.
- Estimated runtime.
- Object count.
- Stroke count.
- Pen/layer count.
- Warning count.

These metrics should be generated from the same plot job data used by the preview.

## Plot Planning And Optimization

### Ordering

Plot/cut planning tools often allow ordering by layer, group, or priority.

EDI should support:

- Current document order.
- Layer order.
- Explicit plot priority.
- Selected-only plot plan.
- Non-plotting object exclusion.

### Optimization

LightBurn-style cut optimization provides useful research targets:

- Inner shapes before outer shapes.
- Directional ordering, such as top-to-bottom or left-to-right.
- Travel minimization.
- Direction-change reduction.
- Backlash-aware ordering.
- Best starting point.
- Start at corners where possible.
- Best drawing direction.
- Remove overlapping lines.

EDI should treat these as optional plot-plan settings. The preview must reflect the chosen settings so users can inspect the result before output.

### Duplicate And Overlap Cleanup

Physical output can be damaged or slowed by duplicate strokes.

EDI should eventually detect:

- Duplicate segments.
- Opposite-direction duplicate segments.
- Near-overlapping segments within tolerance.
- Tiny segments below plotter precision.
- Excessive travel introduced by poor ordering.

The cleanup path should report what would change before modifying any stored geometry.

## Hatch And Fill

### Basic Hatch

Industrial engraving and plotting tools commonly fill closed geometry with generated lines.

EDI should support basic hatch settings:

- Angle.
- Spacing.
- Crosshatch toggle.
- Stroke width.
- Density preset.
- Closed-shape eligibility.

### Fill Variants

LightBurn-style hatch behavior suggests later variants:

- Unidirectional fill.
- Bidirectional fill.
- Offset fill.
- Flood fill.
- Multiple passes or sub-layers.
- Angle increment per pass.

These should be generator settings first. Expansion to raw line objects should be explicit.

### Hatch Safety

Hatch generation can create very large stroke counts.

EDI should report:

- Generated line count.
- Estimated drawing distance.
- Estimated travel distance.
- Density warning.
- Unsupported open-shape warning.
- Excessive runtime warning.

## Calibration And Machine Setup

### Calibration Workflow

Industrial cutter setup commonly includes cleaning the machine, setting tool depth or pressure, setting speed, aligning the head, and running a test cut. For pen plotting, EDI should adapt this into pen pressure, pen lift, travel speed, origin, scale, and repeatability checks.

EDI should support guided calibration:

- Select device/material/preset.
- Draw a known test pattern.
- Record measured output.
- Compute correction factors.
- Store calibration metadata.
- Apply correction during plot planning or output projection.

### Test Patterns

Useful calibration patterns include:

- Square size test.
- Circle diameter test.
- Parallel line spacing test.
- Hatch density test.
- Origin test.
- Pen width test.
- Repeatability test.
- Backlash test.

### Calibration Metadata

Calibration data should be explicit:

- Device id/name.
- Material.
- Tool/pen.
- Date.
- Intended measurement.
- Measured measurement.
- Correction factor.
- Notes.

Calibration should not silently rewrite source geometry. It should inform plotting projection and warnings.

## Phase 3-4 Implications For EDI

1. Implement quick measurement as transient readout before persistent dimension objects.
2. Add explicit measurement commands for distance, angle, length, area, and perimeter.
3. Keep dimension objects typed and command-routed.
4. Use drafting readability rules as defaults and warnings before automatic dimension layout.
5. Build plot preview from plot job data, not from widget paint state.
6. Differentiate drawing strokes from travel moves in preview and metrics.
7. Add plot planning settings as explicit options with inspectable output.
8. Treat hatch/fill as generated projection first, then allow explicit expansion later.
9. Require warnings for high-density hatch, duplicate strokes, missing pen mappings, and out-of-bounds output.
10. Store calibration as device/material metadata and apply it during plot projection, not by mutating source objects.

## Research Pressure On The Next Slices

This research points to the next useful implementation work:

- Quick measurement projection.
- Dimension object inspector parity.
- Plot preview statistics.
- Travel versus drawing segment projection.
- Plot bounds warning improvements.
- Duplicate segment detection.
- Basic hatch generator contract.
- Calibration test-pattern generator.
- Calibration metadata storage.
