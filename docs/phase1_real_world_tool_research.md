# EDI Phase 1 Real-World Tool Research

This document records user-supplied research patterns from AutoCAD, LibreCAD, MicroStation, Abaqus/CAE, and LightBurn. The intent is to guide EDI Phase 1 decisions around the physical canvas, grid and units, origin control, bounds and margins, cursor readout, and navigation.

This is planning input. It does not claim that the listed behaviors are already implemented in EDI.

## Grid And Workspace

### Grid Display And Snap

Mature drafting tools commonly treat the grid as a non-printing overlay. AutoCAD presents the grid like drafting paper under the drawing: useful for alignment, but not part of plotted output. Snap is independently toggleable, so users can display the grid without forcing the cursor onto grid increments.

EDI should preserve that separation:

- Grid display is a visual aid.
- Snap is an input constraint.
- Plot/export output should not include the grid unless an explicit output mode asks for it.

### Grid Spacing And Limits

AutoCAD exposes grid spacing, angle, and alignment. It can limit the grid to a rectangular area rather than showing it across the entire plane. Abaqus/CAE lets users set the sketch sheet size and grid spacing, with automatic spacing available when desired.

EDI should support:

- Fixed grid spacing.
- Adaptive grid spacing for zoomed-out readability.
- Grid limits tied to page, bed, material, or drawable bounds.
- Separate display density from coordinate precision.

### Major And Minor Lines

AutoCAD uses darker major grid lines at configurable intervals. Abaqus/CAE allows a configurable number of minor intervals between major grid lines, with minor lines appearing only when magnification makes them useful. LibreCAD defaults to a 10 mm dot grid with a major line every 100 mm and changes grid scale while zooming.

EDI should support:

- Minor spacing.
- Major-line interval.
- Dynamic minor-line visibility based on zoom.
- Presets such as coarse drafting, fine drafting, and plotter precision.

### Adaptive Grid

AutoCAD and LibreCAD both show the value of adaptive grid density: fewer grid lines when zoomed out, more detail when zoomed in. This reduces clutter without reducing internal coordinate precision.

EDI should treat adaptive grid density as view projection. It should not mutate object geometry, snap precision, or stored document state.

### Grid Rotation And Alignment

AutoCAD can rotate the grid and snap angle through the user coordinate system. The grid and snap origin move with the coordinate system origin.

EDI should eventually support:

- Grid angle.
- User coordinate orientation.
- Grid origin changes.
- Coordinate readout updated from the active coordinate system.

### Sheet And Bed Size

Abaqus/CAE uses a bounded sketch sheet with a user-defined size. CAD and plotting tools also expose paper, sheet, or bed size settings.

EDI should model the drafting surface as a physical bed/page with:

- Standard presets such as letter, A4, square board, and common plotter beds.
- Custom width and height.
- Safe and drawable bounds.
- Margins or clamp areas.

## Grid Presets And Calibration

### Automatic Versus Fixed Spacing

QCAD-style auto grid spacing is useful while exploring a drawing. Fixed spacing is necessary for precision drafting and physical output.

EDI should offer both:

- Automatic grid display density for comfortable navigation.
- Fixed grid and snap increments for drafting precision.
- Project presets that record the intended bed, unit, grid, and snap behavior.

### Calibration

Traditional CAD tools usually assume units are abstract and exact. Plotter and laser workflows need physical calibration because actual output can drift from intended dimensions.

EDI should record calibration as explicit metadata:

- Intended physical distance.
- Measured physical distance.
- Correction factor.
- Device and material context.

Calibration should inform plotter preview and output planning, not silently rewrite existing geometry.

## Units And Measurement

### Unit Choice

MicroStation separates master units from subunits and distinguishes displayed units from geometric size. Changing display unit labels does not inherently scale geometry.

EDI should follow the same discipline:

- Internal geometry remains normalized or otherwise unitless at the core boundary.
- Display units can be mm, cm, inch, foot, or a project-defined scale.
- Unit changes should not mutate geometry unless the user explicitly runs a scale/conversion command.

### Format And Accuracy

MicroStation's format and accuracy controls affect coordinate and distance display, not calculation accuracy.

EDI should separate:

- Stored numeric value.
- Display precision.
- Rounding mode.
- Inspector formatting.

### Coordinate Readout

LibreCAD shows live X/Y coordinates relative to an origin, including positive and negative directions across quadrants. CAD tools commonly provide persistent status bar coordinates.

EDI should provide:

- Raw cursor coordinate.
- Snapped cursor coordinate.
- Physical-unit coordinate.
- Absolute and relative coordinate modes.
- Selected object dimensions in the inspector.

### Measurement Precision

Grid, snap, and measurement precision are different controls. A user may want a coarse visible grid, fine snap, and high-precision measurements.

EDI should keep these independent:

- Grid display interval.
- Snap interval/tolerance.
- Measurement precision.
- Inspector display precision.

## Origin Controls And Physical Bed

### Machine Origin Versus User Origin

LightBurn ties the workspace grid to the physical machine bed. Absolute Coordinates mode maps artwork location directly to the machine coordinate system. Other modes allow output relative to the current head position or a user-defined origin.

EDI should support origin concepts useful for plotters:

- Machine origin.
- User origin.
- Current-position style origin for job preview.
- Origin presets such as bottom-left, top-left, center, and hardware-specific origins.

### Shifting The Origin

AutoCAD ties grid and snap to the UCS origin. MicroStation can change the global origin without changing geometry.

EDI should expose origin control as coordinate-system state:

- Moving the origin changes readout and projection.
- Moving the origin should not rewrite object geometry unless an explicit transform command is requested.
- Grid display and snap projection should follow the active origin.

### Physical Bounds And Margins

Plotter workflows need stronger bounds than a normal infinite CAD plane. The drawing may need to stay inside bed bounds, material bounds, drawable bounds, and clamp-safe margins.

EDI should distinguish:

- Page or bed bounds.
- Material bounds.
- Drawable bounds.
- Safe area or clamp margins.
- Out-of-bounds warnings.

## Cursor Readout And Navigation

### Navigation

LibreCAD-style mouse navigation is a useful baseline: wheel zoom, mouse drag pan, and dynamic grid scaling while the view changes.

EDI should support:

- Pan.
- Zoom to cursor.
- Fit board.
- Fit selection.
- Reset view.
- Grid density updates based on zoom.

### Persistent Feedback

High-precision work benefits from constant feedback. EDI should make cursor and snap state visible without requiring users to infer it from object placement.

EDI should show:

- Raw cursor position.
- Snapped cursor position.
- Snap source label.
- Current unit value.
- Inside/outside drawable status.
- Optional relative-distance readout during drawing or dragging.

## Phase 1 Implications For EDI

1. Build a configurable physical bed first: presets, custom dimensions, margins, safe area, and origin presets.
2. Keep the grid non-printing and independent from snap.
3. Separate adaptive display density from actual coordinate precision.
4. Support major/minor grid lines and fixed/adaptive grid modes.
5. Treat unit selection as display/project context unless an explicit conversion command is used.
6. Provide live raw, snapped, and physical cursor readouts.
7. Model machine origin, user origin, and current-position style plot alignment as separate concepts.
8. Warn when objects exceed page, material, drawable, or plotter-safe bounds.
9. Keep calibration as explicit device/material metadata, not hidden geometry mutation.
10. Make navigation and fit commands part of the Phase 1 drafting surface, not later polish.

## Research Pressure On The Next Slices

This research points to the next useful implementation work:

- Physical numeric inspector parity.
- Origin and coordinate-system controls.
- Grid preset persistence.
- Adaptive grid projection.
- Bounds and margin warnings.
- Calibration metadata and test-pattern planning.
- Plotter preview inputs that reuse the same bed, origin, unit, and bounds contracts.
