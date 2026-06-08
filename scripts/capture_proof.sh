#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$repo_root"

cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure

QT_QPA_PLATFORM=offscreen ./build/edi &
pid=$!
sleep 2
kill "$pid" >/dev/null 2>&1 || true
wait "$pid" >/dev/null 2>&1 || true
