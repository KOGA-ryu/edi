#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PROJECT_PROFILE="${PROJECT_PROFILE:-$ROOT_DIR/data/project_profiles/draftsman_blender_recipe_lab.json}"
OFFSCREEN="${OFFSCREEN:-0}"

usage() {
  cat <<'USAGE'
Usage:
  open_blender_recipe_lab.sh [--project <path>] [--offscreen]

Environment:
  PROJECT_PROFILE    Path to project profile.
  OFFSCREEN          Set to 1 to force offscreen launch.
USAGE
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --help|-h)
      usage
      exit 0
      ;;
    --project)
      PROJECT_PROFILE="$2"
      shift 2
      ;;
    --offscreen)
      OFFSCREEN=1
      shift
      ;;
    *)
      echo "Unknown argument: $1" >&2
      usage
      exit 1
      ;;
  esac
done

APP="$ROOT_DIR/build/edi"
if [[ ! -x "$APP" ]]; then
  echo "Executable not found: $APP" >&2
  echo "Run cmake --build build first." >&2
  exit 1
fi

CMD=("$APP" --project-profile "$PROJECT_PROFILE")

if [[ "$OFFSCREEN" == "1" ]]; then
  QT_QPA_PLATFORM=offscreen "${CMD[@]}"
else
  "${CMD[@]}"
fi
