#!/usr/bin/env bash
# Open department workspaces in iTerm2: one WINDOW per department, four TABS
# (planner / reviewer / builder / researcher), each a `claude --agent` session in
# the department's worktree.
#
#   tools/dept-iterm.sh edi-drafting        # one department's window + 4 tabs
#   tools/dept-iterm.sh all                 # all four (16 sessions — open per-dept
#                                           #   when you can; idle sessions are cheap)
#
# Why iTerm and not Terminal.app: iTerm's AppleScript creates tabs directly
# (`create tab`), so this needs NO Accessibility permission. Terminal.app can only
# tab via ⌘T through System Events, which an automation process can't drive.
# NOTE: the AppleScript application name is "iTerm" (the bundle), not "iTerm2".
# Requires `claude` on PATH; the three domain worktrees (git worktree list);
# edi-ui works in ~/edi on master.

set -euo pipefail

make_dept() {  # dept  dir
  local dept="$1" dir="$2"
  osascript <<OSA
tell application "iTerm"
  set w to (create window with default profile)
  set s to (current session of w)
  tell s to write text "cd ${dir} && clear && claude --agent ${dept}"
  set name of s to "planner"
  tell w to create tab with default profile
  set s to (current session of w)
  tell s to write text "cd ${dir} && clear && claude --agent ${dept}-reviewer"
  set name of s to "reviewer"
  tell w to create tab with default profile
  set s to (current session of w)
  tell s to write text "cd ${dir} && clear && claude --agent ${dept}-builder"
  set name of s to "builder"
  tell w to create tab with default profile
  set s to (current session of w)
  tell s to write text "cd ${dir} && clear && claude --agent ${dept}-researcher"
  set name of s to "researcher"
  tell first tab of w to select
  activate
end tell
OSA
}

case "${1:-}" in
  edi-drafting)    make_dept edi-drafting    "$HOME/edi-drafting" ;;
  edi-blender-lab) make_dept edi-blender-lab "$HOME/edi-blender-lab" ;;
  edi-ui)          make_dept edi-ui          "$HOME/edi" ;;             # the shell owner works on master
  edi-dungeon-map) make_dept edi-dungeon-map "$HOME/edi-dungeon-map" ;;
  all)
    make_dept edi-drafting    "$HOME/edi-drafting"
    make_dept edi-blender-lab "$HOME/edi-blender-lab"
    make_dept edi-ui          "$HOME/edi"
    make_dept edi-dungeon-map "$HOME/edi-dungeon-map"
    ;;
  *)
    echo "usage: $0 <edi-drafting|edi-blender-lab|edi-ui|edi-dungeon-map|all>" >&2
    exit 2 ;;
esac
