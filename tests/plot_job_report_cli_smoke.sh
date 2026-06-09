#!/bin/sh
set -eu

report="$("$1")"

printf '%s' "$report" | grep -q '^edi_plot_job_report$'
printf '%s' "$report" | grep -q '^status: ready$'
printf '%s' "$report" | grep -q '^calibration_scale: 2.000000$'
printf '%s' "$report" | grep -q '^stroke_segments: 2$'
printf '%s' "$report" | grep -q '^travel_segments: 1$'
printf '%s' "$report" | grep -q '^- pen id=pen_black ready=true'
