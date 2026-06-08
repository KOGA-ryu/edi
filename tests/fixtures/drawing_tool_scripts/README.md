# Drawing Tool Workflow Fixtures

These fixtures drive deterministic drawing-tool workflows through the C++ control runner. Use them to measure specific interaction paths instead of clicking around manually.

Status: telemetry collection is disabled by default. Real workflow runs require:

```sh
DRAFTSMAN_ENABLE_DRAWING_HARNESS=1 build/drawing_control_workflow_report --all --compare-baseline
```

Dry-run selector inspection still works without launching the app.

## Files

- `workflow_manifest.json`: selector metadata for each workflow.
- `workflow_coverage_expectations.json`: minimum coverage counts enforced by tests.
- `workflow_metric_budgets.json`: group-level metric budgets by mode, kind, category, fixture, or tag.
- `workflow_metric_baselines.json`: accepted per-fixture metric summaries used for delta comparisons.
- `shared_canvas_library.json`: shared named points, fragments, and per-script metric budgets.
- `*_basic.json`, `*_handle.json`, `*_object.json`: individual workflow scripts.

Manifest fields:

- `fixture`: JSON script file to run.
- `kind`: primary drawing object or system area, such as `line`, `rectangle`, or `viewport`.
- `category`: workflow class, such as `create`, `edit`, `move`, `selection`, or `viewport`.
- `tags`: searchable labels for coverage and focused runs.

Recommended selector fields:

- `command`: compact dry-run coverage probe.
- `runCommand`: real workflow metric run.
- `baselineCommand`: real workflow run plus accepted-baseline comparison.
- `failureCommand`: smallest baseline failure-state report.
- `failureCommands`: optional subsystem-specific failure-state reports.

## First Probe

Ask for recommended selectors, then use compact dry-run first. Neither command launches the app.

```sh
build/drawing_control_workflow_report --recommend
```

```sh
build/drawing_control_workflow_report --recommend --id line_system
```

Recommendation output keeps coverage, metric collection, baseline comparison, and failure-only comparison as separate commands. Use `command` first, then `failureCommand` when you only need to know whether accepted behavior changed. Use `failureCommands.<subsystem>` when investigating one area such as `rendering`, `gesture`, `handles`, or `controller`.

```sh
build/drawing_control_workflow_report --tag line --dry-run --compact
```

Read `coverage` before running real metrics. If the selected set is wrong, adjust the selector instead of spending app-launch time.

## Common Commands

Inspect all workflow coverage:

```sh
build/drawing_control_workflow_report --all --dry-run --compact
```

Inspect selected fixtures with names:

```sh
build/drawing_control_workflow_report --category edit --dry-run
```

Run line workflows and write the summary report:

```sh
build/drawing_control_workflow_report --tag line
```

Run line workflows and compare against accepted baselines:

```sh
build/drawing_control_workflow_report --tag line --compare-baseline
```

Focus baseline deltas on one subsystem:

```sh
build/drawing_control_workflow_report --tag line --compare-baseline --subsystem rendering
```

Ask only whether meaningful baseline failures exist:

```sh
build/drawing_control_workflow_report --tag line --compare-baseline --subsystem rendering --failures-only
```

Refresh accepted baselines after a known-good full run:

```sh
build/drawing_control_workflow_report --all --update-baseline
```

Run edit workflows:

```sh
build/drawing_control_workflow_report --category edit
```

Edit coverage includes representative handle roles across line endpoints, point position, circle center/radius, and rectangle corners.

Run one fixture:

```sh
build/drawing_control_workflow_report --fixture arc_create_basic.json
```

## Output Fields

- `coverage`: dry-run counts by `kind`, `category`, and `tag`.
- `selectedWorkflowCount`: number of workflows selected by the current filter.
- `metricSamples`: number of measured interaction samples in the real run.
- `budgetFailures`: number of workflow metric budget failures.
- `baselineComparison`: compact list of meaningful metric deltas when `--compare-baseline` is used.
- `selectedSubsystem`: requested subsystem filter when `--subsystem` is used with baseline comparison.
- `selectedSubsystemDeltaCount`: count of returned deltas for the selected subsystem.
- `baselineUpdate`: baseline file path and updated workflow count when `--update-baseline` is used.
- `worstFailure`: most important compact failure, or `null`.
- `reportPath`: summary artifact written by real runs.

## Coverage Contract

`workflow_coverage_expectations.json` turns dry-run coverage into a testable contract. If a fixture is removed or retagged and coverage drops below the minimums, `drawing_control_workflow_report_dry_run_tests` fails before any app launch is needed.

The same file also protects focused selectors such as `--tag line`, `--category edit`, and exact fixture runs. This keeps the cheap probe commands useful as the workflow set changes.

`recommendedSelectors` in that file is the data-backed navigation map for agents. Start there when choosing a probe for a line, handle-edit, exact-fixture, or full-suite question. Each recommendation keeps the cheap coverage probe, real metric run, and baseline comparison command explicit.

## Baseline Contract

`workflow_metric_baselines.json` records accepted per-fixture summaries. Baseline comparison treats workflow shape, mode names, sample counts, action counts, object counts, and metric keys as invariants. Duration is compared with a regression threshold because wall-clock timing has normal jitter.

Baseline `topDeltas` include `kind`, `subsystem`, and `recommendation` so agents can route failures without reading raw metrics. Current kinds include `missing_baseline`, `summary_changed`, `mode_added`, `mode_missing`, `metric_added`, `metric_missing`, `metric_regressed`, and `duration_regressed`.

Baseline comparison also includes `bySubsystem`, a compact routing table keyed by areas such as `baseline`, `workflow_fixture`, `gesture`, `controller`, `rendering`, `hit_test`, `snap`, `handles`, and `metrics`.

Use `--compare-baseline` when asking "what changed?" Use `--update-baseline` only after the changed behavior is understood and accepted.

Use `--failures-only` with baseline comparison when the caller only needs the failure state and actionable deltas, not normal run fields such as `metricSamples`, `reportPath`, or `worstFailure`.

## Operating Rule

Probe with `--dry-run --compact`, then run the smallest selector that covers the behavior under investigation. Use full dry-run only when you need exact fixture names.
