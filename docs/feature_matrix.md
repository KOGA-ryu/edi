# EDI Feature Matrix

Status values:

- `core`: Required for the product foundation.
- `planned`: Important, but should follow the core contracts.
- `experimental`: Useful for discovery, but not durable until promoted.
- `deferred`: Intentionally later.

Use `docs/feature_research_map.md` for the larger research inventory. This matrix only tracks promoted feature families and near-term implementation shape.

## Drafting App

| Feature | Status | Required Subfeatures |
| --- | --- | --- |
| Canvas workspace | core | Drawing surface, board bounds, redraw lifecycle, empty state, status readout |
| Viewport | core | Zoom, pan, screen-to-canvas conversion, canvas-to-screen conversion, visible bounds |
| Drawing objects | core | Line, point, rectangle, circle, polygon, polyline |
| Object lifecycle | core | Create, select, move, edit handles, delete, duplicate |
| Geometry contracts | core | Bounds, transforms, hit testing, snapping, handles |
| Layers | planned | Layer IDs, visibility, ordering, locked state, editable state |
| Style system | planned | Stroke, fill, opacity, line width, line style |
| Metadata | planned | Author/source, created time, tool provenance, measurement info |
| Measurement tools | planned | Scale calibration, distance, area, dimensions, real-world unit conversion |
| Build-plan generation | planned | Annotated dimensions, material notes, ordered construction steps |

## Text Editor

| Feature | Status | Required Subfeatures |
| --- | --- | --- |
| Plain text document model | core | Document ID, title/name, text body, dirty state, active document |
| Multiple documents/tabs | planned | Open documents, close document, active tab, reorder, unsaved-state display |
| Save/load/export contract | core | Typed document storage boundary, save result, load result, export result, error messages |
| Role-based documents | planned | Scratch, prompt, context, reference, build note |
| Search and navigation | planned | Search, replace, next/previous match, line/section navigation |
| Undo/redo | core | Text edit history, command grouping, clear boundaries between document changes |
| Selection and clipboard | core | Select, copy, cut, paste, keyboard workflow |
| Structured side metadata | planned | Document role, source, tags, created/updated timestamps |
| Handoff/export bundles | planned | AI-facing packet, planning packet, selected document bundle |
| ASCII-aware editing modes | experimental | Fixed-width view, grid awareness, character-cell navigation |

## Automation / Scripting

| Feature | Status | Required Subfeatures |
| --- | --- | --- |
| C++ command ownership | core | Command validation, mutation boundary, undo-aware command routing |
| Lua recipe layer | planned | Authored recipes, command requests, no direct state ownership |
| Scriptable workflows | planned | Batch conversion, preset application, export recipes |
| Storage protection | core | No direct scripting access to raw object storage |
| Dry-run/introspection | planned | Preview command effects, report planned mutations, fail before mutation |
| Script settings surface | deferred | UI settings only after command and format contracts stabilize |

## Project / Workspace

| Feature | Status | Required Subfeatures |
| --- | --- | --- |
| Project settings | core | Project identity, app settings, format-backed configuration |
| Recent files | planned | In-memory list first, later persistent recent entries |
| Workspace layout | planned | Panels, splitter state, active workspace, restored view state |
| Tool presets | planned | Drafting tools, text tools, export presets |
| Asset libraries | planned | Local assets, project assets, selected asset set |
| Document collections | planned | Document group, active document, role filters |
| Import/export boundaries | core | Typed import result, typed export result, format-specific adapters |
| User preferences | planned | Theme, typography, keyboard behavior, default tools |

## Implementation Phases

1. Rebuild stable C++ contracts for drafting objects and text documents.
2. Add storage boundaries for the smallest project/workspace settings.
3. Add command-level tests around object edits, text edits, and project settings.
4. Add format adapters once the typed C++ contracts are stable.
5. Add automation recipes only after commands can be safely previewed and validated.
