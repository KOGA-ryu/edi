#!/usr/bin/env bash
# Runs ON the Linux worker (KDE/Wayland). Opens Konsole windows on the box's own
# desktop — one window per department, four tabs (planner/reviewer/builder/
# researcher) each attached to its tmux session — so the running claude sessions
# are VISIBLE on the box's screen (not just detached in tmux). Pairs with
# ~/dept-tmux.sh (which starts the sessions). Deploy to the box at ~/dept-konsole.sh.
#   ~/dept-konsole.sh edi-drafting | all
set -euo pipefail
export XDG_RUNTIME_DIR="${XDG_RUNTIME_DIR:-/run/user/$(id -u)}"
export WAYLAND_DISPLAY="${WAYLAND_DISPLAY:-wayland-0}"
export DISPLAY="${DISPLAY:-:0}"

open_dept() {
  local dept="$1"
  local f="/tmp/konsole-${dept}.tabs"
  : > "$f"
  for role in planner reviewer builder researcher; do
    echo "title: ${dept#edi-} ${role} ;; command: tmux attach -t ${dept}-${role}" >> "$f"
  done
  setsid konsole --tabs-from-file "$f" >/dev/null 2>&1 < /dev/null &
  echo "  opened Konsole window for ${dept}"
}

case "${1:-}" in
  edi-drafting|edi-blender-lab|edi-dungeon-map) open_dept "$1" ;;
  all) for d in edi-drafting edi-blender-lab edi-dungeon-map; do open_dept "$d"; sleep 1; done ;;
  *) echo "usage: $0 <edi-drafting|edi-blender-lab|edi-dungeon-map|all>"; exit 2 ;;
esac
