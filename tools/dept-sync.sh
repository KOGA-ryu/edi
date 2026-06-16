#!/usr/bin/env bash
# Pull a department's worker commits from the Linux box DIRECTLY over SSH into its
# local worktree — no GitHub round-trip. The box builds; this brings the verified
# commits back to the Mac. The box's shared repo (~/edi/.git) holds every dept/*
# branch its worktrees commit to, so we fetch from there.
#   tools/dept-sync.sh edi-drafting | all
# Override the host/repo with EDI_WORKER_HOST / EDI_WORKER_REPO if needed.

set -euo pipefail
HOST="${EDI_WORKER_HOST:-linux-worker}"
REPO="${EDI_WORKER_REPO:-edi}"   # ~/edi on the box (the shared .git)

ensure_remote() {  # worktree
  local wt="$1"
  git -C "$wt" remote get-url box >/dev/null 2>&1 || git -C "$wt" remote add box "${HOST}:${REPO}"
}
sync_one() {  # dept  worktree  branch
  local dept="$1" wt="$2" br="$3"
  echo "── $dept ($br) ← box ──"
  [[ -d "$wt" ]] || { echo "  worktree missing: $wt"; return; }
  ensure_remote "$wt"
  git -C "$wt" fetch -q box "$br"
  if git -C "$wt" merge --ff-only "box/$br" 2>&1 | sed 's/^/  /'; then
    echo "  at $(git -C "$wt" rev-parse --short HEAD)"
  else
    echo "  DIVERGED (you also committed locally) — rebase in $wt before syncing"
  fi
}

case "${1:-}" in
  edi-drafting)    sync_one edi-drafting    "$HOME/edi-drafting"    dept/drafting ;;
  edi-blender-lab) sync_one edi-blender-lab "$HOME/edi-blender-lab" dept/blender-lab ;;
  edi-dungeon-map) sync_one edi-dungeon-map "$HOME/edi-dungeon-map" dept/dungeon-map ;;
  all)
    sync_one edi-drafting    "$HOME/edi-drafting"    dept/drafting
    sync_one edi-blender-lab "$HOME/edi-blender-lab" dept/blender-lab
    sync_one edi-dungeon-map "$HOME/edi-dungeon-map" dept/dungeon-map ;;
  *)
    echo "usage: $0 <edi-drafting|edi-blender-lab|edi-dungeon-map|all>" >&2
    exit 2 ;;
esac
