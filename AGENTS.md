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

Legacy drawing workflow metric artifacts are retained for reference, but telemetry collection is disabled unless explicitly requested.

Do not run telemetry collection unless the user explicitly re-enables it.

The repo has no JavaScript or QML runtime. Use the C++ workflow report CLI for compact review.

Do not read raw telemetry directly during normal review:

- `tests/artifacts/drawing_metrics/raw/*.jsonl`
- `tests/artifacts/drawing_metrics_cli/raw/*.jsonl`
- `tests/artifacts/drawing_metrics/*.log`

If the harness is explicitly re-enabled, use compact reports first:

```bash
build/drawing_control_workflow_report --all --compare-baseline --failures-only
```

For selection preview without launching the app:

```bash
build/drawing_control_workflow_report --all --dry-run --compact
```

Only inspect raw JSONL/log files when explicitly asked or when a compact report identifies a specific failing workflow that requires raw evidence.

Treat stale absolute path references to `/Users/kogaryu/draft`, `/Users/kogaryu/dev`, and `/Users/kogaryu/gameguy-3d-lab` as legacy unless explicitly reactivated.
