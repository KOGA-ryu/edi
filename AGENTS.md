# EDI Codex Policy

This project root is:

```text
/Users/kogaryu/edi
```

Work only inside this repo unless the user explicitly asks otherwise.

Do not inspect or use sibling/stale project directories, including:

- `/Users/kogaryu/draft`
- `/Users/kogaryu/dev`
- `/Users/kogaryu/game`
- `/Users/kogaryu/gameguy-3d-lab`
- `/Users/kogaryu/ui`
- `/Users/kogaryu/Documents`

Do not use Gmail, Google, Calendar, browser, web, apps, plugins, MCP, or memories for ordinary code work unless explicitly requested.

Before edits, confirm:

- `pwd` is `/Users/kogaryu/edi`
- git root is `/Users/kogaryu/edi`
- `git status --short` has been checked

Legacy drawing workflow metric artifacts are retained for reference, but the drawing telemetry harness is disabled while canvas behavior is migrated to C++.

Do not run telemetry collection unless the user explicitly re-enables it.

CMake does not register legacy JS/QML canvas harness tests by default. To opt in temporarily:

```bash
cmake -S . -B build -DDRAFTSMAN_ENABLE_DRAWING_JS_HARNESS_TESTS=ON
DRAFTSMAN_ENABLE_DRAWING_HARNESS=1 node tests/helpers/drawing_control_workflow_report.js --all --compare-baseline --failures-only
```

Do not read raw telemetry directly during normal review:

- `tests/artifacts/drawing_metrics/raw/*.jsonl`
- `tests/artifacts/drawing_metrics_cli/raw/*.jsonl`
- `tests/artifacts/drawing_metrics/*.log`

If the harness is explicitly re-enabled, use compact reports first:

```bash
node tests/helpers/drawing_control_workflow_report.js --all --compare-baseline --failures-only
```

For selection preview without launching the app:

```bash
node tests/helpers/drawing_control_workflow_report.js --all --dry-run --compact
```

Only inspect raw JSONL/log files when explicitly asked or when a compact report identifies a specific failing workflow that requires raw evidence.

Treat stale absolute path references to `/Users/kogaryu/draft`, `/Users/kogaryu/dev`, and `/Users/kogaryu/gameguy-3d-lab` as legacy unless explicitly reactivated.
