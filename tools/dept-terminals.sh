#!/usr/bin/env bash
# Spin up the department workspace in macOS Terminal.app: one WINDOW per
# department, one TAB per role (planner / reviewer / builder / researcher),
# each a `claude` session launched as that role's agent, cwd = the department's
# worktree.
#
#   tools/dept-terminals.sh edi-drafting        # one department's window + 4 tabs
#   tools/dept-terminals.sh all                 # all four (16 sessions — token-heavy)
#
# VERIFY FIRST in your Terminal: `claude --help | grep -i agent`. This script
# assumes `claude --agent <name>` launches a session as a custom agent. If your
# CLI uses a different flag, edit AGENT_LAUNCH below.
#
# REQUIRES: Terminal.app must have Accessibility permission
# (System Settings → Privacy & Security → Accessibility) so the script can open
# tabs (it sends ⌘T via System Events). `claude` on PATH. The three domain
# worktrees exist (git worktree list); edi-ui works in ~/edi on master.

set -euo pipefail

# Per-role launch command. {AGENT} is replaced with the agent name, {DIR} with cwd.
AGENT_LAUNCH='cd {DIR} && clear && claude --agent {AGENT}'

open_tab() {  # label  agent  dir  is_first
  local label="$1" agent="$2" dir="$3" first="$4"
  local cmd="${AGENT_LAUNCH//\{AGENT\}/$agent}"; cmd="${cmd//\{DIR\}/$dir}"
  if [[ "$first" == "1" ]]; then
    osascript -e 'tell application "Terminal" to activate' \
              -e "tell application \"Terminal\" to do script \"$cmd\""
  else
    osascript -e 'tell application "System Events" to keystroke "t" using command down'
    sleep 0.4
    osascript -e "tell application \"Terminal\" to do script \"$cmd\" in front window"
  fi
  sleep 0.5
  osascript -e "tell application \"Terminal\" to set custom title of selected tab of front window to \"$label\""
}

launch_department() {  # dept  worktree
  local dept="$1" dir="$2"
  open_tab "$dept · planner"    "$dept"            "$dir" 1
  open_tab "$dept · reviewer"   "${dept}-reviewer"  "$dir" 0
  open_tab "$dept · builder"    "${dept}-builder"   "$dir" 0
  open_tab "$dept · researcher" "${dept}-researcher" "$dir" 0
}

case "${1:-}" in
  edi-drafting)    launch_department edi-drafting    "$HOME/edi-drafting" ;;
  edi-blender-lab) launch_department edi-blender-lab "$HOME/edi-blender-lab" ;;
  edi-ui)          launch_department edi-ui          "$HOME/edi" ;;            # the shell owner works on master
  edi-dungeon-map) launch_department edi-dungeon-map "$HOME/edi-dungeon-map" ;;
  all)
    launch_department edi-drafting    "$HOME/edi-drafting"
    launch_department edi-blender-lab "$HOME/edi-blender-lab"
    launch_department edi-ui          "$HOME/edi"
    launch_department edi-dungeon-map "$HOME/edi-dungeon-map"
    ;;
  *)
    echo "usage: $0 <edi-drafting|edi-blender-lab|edi-ui|edi-dungeon-map|all>" >&2
    exit 2 ;;
esac
