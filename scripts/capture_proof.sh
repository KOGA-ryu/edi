#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$repo_root"

build/edi_validate review-subjects data/review_subjects/draftsman_ui_taxonomy.json
build/edi_validate project-profiles \
  data/project_profiles/draftsman_blank.json \
  data/project_profiles/draftsman_ui_taxonomy.json \
  data/project_profiles/draftsman_drawing_tool_blank.json \
  data/project_profiles/draftsman_text_editor.json \
  data/project_profiles/draftsman_game_guy_map_editor.json
build/edi_validate ui-theme data/ui_theme.json
build/edi_validate shell-layout data/shell_layout.json
build/edi_validate design-principles data/design_principles.json
build/edi_validate csv-map-editor data/project_profiles/draftsman_game_guy_map_editor.json
build/edi_validate shell-surface-map data/shell_surface_map.json

cmake --build build

profiles=(
  data/project_profiles/draftsman_blank.json
  data/project_profiles/draftsman_drawing_tool_blank.json
  data/project_profiles/draftsman_text_editor.json
  data/project_profiles/draftsman_game_guy_map_editor.json
)

for profile in "${profiles[@]}"; do
  QT_QPA_PLATFORM=offscreen ./build/edi --project-profile "$profile" &
  pid=$!
  sleep 2
  kill "$pid" >/dev/null 2>&1 || true
  wait "$pid" >/dev/null 2>&1 || true
done
