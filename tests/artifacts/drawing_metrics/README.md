# Drawing metric artifacts

This directory contains retained generated drawing workflow telemetry.

Telemetry collection is disabled unless explicitly requested.

Raw JSONL and log files are retained intentionally for reproducibility and deep debugging, but they are not normal review input.

Re-enable only when explicitly needed:

```bash
DRAFTSMAN_ENABLE_DRAWING_HARNESS=1 build/drawing_control_workflow_report --all --compare-baseline --failures-only
```

Legacy compact entrypoint:

```bash
build/drawing_control_workflow_report --all --compare-baseline --failures-only
```

Preview workflow selection:

```bash
build/drawing_control_workflow_report --all --dry-run --compact
```

Review order:

1. Run compact report.
2. Read summary/baseline result.
3. Identify workflow, metric, and subsystem.
4. Inspect raw JSONL/log only when explicitly needed.

Reason:

Raw telemetry is large and repetitive. The report harness exists to reduce token/context load and route failures cleanly.
