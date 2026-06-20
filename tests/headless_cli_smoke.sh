#!/bin/sh
set -eu

# D04: the shell binary must run a headless CLI action with NO display server
# and NO explicit platform plugin — main() falls back to the offscreen plugin
# instead of aborting at QApplication construction. Strip every display hint so
# the fallback is what's under test (an inherited QT_QPA_PLATFORM=offscreen from
# ctest would mask the bug), then generate a crypt TOON and assert the process
# both exits 0 and actually wrote a non-empty file.

edi_bin="$1"
work="$(mktemp -d)"
trap 'rm -rf "$work"' EXIT
out="$work/crypt.toon"

env -u DISPLAY -u WAYLAND_DISPLAY -u QT_QPA_PLATFORM "$edi_bin" --generate-crypt "$out"

# Non-empty output proves the headless terminus ran end-to-end, not just that the
# process survived construction.
test -s "$out"

# The TOON map header is the export's signature — a sanity check that we got a
# real map document, not a stray write. (The advisory 'scale:' line is omitted at
# the identity scale 1, so assert on the always-present kind/title instead.)
grep -q '^kind: map$' "$out"
grep -q '^title: crypt$' "$out"
