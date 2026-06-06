# Drawing Tool Visual Contract

This contract defines the Draftsman drawing/drafting surface.

The generic shell keeps right and bottom panels project-defined. For the drawing tool, the project decision is now:

```text
main_workspace: drawing canvas only
left_panel: drawing tools
right_panel: selected tool types / variants
bottom_panel: drafting and art formatting controls
```

## Main Workspace

The main workspace is the drawing canvas. That is it.

Allowed:

- artboard
- grid/guides
- live preview
- selected object highlight
- direct canvas interaction
- minimal canvas-local status only when it does not cover the work

Not allowed:

- broad help text
- document panels
- tool descriptions
- generated logs
- settings forms
- duplicated tool lists
- inspector rows
- empty placeholder panels

The main workspace should feel like a drafting table surface, not a dashboard.

## Left Panel

The left panel owns drawing tool selection.

Allowed:

- compact drawing tool groups
- working primitives
- drawing helpers
- layer selection when needed for drawing
- asset/source selectors when they behave like drawing inputs

Default visible tool groups:

```text
Draw:
- select
- point
- line
- circle
- rectangle
- polygon

Reference:
- image frame
- ASCII crop
```

Hidden until proven:

```text
- spline / curve
- hatch / boundary
- offset / trim
- mirror / array
- measure / inspect
- layer review
- trace / markup
- ASCII cell region
- tone probe
- glyph baseline
- Blender/script execution tools
```

Not allowed:

- fake disabled tools
- long descriptions
- runner controls
- Blender operation chains
- ASCII generator execution controls
- formatting controls that belong in the bottom panel

The left panel answers: which drawing tool is active?

## Right Panel

The right panel owns selected-tool types and variants.

Examples:

- line type: straight, polyline, constrained, future curve-derived line
- circle type: circle, arc, ellipse when implemented
- rectangle type: rectangle, frame, crop, cell block
- polygon type: regular polygon, custom polygon when implemented
- image frame type: reference image frame, crop source frame
- ASCII crop type: crop frame, output frame, future character-cell frame

Allowed:

- selected tool variants
- tool-specific mode choices
- tool-specific behavior toggles
- tool-specific numeric parameters when they define the tool type
- short explanation only when the type is otherwise ambiguous

Not allowed:

- selected object formatting
- color/thickness/material controls
- global settings
- logs
- export receipts
- broad documentation
- duplicate left-panel tool list

The right panel answers: what kind of this selected drawing tool am I using?

If no selected tool has variants yet, keep the right panel closed or show nothing useful. Do not fill it with filler.

## Bottom Panel

The bottom panel owns drafting/art formatting controls.

This is intentionally similar to controls often found in a top toolbar in drawing apps, but Draftsman places them at the bottom.

Allowed:

- stroke color
- fill color
- line thickness
- line style
- opacity
- snap toggles
- grid/guideline toggles
- object alignment options
- text/glyph style when text/glyph drawing exists
- layer visibility shortcuts when formatting-related
- current selection formatting when an object is selected

Not allowed:

- logs by default
- validation tables by default
- export receipts by default
- tool selection
- selected tool type variants
- broad documentation
- fake controls for unimplemented formatting behavior

The bottom panel answers: how should this drawing be formatted?

If formatting controls are not implemented yet, keep the bottom panel closed. Do not show fake sliders or color pickers.

## Settings

Settings owns defaults and visibility, not active drawing work.

Allowed:

- default grid visibility
- default snap behavior
- default stroke/fill/thickness
- visible tool groups
- hidden tool opt-ins
- external image/ASCII tool paths
- write/export safety
- canvas size defaults

Not allowed:

- active canvas content
- current drawing object editing
- logs
- tool variant picking for the current tool

Settings answers: what should the drawing tool default to?

## ASCII And Blender Boundaries

ASCII crop and image frame are drawing helpers. They may appear in the drawing tool because they define regions on the canvas.

ASCII generator execution does not belong in the drawing toolbar. It belongs in a tool library/runner feature once that contract exists.

Blender scripts do not belong in the drawing toolbar. They belong in a script/tool library feature. A drawing object may later reference Blender output, but execution and script planning stay outside this drawing surface.

## Proof Rule

A drawing tool can be visible by default only after it passes this loop:

```text
select tool
create object
select object
inspect/edit if applicable
delete/reset without stale state
```

If the loop fails, hide the tool until it is fixed.

## Builder Checklist

Before changing the drawing UI, answer:

```text
Does the main workspace remain canvas-only?
Does the left panel only select drawing tools or drawing inputs?
Does the right panel only show selected-tool variants/types?
Does the bottom panel only show real formatting controls?
Are fake/future controls hidden?
Are ASCII/Blender execution controls kept out of the drawing toolbar?
```
