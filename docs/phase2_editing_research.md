# EDI Phase 2 Editing Research

This document records user-supplied research patterns from mature CAD and vector drawing tools, including AutoCAD, LibreCAD, QCAD, and Inkscape. The intent is to guide EDI Phase 2 decisions around typed objects, handles, selection, snapping, numeric inspector behavior, and transform tools.

This is planning input. It does not claim that the listed behaviors are already implemented in EDI.

## Phase 2 Focus

Phase 2 builds the editing environment on top of the physical drafting bed:

- Typed drawing objects with explicit geometry.
- Handles that map to legal edit commands.
- Selection workflows for single and multi-object editing.
- Precision snapping beyond the grid.
- Numeric inspector fields for parametric editing.
- Transform tools such as move, rotate, scale, mirror, align, and distribute.

The key product direction is that direct manipulation and numeric editing should both route through the same C++ command and validation discipline.

## Typed Objects And Handles

### Direct Manipulation

Mature CAD tools expose editable handles on selected geometry. AutoCAD calls these grips. In vector tools such as Inkscape, selected objects expose bounding-box handles for scaling, rotation, and skewing, while path tools expose node handles for editing vertices and curves.

EDI should treat handles as typed edit intent, not decoration:

- A visible editable handle must map to a legal command.
- Unsupported shapes should not expose editable handles until their commands exist.
- Handle dragging should produce an edit plan that C++ validates before mutation.
- Rendering and hit testing should consume the same handle contract.

### Shape-Specific Handles

EDI should define handle contracts per object kind:

- Point: position handle.
- Line: start endpoint, end endpoint, and whole-line movement through selection.
- Rectangle: corner handles, side handles later, rotation handle.
- Circle: center handle, radius/diameter handle.
- Polyline: vertex handles, insert vertex, delete vertex later.
- Polygon: vertex handles, insert/delete vertex, close/open rules later.
- Dimension: start, end, offset, and label-position handles.
- Group: combined transform handles after group objects exist.

### Advanced Handle Research

The mature-tool pattern points toward later features:

- Multi-handle editing.
- Vertex insertion and deletion from handles.
- Handle context menus.
- Constrained handle movement.
- Snapping during handle edits.
- Reference-angle edits using another object.

These should be deferred until the basic handle-to-command bridge is reliable.

## Selection Techniques

### Single And Multi-Selection

Common workflows include click selection, Shift-click additive selection, rectangle marquee selection, and selection toggling. Inkscape-style multi-selection uses one combined bounding box and lets the user transform the selected set as a unit.

EDI should support:

- Click to select one object.
- Shift-click to add or toggle.
- Marquee selection.
- Combined selection bounds.
- Multi-object movement through a single command.
- Selection cleanup when objects are deleted or hidden.

### Selection Modes

Research suggests useful selection modes:

- Enclosed selection: only objects fully inside the marquee.
- Crossing selection: objects touched by the marquee.
- Lasso or touch selection later.
- Selection cycling for stacked objects.
- Filters for object kind, layer, locked state, plot-enabled state, or selected entity type.

### Multi-Selection Inspector

Tools such as AutoCAD and QCAD show common editable properties for mixed selections and display a "varies" state when values differ.

EDI should plan for:

- Common-property display.
- Mixed-value display.
- Object-kind filtering.
- Batch edits routed through typed commands.
- Rejection if any selected object cannot legally accept the edit.

## Precision Snapping

### Snap Modes

CAD tools converge on a broad snap set. EDI should grow toward:

- Grid snap.
- Endpoint snap.
- Midpoint snap.
- Center snap.
- Vertex snap.
- Intersection snap.
- Nearest-on-object snap.
- Distance-from-endpoint snap.
- Guide snap.
- Relative-zero snap.

The first priority remains endpoint, midpoint, center, vertex, object priority, and tolerance controls.

### Snap Feedback

Mature tools use marker shapes, labels, status-bar feedback, and temporary tracking guides to make snap behavior visible.

EDI should expose:

- Snap source label.
- Snap kind.
- Source object id where relevant.
- Visible snap marker.
- Candidate preview later.
- Rejected candidate debug overlay later.

### Running Versus Temporary Snaps

AutoCAD-style running snaps stay enabled until toggled, while temporary snap overrides apply to one operation. LibreCAD and QCAD also expose restrictions such as horizontal, vertical, orthogonal, and relative-zero constraints.

EDI should distinguish:

- Persistent snap settings.
- Temporary modifier behavior.
- Snap restrictions.
- Angle constraints.
- Relative origin constraints.

## Numeric Inspector And Property Editing

### Basic Inspector Fields

Numeric editing should expose shape-specific geometry:

- Point: x, y.
- Line: x1, y1, x2, y2, length, angle.
- Rectangle: x, y, width, height, rotation.
- Circle: cx, cy, radius, diameter.
- Dimension: start, end, length, angle, offset.
- Guide: position, orientation, lock state.

The inspector should edit physical-unit values while the drafting core validates normalized geometry.

### Unit Controls

Tools such as Inkscape expose unit selection near numeric fields, while CAD tools separate display units from object truth.

EDI should preserve:

- Physical display units.
- Internal normalized geometry.
- Explicit conversion commands when geometry should be scaled.
- Precision and rounding controls that affect display, not stored geometry.

### Keyboard Workflow

Numeric editors are faster when fields are keyboard-friendly.

EDI should plan for:

- Tab and Shift+Tab between fields.
- Enter to accept.
- Esc to cancel.
- Arrow-key nudging.
- Larger increments through Page Up/Page Down or command shortcuts.
- Undo after accepted edits.

### Computed Fields

Line length, line angle, circle diameter, and dimension length are computed from geometry but still editable. Editing a computed field should produce a typed edit plan, not raw field mutation.

EDI already moved physical inspector edits in this direction. The next inspector work should keep extending that contract rather than placing conversion logic in UI/controller code.

## Transform Tools

### Core Transforms

Mature editors expose movement, rotation, scaling, mirror, alignment, and distribution through handles, commands, and numeric fields.

EDI should support:

- Move.
- Rotate.
- Scale.
- Mirror.
- Offset.
- Align.
- Distribute.
- Array/repeat.
- Fit to board or drawable bounds.

### Transform Constraints

Common constraints include horizontal/vertical movement, proportional scaling, fixed-angle rotation, and custom pivots.

EDI should plan for:

- Shift-constrained movement.
- Angle snap during rotation.
- Scale from center.
- Custom pivot for rotate/scale.
- Guide-based mirror axis.
- Numeric transform entry.

### Groups

Group objects let multiple objects behave as one transform target while preserving the ability to enter/edit the group later.

EDI should defer group implementation until:

- Selection bounds are stable.
- Multi-object move is command-routed.
- Transform commands can validate all affected objects.
- Storage can represent group membership without losing stable object ids.

## Phase 2 Implications For EDI

1. Keep typed object editing ahead of new drawing tools.
2. Treat handles as command-producing edit intents.
3. Make rendering, hit testing, and dragging consume the same handle contract.
4. Add multi-selection behavior before advanced transforms.
5. Expand snap modes carefully and keep snap feedback visible.
6. Keep the numeric inspector command-routed and unit-aware.
7. Support mixed-value inspector states before broad batch editing.
8. Add transform tools in this order: move, rotate, mirror, align/distribute, scale, array.
9. Defer group objects until selection, command validation, and transform plans are stable.
10. Use contract tests before widget-level interaction tests for each edit path.

## Research Pressure On The Next Slices

This research points to the next useful implementation work:

- Finish physical numeric inspector parity.
- Expand handle edit plans for all currently supported typed objects.
- Add multi-selection movement and combined selection bounds.
- Add selection filters or object-kind filtering.
- Add intersection and nearest-object snapping after current snap controls stabilize.
- Add visible snap candidate debugging.
- Build rotate and mirror as command-routed transforms.
- Build align/distribute on top of multi-selection bounds.
