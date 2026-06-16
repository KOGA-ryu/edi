#!/usr/bin/env bash
# Pull a department's CLOUD-BUILDER commits down into its local worktree, so the
# Mac stays in sync with work done on the GitHub Actions runner. The other half of
# the chain: the hub triggers `gh workflow run cloud-builder.yml ...`, the runner
# pushes to dept/<name>, and this pulls it back.
#   tools/dept-sync.sh edi-drafting | all
# Fast-forward only: if local and cloud diverged (you also committed locally), it
# says so and leaves the worktree for you to rebase/merge deliberately.

set -euo pipefail

sync_one() {  # dept  worktree  branch
  local dept="$1" wt="$2" br="$3"
  echo "── $dept ($br) ──"
  if [[ ! -d "$wt" ]]; then echo "  worktree missing: $wt"; return; fi
  git -C "$wt" fetch origin "$br"
  if git -C "$wt" merge-base --is-ancestor "$br" "origin/$br"; then
    git -C "$wt" merge --ff-only "origin/$br" && echo "  synced → $(git -C "$wt" rev-parse --short HEAD)"
  else
    echo "  DIVERGED — local has commits the cloud doesn't; rebase in $wt before syncing"
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
