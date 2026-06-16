#!/usr/bin/env bash
# Runs ON the Linux worker. Starts a tmux session per role (department x role),
# each running its claude agent in BYPASS-PERMISSIONS mode (fully hands-off — no
# edit/command prompts), in the right worktree. Run after `claude` is logged in;
# then ~/dept-konsole.sh shows them on the box's desktop. Deploy to ~/dept-tmux.sh.
#   ~/dept-tmux.sh edi-drafting | all
set -euo pipefail
export PATH="$HOME/.local/bin:$PATH"

start_role() {  # session  dir  agent
  local sess="$1" dir="$2" agent="$3"
  if tmux has-session -t "$sess" 2>/dev/null; then echo "  $sess already running"; return; fi
  tmux new-session -d -s "$sess" -c "$dir"
  tmux send-keys -t "$sess" "claude --dangerously-skip-permissions --agent $agent" Enter
  sleep 4
  # auto-accept the one-time prompts: bypass-mode warning, then trust-folder
  if tmux capture-pane -t "$sess" -p 2>/dev/null | grep -q "Bypass Permissions mode"; then
    tmux send-keys -t "$sess" Down; sleep 0.3; tmux send-keys -t "$sess" Enter; sleep 1
  fi
  if tmux capture-pane -t "$sess" -p 2>/dev/null | grep -q "trust this folder"; then
    tmux send-keys -t "$sess" Enter; sleep 0.5
  fi
  echo "  started $sess  ($agent, bypass)"
}
start_dept() {  # dept  dir
  local dept="$1" dir="$2"
  start_role "${dept}-planner"    "$dir" "$dept"
  start_role "${dept}-reviewer"   "$dir" "${dept}-reviewer"
  start_role "${dept}-builder"    "$dir" "${dept}-builder"
  start_role "${dept}-researcher" "$dir" "${dept}-researcher"
}

case "${1:-}" in
  edi-drafting)    start_dept edi-drafting    "$HOME/edi-drafting" ;;
  edi-blender-lab) start_dept edi-blender-lab "$HOME/edi-blender-lab" ;;
  edi-dungeon-map) start_dept edi-dungeon-map "$HOME/edi-dungeon-map" ;;
  all)
    start_dept edi-drafting    "$HOME/edi-drafting"
    start_dept edi-blender-lab "$HOME/edi-blender-lab"
    start_dept edi-dungeon-map "$HOME/edi-dungeon-map" ;;
  *) echo "usage: $0 <edi-drafting|edi-blender-lab|edi-dungeon-map|all>"; exit 2 ;;
esac
echo "--- tmux sessions ---"; tmux ls 2>/dev/null || echo "(none)"
