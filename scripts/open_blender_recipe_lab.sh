#!/usr/bin/env bash
set -euo pipefail

root_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
offscreen="${OFFSCREEN:-0}"

usage() {
  cat <<'USAGE'
Usage:
  open_blender_recipe_lab.sh [--offscreen]

Environment:
  OFFSCREEN    Set to 1 to force offscreen launch.
USAGE
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --help|-h)
      usage
      exit 0
      ;;
    --offscreen)
      offscreen=1
      shift
      ;;
    *)
      echo "Unknown argument: $1" >&2
      usage
      exit 1
      ;;
  esac
done

app="$root_dir/build/edi"
if [[ ! -x "$app" ]]; then
  echo "Executable not found: $app" >&2
  echo "Run cmake --build build first." >&2
  exit 1
fi

if [[ "$offscreen" == "1" ]]; then
  QT_QPA_PLATFORM=offscreen "$app"
else
  "$app"
fi
