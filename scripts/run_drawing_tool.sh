#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
APP_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
BUILD_DIR="${APP_DIR}/build"
APP_BIN="${BUILD_DIR}/qt_qml_region_split"
PROFILE_PATH="${APP_DIR}/data/project_profiles/draftsman_drawing_tool_blank.json"

cd "${APP_DIR}"

if [[ ! -x "${APP_BIN}" ]]; then
  cmake -S "${APP_DIR}" -B "${BUILD_DIR}"
fi

cmake --build "${BUILD_DIR}"

exec "${APP_BIN}" \
  --project-profile "${PROFILE_PATH}" \
  --activity drawing_tool \
  "$@"
