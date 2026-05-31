#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$repo_root"

scripts/validate_review_subjects.js data/review_subjects/draftsman_ui_taxonomy.json
cmake --build build

./build/qt_qml_region_split \
  --screenshot docs/proof/default_shell_1280x820.png \
  --width 1280 \
  --height 820

./build/qt_qml_region_split \
  --screenshot docs/proof/activity_rail_drilldown_1280x820.png \
  --route activity_rail \
  --width 1280 \
  --height 820

./build/qt_qml_region_split \
  --screenshot docs/proof/note_entry_1280x820.png \
  --route left_panel \
  --tab Notes \
  --note-status needs_rework \
  --note "Need clearer add/remove affordances before this becomes the project customization panel." \
  --width 1280 \
  --height 820

./build/qt_qml_region_split \
  --screenshot docs/proof/minimum_shell_960x620.png \
  --width 960 \
  --height 620
