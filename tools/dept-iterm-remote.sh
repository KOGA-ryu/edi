#!/usr/bin/env bash
# COCKPIT: one iTerm window per department, four tabs (planner/reviewer/builder/
# researcher), each SSHing into the Linux worker and attaching that role's tmux
# session. The claude sessions + builds run on linux-worker (its RAM/CPU); the Mac
# just views and types. The local twin (tools/dept-iterm.sh) runs claude on the Mac
# instead — use this one to offload to the box.
#
#   tools/dept-iterm-remote.sh edi-drafting        # one department
#   tools/dept-iterm-remote.sh all                 # the three domain departments
#
# Prereq: on the box, `claude` is logged in and `~/dept-tmux.sh all` has started the
# sessions. HOST is the ssh alias from ~/.ssh/config.

set -euo pipefail
HOST="${EDI_WORKER_HOST:-linux-worker}"

make_dept() {  # dept
  local dept="$1"
  osascript <<OSA
tell application "iTerm"
  set w to (create window with default profile)
  set s to (current session of w)
  tell s to write text "ssh ${HOST} -t 'tmux attach -t ${dept}-planner'"
  set name of s to "planner"
  tell w to create tab with default profile
  set s to (current session of w)
  tell s to write text "ssh ${HOST} -t 'tmux attach -t ${dept}-reviewer'"
  set name of s to "reviewer"
  tell w to create tab with default profile
  set s to (current session of w)
  tell s to write text "ssh ${HOST} -t 'tmux attach -t ${dept}-builder'"
  set name of s to "builder"
  tell w to create tab with default profile
  set s to (current session of w)
  tell s to write text "ssh ${HOST} -t 'tmux attach -t ${dept}-researcher'"
  set name of s to "researcher"
  tell first tab of w to select
  activate
end tell
OSA
}

case "${1:-}" in
  edi-drafting|edi-blender-lab|edi-dungeon-map) make_dept "$1" ;;
  all) for d in edi-drafting edi-blender-lab edi-dungeon-map; do make_dept "$d"; done ;;
  *) echo "usage: $0 <edi-drafting|edi-blender-lab|edi-dungeon-map|all>" >&2; exit 2 ;;
esac
