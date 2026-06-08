#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "${script_dir}/.." && pwd)"
build_dir="${repo_root}/build"
app_bin="${build_dir}/edi"

cd "$repo_root"

if [[ ! -x "$app_bin" ]]; then
  cmake -S "$repo_root" -B "$build_dir"
fi

cmake --build "$build_dir"
exec "$app_bin" "$@"
