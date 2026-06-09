#!/bin/sh
set -eu

report="$("$1" --sample ready)"

printf '%s' "$report" | grep -q '^edi_plot_job_report$'
printf '%s' "$report" | grep -q '^status: ready$'
printf '%s' "$report" | grep -q '^calibration_scale: 2.000000$'
printf '%s' "$report" | grep -q '^stroke_segments: 2$'
printf '%s' "$report" | grep -q '^travel_segments: 1$'
printf '%s' "$report" | grep -q '^- pen id=pen_black ready=true'

calibrated_blocked="$("$1" --sample calibrated-blocked)"
printf '%s' "$calibrated_blocked" | grep -q '^status: blocked$'
printf '%s' "$calibrated_blocked" | grep -q '^warnings: 1$'
printf '%s' "$calibrated_blocked" | grep -q 'kind=calibrated_plot_out_of_drawable_bounds'
printf '%s' "$calibrated_blocked" | grep -q '^- reason=calibrated_plot_out_of_drawable_bounds$'

raw_blocked="$("$1" --sample raw-blocked)"
printf '%s' "$raw_blocked" | grep -q '^status: blocked$'
printf '%s' "$raw_blocked" | grep -q '^warnings: 1$'
printf '%s' "$raw_blocked" | grep -q 'kind=raw_out_of_drawable_bounds'
printf '%s' "$raw_blocked" | grep -q '^- reason=raw_out_of_drawable_bounds$'
