# Drawing metric artifacts

This directory contains generated drawing workflow telemetry.

Raw JSONL and log files are retained intentionally for reproducibility and deep debugging, but they are not normal review input.

Normal review entrypoint:

```bash
node tests/helpers/drawing_control_workflow_report.js --all --compare-baseline --failures-only
```

Preview workflow selection:

```bash
node tests/helpers/drawing_control_workflow_report.js --all --dry-run --compact
```

Review order:

1. Run compact report.
2. Read summary/baseline result.
3. Identify workflow, metric, and subsystem.
4. Inspect raw JSONL/log only when explicitly needed.

Reason:

Raw telemetry is large and repetitive. The report harness exists to reduce token/context load and route failures cleanly.
