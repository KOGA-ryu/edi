# Realizer invocation contract — for the one-command chain (edi-ui)

The realizer is a STANDALONE bpy program; `edi --generate-crypt` writes only the
TOON. To complete `one command → PNG`, the chain invokes the realizer as a
subprocess (the existing `BlenderRunPlan` seam fits this exactly). This is the
exact, verified call.

## The exact invocation
```
<blender> --background --python <repo>/tools/blender/edi_realize.py -- \
    <map.toon> --render=<out.png> [--scale=S] [--reference] [--samples=N]
```
- `<blender>`: the Blender 4.5.9 binary. On this box: `~/.local/bin/blender`
  (or `blender` if on PATH).
- `<repo>`: the repo root. The script path is `tools/blender/edi_realize.py`.
- **Everything after `--` is passed to the script** (Blender argv convention) —
  the chain MUST keep the `--` separator.

## Arguments (after the `--`)
| arg | required | meaning |
| --- | --- | --- |
| `<map.toon>` | yes | the Seam-B TOON map (positional; the only bare arg) |
| `--render=<out.png>` | **yes** | where the 1080p PNG is written |
| `--scale=S` | no | scale knob; OVERRIDES the TOON `scale:` header. Omit ⇒ use the header (missing header ⇒ 1) |
| `--reference` | no | **the figure+grid scale overlay** (fixed 6 ft red human + 5 ft floor checker). A bare flag, presence = ON. Omit ⇒ off (art-clean) |
| `--samples=N` | no | Cycles samples (default 64) |

## --reference is WIRED (your question)
YES — `--reference` is a realizer arg, parsed in BOTH entry points
(`run_in_blender` and the pure `main`). So the one command can pass it straight
through to turn the overlay on for the greybox demo, off for clean renders. No
further work needed on my side; just include `--reference` in the argv when the
user wants the scale-reference overlay.

## Contract details the chain can rely on
- **Exit code:** `0` success · `2` usage / missing `<toon>` or `--render` ·
  `3` no OptiX/CUDA GPU (the realizer REFUSES a silent CPU fallback).
- **Scale source:** prefer letting the TOON carry `scale:` (the locked demo files
  do: base⇒1, doubled⇒2, doubled2⇒4) and omit `--scale`; or pass `--scale=S` to
  override. Either reproduces the dial.
- **stdout** is the render log; the chain can assert success on
  `GPU CONFIRMED:` and `render complete: <png>` lines.
- **Timing/VRAM:** ~3.5–3.9 s, ~1.5 GB on the RTX 5090 (well inside the 2 min /
  32 GB gate).
- Verified end-to-end 2026-06-18 (exit 0, valid 1920×1080 PNG, GPU confirmed).

## Integration dependency
The chain finds the realizer at `<repo>/tools/blender/edi_realize.py`. That file
lives on `dept/blender-lab` (@f58119f) and is NOT yet on master. **FF
dept/blender-lab into master** (edi-ui's lane) so `~/edi/tools/blender/
edi_realize.py` exists for the chain. Green there: smoke + scan.

## BlenderRunPlan note
`planBlenderRender(executablePath, scriptPath, outputImagePath)` already builds
`--background --python <script> -- <outputImagePath>`. To use it as-is, the extra
realizer args (`<toon>`, `--scale`, `--reference`, `--samples`) need to ride in
the args vector too — i.e. the plan's argv must include the TOON path + flags, not
just the output path. If `BlenderRunPlan` only threads one post-`--` arg today,
that's the small extension the chain needs (an args list, not a single path).
</content>
