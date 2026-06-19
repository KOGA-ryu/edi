#!/usr/bin/env bash
# M0 ONE-COMMAND chain: crypt MapSpec -> Seam-B TOON -> greybox -> Cycles OptiX
# PNG on the GPU. This is the orchestration TIER: it runs two decoupled
# subprocess tiers and couples neither to the other (three-tier law) —
#   tier 1  edi (C++/Qt)   --generate-crypt -> TOON   [headless, offscreen]
#   tier 2  realizer (bpy) TOON -> render -> PNG       [Blender, OptiX GPU]
# edi never imports Blender; the realizer never imports edi. The script is the
# only place they meet.
#
# Usage:
#   tools/m0/render-crypt.sh --out <png> [--scale S] [--reference] [--samples N] [--toon <path>]
#
# Flags:
#   --out <png>     (required) the rendered PNG path
#   --scale S       uniform crypt scale (default 1); forwarded to --generate-crypt
#   --reference     add the realizer's scale-reference overlay (fixed 6 ft figure +
#                   floor checker); forwarded to the realizer
#   --samples N     Cycles samples (default 64); forwarded to the realizer
#   --toon <path>   where to write the intermediate TOON (default: alongside --out)
#
# Env:
#   EDI       path to the edi binary   (default: <repo>/build/edi)
#   BLENDER   blender executable       (default: blender on PATH)
set -euo pipefail

scale=1
out=""
toon=""
samples=64
reference=""

while [ "$#" -gt 0 ]; do
  case "$1" in
    --out)       out="$2"; shift 2 ;;
    --scale)     scale="$2"; shift 2 ;;
    --samples)   samples="$2"; shift 2 ;;
    --toon)      toon="$2"; shift 2 ;;
    --reference) reference="--reference"; shift ;;
    -h|--help)   sed -n '2,30p' "$0"; exit 0 ;;
    *) echo "render-crypt: unknown arg '$1'" >&2; exit 2 ;;
  esac
done

[ -n "$out" ] || { echo "render-crypt: --out <png> is required" >&2; exit 2; }

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo="$(cd "$script_dir/../.." && pwd)"
EDI="${EDI:-$repo/build/edi}"
BLENDER="${BLENDER:-blender}"
realizer="$repo/tools/blender/edi_realize.py"

[ -x "$EDI" ] || { echo "render-crypt: edi binary not found/executable: $EDI (build it, or set EDI=)" >&2; exit 1; }
command -v "$BLENDER" >/dev/null 2>&1 || { echo "render-crypt: blender not found: $BLENDER (set BLENDER=)" >&2; exit 1; }
[ -f "$realizer" ] || { echo "render-crypt: realizer not found: $realizer" >&2; exit 1; }

[ -n "$toon" ] || toon="${out%.png}.toon"
mkdir -p "$(dirname "$out")" "$(dirname "$toon")"

echo "render-crypt: [1/2] generating crypt TOON (scale $scale) -> $toon"
# Qt needs an offscreen platform when running headless (no display).
QT_QPA_PLATFORM=offscreen "$EDI" --generate-crypt "$toon" --scale "$scale"

echo "render-crypt: [2/2] realizing $toon -> $out (scale $scale, samples $samples${reference:+, reference})"
# Thread ALL post-`--` args to the realizer per docs/realizer-invocation.md: the
# positional TOON, --render, --scale (overrides/confirms the TOON scale: header),
# --samples, and --reference (the figure+grid scale overlay) when requested. Keep
# the `--` separator (Blender argv convention).
"$BLENDER" --background --python "$realizer" -- \
    "$toon" --render="$out" --scale="$scale" --samples="$samples" $reference

echo "render-crypt: DONE -> $out"
