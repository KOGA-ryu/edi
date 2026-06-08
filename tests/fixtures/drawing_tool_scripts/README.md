# Drawing Tool Workflow Fixtures

These fixtures drive deterministic drawing-tool workflows through the QML control runner. Use them to measure specific interaction paths instead of clicking around manually.

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

## First Probe

Ask for recommended selectors, then use compact dry-run first. Neither command launches the app.

```sh
node tests/helpers/drawing_control_workflow_report.js --recommend
```

```sh
node tests/helpers/drawing_control_workflow_report.js --recommend --id line_system
```

Recommendation output keeps coverage, metric collection, and baseline comparison as separate commands. Use `command` first, then `baselineCommand` when you want to know what changed.

```sh
node tests/helpers/drawing_control_workflow_report.js --tag line --dry-run --compact
```

Read `coverage` before running real metrics. If the selected set is wrong, adjust the selector instead of spending app-launch time.

## Common Commands

Inspect all workflow coverage:

```sh
node tests/helpers/drawing_control_workflow_report.js --all --dry-run --compact
```

Inspect selected fixtures with names:

```sh
node tests/helpers/drawing_control_workflow_report.js --category edit --dry-run
```

Run line workflows and write the summary report:

```sh
node tests/helpers/drawing_control_workflow_report.js --tag line
```

Run line workflows and compare against accepted baselines:

```sh
node tests/helpers/drawing_control_workflow_report.js --tag line --compare-baseline
```

Refresh accepted baselines after a known-good full run:

```sh
node tests/helpers/drawing_control_workflow_report.js --all --update-baseline
```

Run edit workflows:

```sh
node tests/helpers/drawing_control_workflow_report.js --category edit
```

Run one fixture:

```sh
node tests/helpers/drawing_control_workflow_report.js --fixture arc_create_basic.json
```

## Output Fields

- `coverage`: dry-run counts by `kind`, `category`, and `tag`.
- `selectedWorkflowCount`: number of workflows selected by the current filter.
- `metricSamples`: number of measured interaction samples in the real run.
- `budgetFailures`: number of workflow metric budget failures.
- `baselineComparison`: compact list of meaningful metric deltas when `--compare-baseline` is used.
- `baselineUpdate`: baseline file path and updated workflow count when `--update-baseline` is used.
- `worstFailure`: most important compact failure, or `null`.
- `reportPath`: summary artifact written by real runs.

## Coverage Contract

`workflow_coverage_expectations.json` turns dry-run coverage into a testable contract. If a fixture is removed or retagged and coverage drops below the minimums, `drawing_canvas_control_workflow_coverage_tests` fails before any app launch is needed.

The same file also protects focused selectors such as `--tag line`, `--category edit`, and exact fixture runs. This keeps the cheap probe commands useful as the workflow set changes.

`recommendedSelectors` in that file is the data-backed navigation map for agents. Start there when choosing a probe for a line, handle-edit, exact-fixture, or full-suite question. Each recommendation keeps the cheap coverage probe, real metric run, and baseline comparison command explicit.

## Baseline Contract

`workflow_metric_baselines.json` records accepted per-fixture summaries. Baseline comparison treats workflow shape, mode names, sample counts, action counts, object counts, and metric keys as invariants. Duration is compared with a regression threshold because wall-clock timing has normal jitter.

Use `--compare-baseline` when asking "what changed?" Use `--update-baseline` only after the changed behavior is understood and accepted.

## Operating Rule

Probe with `--dry-run --compact`, then run the smallest selector that covers the behavior under investigation. Use full dry-run only when you need exact fixture names.
