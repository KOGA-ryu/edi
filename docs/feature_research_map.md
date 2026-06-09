# EDI Feature Research Map

## Summary

This is a large feature inventory for EDI as a data-oriented creative drafting, drawing, plotter, ASCII, and planning tool. Each feature has subfeatures to research further before implementation. C++ remains the durable owner of geometry, commands, validation, state, and mutation.

For Phase 1 drafting-surface decisions, also use `docs/phase1_real_world_tool_research.md` as the concrete reference for grid, units, origin, bounds, and cursor readout.

For Phase 2 editing decisions, use `docs/phase2_editing_research.md` as the concrete reference for typed objects, handles, selection, snapping, inspector behavior, and transform tools.

## Drafting And Drawing Surface

### Physical Grid And Workspace

- Physical bed presets: custom, letter, A4, square board, plotter bed sizes, material sheet sizes.
- Unit system: normalized canvas, mm, cm, inch, foot, custom scale units.
- Origin controls: top-left, center, bottom-left, plotter-origin, user-defined origin.
- Margin system: page bounds, drawable bounds, safe area, material clamp area.
- Major/minor grid lines: configurable spacing, major interval, visibility, color, thickness.
- Grid density modes: coarse drafting, fine drafting, plotter precision, pixel/art grid.
- Drawable-area warnings: object outside page, object outside material, object outside plotter range.
- Coordinate readout: raw cursor, snapped cursor, physical unit value, selected object dimensions.
- Grid presets UI: saved presets, recent presets, project preset, per-document preset.
- Grid calibration metadata: intended dimensions, measured dimensions, correction factor.

### Viewport And Navigation

- Zoom controls: fit board, fit selection, fit drawable area, zoom to cursor.
- Pan controls: hand tool, middle mouse pan, keyboard pan, viewport reset.
- View modes: drafting view, plotter preview view, measurement view, ASCII cell view.
- View overlays: grid, bounds, snap markers, handles, dimensions, plot travel moves.
- Minimap or overview panel: visible board rectangle, object extent, selected extent.
- View bookmarks: named zoom/pan states for large drawings.
- High precision cursor mode: magnified pointer area, coordinate lock, snapping preview.
- Split view: design view beside plotter preview or measurement view.
- Presentation view: hide editor controls, show clean drawing surface.
- Debug view: object IDs, bounds boxes, handle IDs, snap candidates.

### Typed Drawing Objects

- Point object: coordinate, style, metadata, snap target.
- Line object: endpoints, length, angle, stroke style, plot order.
- Rectangle object: origin, width, height, rotation, corner handles.
- Circle object: center, radius, diameter, circumference, area.
- Polyline object: ordered vertices, open path, vertex handles.
- Polygon object: closed vertices, fill/hatch eligibility, area.
- Guide object: horizontal, vertical, angled, locked/reference only.
- Construction line object: non-plotting reference geometry.
- Dimension object: distance, width, height, radius, diameter, angle.
- Text label object: annotation text, position, rotation, style.
- Group object: selected objects treated as one transform target.
- Symbol/component object: reusable inserted drawing object set.
- Image reference object: imported raster used for tracing or ASCII conversion.

### Object Lifecycle

- Create object: tool intent creates typed object through command.
- Preview object: temporary shape before commit, never stored as document truth.
- Select object: click, marquee, additive selection, toggle selection.
- Move object: typed translation, validated before mutation.
- Edit object: handles, inspector fields, typed command path.
- Duplicate object: copy geometry, assign new ID, preserve or reset metadata.
- Delete object: command-routed, selection cleanup, locked-object rejection.
- Copy/paste object: internal clipboard, cross-document paste later.
- Clone object: linked or unlinked duplicate research path.
- Convert object: rectangle to polygon, circle to path, guide to construction line.
- Lock object: prevent edits, moves, deletes, and plot changes.
- Hide object: remove from canvas view and hit testing while preserving data.

### Precision Snapping

- Grid snap: minor grid, major grid, custom step.
- Endpoint snap: line ends, polyline vertices, dimension endpoints.
- Midpoint snap: line midpoint, rectangle side midpoint.
- Center snap: circle center, rectangle center, selection center.
- Vertex snap: polygon/polyline vertices.
- Guide snap: guide intersection, guide line, guide anchor.
- Object snap priority: object-before-grid, grid-before-object.
- Snap tolerance presets: tight, normal, loose, custom pixel tolerance.
- Snap labels: grid, endpoint, midpoint, center, guide, intersection.
- Snap candidate preview: show possible snap targets near cursor.
- Snap bypass modifier: temporary no-snap movement.
- Fine snap modifier: smaller step while dragging.
- Angle snap: 0, 15, 30, 45, 60, 90 degree constraints.
- Distance snap: repeat previous segment length, fixed distance.
- Intersection snap: line-line, guide-guide, object-guide intersections.

### Hit Testing And Selection

- Object body hit testing: point, line, rectangle, circle, polygon, guide, dimension.
- Handle hit testing: endpoint, radius, corner, rotation, dimension offset.
- Selection priority: handles above object body, selected objects above unselected, top layer wins.
- Marquee selection: inside-only, crossing selection, additive/toggle mode.
- Selection bounds: visual bounds, plot bounds, physical bounds.
- Multi-selection movement: all selected objects move as one command.
- Selection filtering: select only guides, plotting objects, locked objects, visible layer.
- Selection cycling: cycle through stacked objects under cursor.
- Hover targeting: preview target before click.
- Hit-test debug overlay: show winner, distance, tolerance, rejected candidates.

### Handles And Object Editing

- Point handle: move point.
- Line handles: move start, move end, move whole line.
- Rectangle handles: four corners, side handles later, rotation handle.
- Circle handles: center, radius, diameter handles.
- Polyline handles: vertex move, insert vertex, delete vertex.
- Polygon handles: vertex move, insert vertex, delete vertex, close/open behavior.
- Guide handles: move guide, rotate angled guide later.
- Dimension handles: start, end, offset, label position.
- Group handles: move, resize, rotate group.
- Handle cursor styles: move, resize, rotate, unavailable.
- Handle legality: visible editable handle must map to a valid command.
- Handle constraints: shift-constrain, snap during drag, numeric override.

### Numeric Inspector

- Point fields: x, y.
- Line fields: x1, y1, x2, y2, length, angle.
- Rectangle fields: x, y, width, height, rotation.
- Circle fields: cx, cy, radius, diameter.
- Guide fields: position, orientation, label, lock.
- Dimension fields: start, end, length, offset, angle.
- Physical-unit fields: display/edit in mm, cm, inch, foot.
- Normalized fields: internal canvas coordinate editing.
- Bounds readout: x, y, width, height.
- Measurement readout: length, area, perimeter, radius, diameter.
- Plot safety readout: inside/outside drawable area, plot enabled, pen mapping.
- Validation feedback: reject invalid edits and restore displayed value.
- Nudge controls: fine, grid, major-grid, physical-unit increments.
- Multi-selection inspector: shared fields, mixed-value display, batch update.

### Transform Tools

- Move tool: drag, numeric dx/dy, arrow-key nudge.
- Rotate tool: object rotation, selection rotation, pivot selection.
- Scale tool: uniform, non-uniform, from center, from handle.
- Mirror tool: horizontal, vertical, custom axis, guide-based mirror.
- Offset tool: parallel line/path at fixed distance.
- Array tool: repeat by count and spacing, x/y grid, radial array later.
- Align tools: left, center, right, top, middle, bottom.
- Distribute tools: horizontal spacing, vertical spacing, equal gaps.
- Fit tools: fit selected object to drawable bounds, fit to margin.
- Normalize tools: snap object bounds to grid, round coordinates, clean tiny values.
- Path cleanup: remove duplicate vertices, collapse near-zero segments.
- Join/split tools: join lines/polylines, split at point/intersection.

### Core Drawing Tools

- Point tool: place precise coordinate marker.
- Line tool: two-click line, constrained line, length/angle entry.
- Rectangle tool: two-corner rectangle, center-out rectangle, fixed size.
- Circle tool: center-radius, diameter, three-point circle later.
- Polyline tool: multi-click open path, finish/cancel behavior.
- Polygon tool: multi-click closed path, snap-to-close behavior.
- Guide tool: horizontal, vertical, angled, from object bounds.
- Construction line tool: non-plotting line/reference path.
- Dimension tool: distance, width, height, radius, diameter, angle.
- Text annotation tool: label, note, dimension label override.
- Trace tool: draw over image reference or ASCII source.
- Calibration tool: create test patterns and record measured output.
- Plot preview tool: inspect stroke order and travel movement.

### Layers And Object Organization

- Layer list: create, rename, delete, reorder.
- Layer flags: visible, locked, plot-enabled.
- Layer style defaults: stroke color, width, dash pattern, opacity.
- Layer pen mapping: map layer to plotter pen/color/tool.
- Active layer: new objects land on active layer.
- Move object to layer: single and batch operation.
- Layer groups: construction, drawing, annotation, plot, hidden reference.
- Layer filters: show only plotting objects, show only selected layer.
- Layer isolation: temporarily hide all other layers.
- Layer warnings: locked active layer, hidden selected object, unplottable layer.

### Style System

- Stroke color: UI picker, preset swatches, layer default.
- Stroke width: physical width and screen preview width.
- Line style: solid, dashed, dotted, construction.
- Fill style: none, solid, hatch later.
- Opacity: view-only opacity, plotter-safe opacity policy later.
- Style presets: drafting black, construction blue, dimension gray, plot pen colors.
- Style inheritance: object override vs layer default.
- Plot style: pen ID, speed, pressure, pass count later.
- Selection style: outline, handles, hover color.
- Warning style: out-of-bounds, locked, invalid, non-plotting.

### Measurement And Dimensions

- Distance measurement: between two points, between objects.
- Area measurement: rectangle, circle, polygon.
- Perimeter measurement: closed shapes.
- Radius/diameter measurement: circle and arc later.
- Angle measurement: between lines or points.
- Object dimension summary: bounds, real units, plot dimensions.
- Calibration measurement: expected vs measured output.
- Scale calibration: set drawing scale from known distance.
- Unit conversion: normalized to physical units, physical to normalized.
- Measurement labels: static annotation or live-linked dimension.
- Measurement precision: decimal places, rounding mode, tolerance.
- Measurement reports: selected object summary, full drawing summary.

## Plotter And Physical Output

### Plotter Preview

- Plot bounds overlay: expected plotted area.
- Drawable bounds check: block or warn before export.
- Travel preview: show non-drawing moves.
- Stroke preview: show drawing moves and order.
- Start/end marker: first stroke, final position.
- Pen/layer preview: colors mapped to pens.
- Multi-pass preview: hatch/fill passes, repeated strokes.
- Plot statistics: total drawing distance, travel distance, object count.
- Plot risk warnings: outside bounds, unsupported object, missing pen mapping.
- Preview filters: show travel only, strokes only, warnings only.

### Plot Job Planning

- Plot job object list: ordered drawable objects.
- Stroke ordering: current order, layer order, optimized order later.
- Travel minimization: nearest-neighbor baseline, advanced optimizer later.
- Pen change planning: group by pen/layer.
- Object flattening: convert drawing objects into plot segments.
- Non-plotting exclusion: guides, construction lines, hidden layers.
- Bounds validation: raw bounds and calibrated bounds.
- Job report: warnings, stats, pen usage, object coverage.
- Dry-run mode: report without writing output.
- Plot reproducibility: stable ordering and deterministic output.

### Plotter Calibration

- Square test pattern: expected size, measured size.
- Circle test pattern: diameter/circularity check.
- Line spacing pattern: parallel lines and hatch density.
- Axis scale correction: x scale, y scale, combined scale.
- Origin test: verify machine origin against drawing origin.
- Pen width test: stroke thickness and overlap.
- Repeatability test: draw same path multiple times.
- Backlash test: reverse-direction line accuracy.
- Calibration history: recorded results, date, device, material.
- Correction plan: measured error to scale factor.

### Pen And Material Management

- Pen library: pen ID, color, width, type.
- Material library: paper, cardboard, vinyl, wood, fabric.
- Material bounds: sheet size, safe area, clamp area.
- Speed presets: draft, normal, slow/detail.
- Pressure/depth placeholders: future plotter/cutter devices.
- Multi-tool mapping: pen, marker, cutter, engraver later.
- Layer-to-pen mapping: per layer output intent.
- Style-to-pen mapping: per object output intent.
- Missing mapping warnings: unassigned pen or material.
- Material usage estimate: bounding area, approximate coverage.

### Hatch And Fill

- Basic hatch: angle, spacing, stroke width.
- Crosshatch: two or more hatch angles.
- Contour fill: inward offsets for closed shapes.
- Stipple fill: point distribution later.
- Density presets: light, medium, heavy.
- Hatch clipping: clip generated lines to closed shape.
- Hatch preview: generated strokes visible before plot.
- Hatch as object: editable generator settings.
- Hatch expansion: convert generated fill to raw lines.
- Plot safety: line count, density warning, excessive travel warning.

## ASCII Art System

### ASCII Canvas

- Character-cell grid: rows, columns, cell width, cell height.
- Monospace rendering: font choice, cell alignment, baseline.
- Cell coordinate system: row/column, normalized mapping, physical mapping.
- ASCII layers: text layer, color layer, metadata layer.
- Cell selection: single cell, range, rectangle, lasso later.
- Cursor modes: insert, overwrite, brush, select.
- Grid snapping: object placement aligned to character cells.
- Zoom modes: fit text, fit width, pixel-perfect cell view.
- Text export: plain text, ANSI text, structured document.
- Image export: raster preview of ASCII output.

### Image-To-ASCII Conversion

- Image import: raster source, crop, resize, rotate.
- Brightness mapping: luminance to character ramp.
- Color sampling: per cell average, dominant color, edge color.
- Palette extraction: fixed palette, adaptive palette, limited palette.
- Dithering: threshold, ordered, error diffusion later.
- Edge-aware conversion: preserve outlines and contrast.
- Character ramp editor: choose character set for brightness.
- Resolution controls: rows/columns, cell aspect correction.
- Preview comparison: source image beside ASCII output.
- Conversion presets: portrait, blueprint, high contrast, color poster.

### ASCII + Drafting Bridge

- Convert drawing geometry to ASCII grid.
- Convert ASCII cells to drafting objects.
- Place drafting objects aligned to character cells.
- Use drafting measurements to define ASCII cell scale.
- Overlay ASCII art on physical drafting grid.
- Use image-to-ASCII output as tracing reference.
- Export mixed drawing + ASCII document.
- Preserve metadata across drafting and ASCII views.

## Text Editor And Planning Workspace

### Text Document Model

- Plain text documents: scratch, notes, prompts, references.
- Multiple documents: tabs, sidebar list, active document.
- Save/load/export contract: durable document state.
- Dirty/revision tracking: unsaved changes, edit history.
- Document roles: scratch, prompt, context, build note, reference.
- Side metadata: title, role, tags, source, created time.
- Search/replace: current document and project-wide.
- Navigation: line/column, symbol anchors later.
- Selection/clipboard: keyboard workflow, multi-line selection.
- Undo/redo: text command history.
- Fixed-width mode: ASCII-aware editing and alignment.

### Planning Documents

- Project brief: goal, constraints, materials, outputs.
- Build notes: steps, measurements, dependencies.
- Research notes: links, observations, decisions.
- Prompt packets: AI-facing context bundles.
- Review packets: code/design review summaries.
- Measurement logs: calibration, test results, plot output notes.
- Change logs: project decisions over time.
- Export notes: output settings and production instructions.
- Template documents: reusable planning forms.
- Document-to-drawing links: note references object IDs or layers.

### AI Handoff And Context

- TOON planning packet: compact context export.
- Review packet: selected docs, drawing summary, warnings.
- Agent handoff bundle: current task, state, constraints.
- Drawing summary export: object count, layers, measurements.
- Text summary export: document roles and selected content.
- Context size controls: include/exclude large data.
- Redaction controls: omit private notes or assets.
- Round-trip notes: imported AI suggestions become text docs.
- Audit trail: generated summary date/source.
- Non-truth rule: handoff exports never own app state.

## Project And Workspace

### Project State

- Project identity: name, root, created time, active document.
- Workspace layout: panels, toolbars, inspector state.
- Recent files: drawings, text docs, assets.
- Project settings: units, default grid, default format policy.
- Asset libraries: images, presets, materials.
- Document collections: drafting docs, text docs, reference assets.
- Autosave plan: dirty state, recovery files later.
- Project validation: missing assets, unsupported formats, stale references.
- Project summary: object count, docs count, warnings.
- Project templates: plotter project, ASCII art project, blueprint project.

### Presets And Libraries

- Grid presets: physical bed and page sizes.
- Snap presets: drafting, loose sketching, plotter precision.
- Style presets: stroke, fill, pen styles.
- Tool presets: line defaults, dimension defaults, hatch presets.
- Material presets: sheet sizes and safe margins.
- Plotter presets: device dimensions, pen mapping, calibration.
- Export presets: image, text, plot job, handoff packet.
- Workspace presets: panel arrangement and default visible tools.
- Validation presets: strict plotter mode, relaxed design mode.

### Settings UI

- General settings: theme, units, default project folder.
- Drafting settings: grid, snap, handles, inspector precision.
- Plotter settings: bed, material, pens, calibration.
- ASCII settings: font, cell size, character ramps.
- Scripting settings: enable/disable recipes later.
- Format settings: inspect tools, export defaults.
- Diagnostics settings: debug overlays, performance counters.
- Keyboard shortcuts: tool selection, nudge, snap modifiers.
- Reset/import/export settings: share presets across projects.

## Automation And Scripting

### Command Engine

- C++ command engine owns all mutation.
- Commands are declarative and replayable where possible.
- Command categories: create, edit, move, select, layer, style, export.
- Dry-run command plans: validate before mutation.
- Command reports: accepted, rejected, warnings, changed objects.
- Undo/redo integration: command history later.
- Batch command execution: all-or-nothing option.
- Script command bridge: recipes request commands, C++ validates.
- Command provenance: tool ID, script ID, user action.
- Command tests: invalid command does not mutate state.

### Lua Recipes Later

- Procedural drawing recipes.
- Batch conversion recipes.
- Export recipes.
- Build-plan generation recipes.
- Plotter preparation recipes.
- Dry-run inspection before mutation.
- No raw storage access.
- No app-state ownership.
- Recipe metadata: name, version, required capabilities.

### Tool Automation

- Batch object creation from table-like inputs.
- Apply preset to selection.
- Generate calibration pattern.
- Generate hatch/fill for closed shapes.
- Convert image to ASCII.
- Convert ASCII to plotter strokes.
- Export selected layers.
- Validate plotter safety.
- Generate project summary packet.

## Formats And Storage Research

### TOML

- Human-authored app settings.
- Theme settings.
- Grid presets.
- Tool presets.
- Workspace layout.
- Project profile.
- Export presets when declarative.
- Material and pen libraries if static.
- Validation: stable fields, clear errors, no runtime mutation ownership.

### MessagePack

- Drafting document state.
- Canvas object snapshots.
- Undo/replay fixtures.
- Compact plot job records.
- Binary golden fixtures.
- Calibration history if machine-owned.
- Inspector/unpack tooling required.
- Version/schema header required.
- No opaque binary without readable inspection.

### TOON

- AI handoff packets.
- Planning summaries.
- Review packets.
- Agent context bundles.
- Compact document exports.
- Drawing summaries for discussion.
- Build-plan drafts.
- Non-truth export only.
- Context-size budgeting.
- Redaction/export filters.

### Lua

- Authored behavior recipes.
- Procedural composition.
- Batch tool chains.
- Export/build-plan recipes.
- Dry-run command planning.
- User script library later.
- Script settings page later.
- C++ command bridge only.
- No raw object storage mutation.
- Sandboxing and capability control research.

## Diagnostics, Metrics, And Testing

### Contract Tests

- Geometry validation.
- Bounds recomputation.
- Numeric edits.
- Handle edit plans.
- Snap resolution.
- Hit testing.
- Selection movement.
- Tool creation.
- Grid projection.
- Plot job generation.

### Widget And Interaction Tests

- Deterministic control runner.
- Tool click scripts.
- Preview lifecycle tests.
- Selection tests.
- Handle drag tests.
- Snap marker tests.
- Inspector edit tests.
- No random manual clicking as primary proof.
- Screenshot tests only after contracts stabilize.
- Startup smoke test.

### Metrics And Performance

- Pointer move event count.
- Snap candidates evaluated.
- Hit-test candidate count.
- Paint duration.
- Object count by kind.
- Plot segment count.
- Travel distance vs drawing distance.
- Command acceptance/rejection counts.
- Invalid edit rejection reasons.
- Test summary output with only useful aggregates.

### Debug Overlays

- Object bounds.
- Plot bounds.
- Drawable bounds.
- Hit-test target.
- Snap candidates.
- Handle IDs.
- Layer order.
- Object IDs.
- Warning markers.
- Performance counters.

## Suggested Research Order

1. Physical grid, units, snap behavior, and cursor readout.
2. Typed object editing, handles, inspector, and command routing.
3. Selection, transforms, align/distribute, offset, mirror, array.
4. Dimension and measurement tools.
5. Plotter preview, calibration, pen/material mapping.
6. ASCII cell grid and image-to-ASCII conversion.
7. Project/workspace settings and reusable presets.
8. Durable formats: TOML, MessagePack, TOON.
9. Lua recipes and scripting UI after command contracts are stable.

## Assumptions

- First priority is a serious physical drafting surface, not decorative drawing.
- Plotter-readiness matters: grid, bounds, calibration, and stroke planning should shape feature decisions.
- ASCII systems are part of the long-term product, but they should reuse the same command/data discipline as drafting.
- UI exposes controls, but C++ owns truth, validation, commands, and mutation.
- This is a research map, not an implementation commitment order.
