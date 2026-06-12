#!/bin/sh
# edi_recipe_ops CLI smoke (R1-B05). $1 = the CLI binary, $2 = the samples dir.
# Proves the op pipeline is reachable end-to-end and gated: the doric ops load,
# pass the resolve gate (they are already resolved-shape), compile, validate,
# and render byte-equal to the committed previews; the compiled emission equals
# the committed compiled sample; and an UNRESOLVED stream is refused by name.
set -eu

cli="$1"
samples="$2"
ops="$samples/doric_column/doric_column_ops.toml"
previews="$samples/doric_column/previews"

tmp="$(mktemp -d)"
trap 'rm -rf "$tmp"' EXIT

# Happy path: load -> gate -> compile -> validate -> previews + compiled-out.
report="$("$cli" "$ops" --previews "$tmp" --compiled-out "$tmp/compiled.toml")"
printf '%s' "$report" | grep -q '^edi_recipe_ops$'
printf '%s' "$report" | grep -q '^status: ok$'

# The three projections are the committed previews, byte for byte.
cmp "$tmp/front.txt" "$previews/doric_front_preview.txt"
cmp "$tmp/side.txt" "$previews/doric_side_preview.txt"
cmp "$tmp/top.txt" "$previews/doric_top_preview.txt"

# The compiled emission is the committed compiled sample, byte for byte.
cmp "$tmp/compiled.toml" "$samples/doric_column/doric_column_ops_compiled.toml"

# The gate refuses an UNRESOLVED stream (a measurement binding, no --resolve):
# non-zero exit, message names the cause. A cylinder whose radius is bound.
cat > "$tmp/bound.toml" <<'EOF'
op.0.type = "AddCylinder"
op.0.name = "probe.shaft"
op.0.radius.object = "shaft_top"
op.0.radius.field = "radius"
op.0.height = "2"
op.0.z = "0"
EOF
if "$cli" "$tmp/bound.toml" >/dev/null 2>"$tmp/err"; then
    echo "smoke: expected refusal for an unresolved stream" >&2
    exit 1
fi
grep -q 'not resolved' "$tmp/err"

# A garbage path is refused, not crashed.
if "$cli" "$tmp/does_not_exist.toml" >/dev/null 2>/dev/null; then
    echo "smoke: expected refusal for a missing file" >&2
    exit 1
fi

echo "edi_recipe_ops cli smoke: ok"
