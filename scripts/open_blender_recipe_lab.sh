#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PROJECT_PROFILE="${PROJECT_PROFILE:-$ROOT_DIR/data/project_profiles/draftsman_blender_recipe_lab.json}"
ACTIVITY="${ACTIVITY:-blender_recipe_lab}"
SCREENSHOT="${SCREENSHOT:-}"
WIDTH="${SCREENSHOT_WIDTH:-1280}"
HEIGHT="${SCREENSHOT_HEIGHT:-820}"
OFFSCREEN="${OFFSCREEN:-0}"

usage() {
  cat <<'USAGE'
Usage:
  open_blender_recipe_lab.sh [--activity <id>] [--project <path>] [--screenshot <path>] [--offscreen]

Environment:
  PROJECT_PROFILE    Path to project profile (default: draftsman_blender_recipe_lab.json in this repo)
  ACTIVITY          Activity id to start (default: blender_recipe_lab)
  SCREENSHOT        Optional screenshot path
  SCREENSHOT_WIDTH  Optional screenshot width (default: 1280)
  SCREENSHOT_HEIGHT Optional screenshot height (default: 820)
  OFFSCREEN         Set to 1 to force offscreen launch
USAGE
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --help|-h)
      usage
      exit 0
      ;;
    --activity)
      ACTIVITY="$2"
      shift 2
      ;;
    --project)
      PROJECT_PROFILE="$2"
      shift 2
      ;;
    --screenshot)
      SCREENSHOT="$2"
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

APP="$ROOT_DIR/build/qt_qml_region_split"
if [[ ! -x "$APP" ]]; then
  echo "Executable not found: $APP" >&2
  echo "Run cmake --build build first." >&2
  exit 1
fi

CMD=("$APP" --project-profile "$PROJECT_PROFILE" --activity "$ACTIVITY")
if [[ -n "$SCREENSHOT" ]]; then
  CMD+=(--screenshot "$SCREENSHOT" --width "$WIDTH" --height "$HEIGHT")
fi

if [[ "$OFFSCREEN" == "1" ]]; then
  QT_QPA_PLATFORM=offscreen "${CMD[@]}"
else
  "${CMD[@]}"
fi
