# Drawing Tool Workflow Fixtures

These fixtures drive deterministic drawing-tool workflows through the QML control runner. Use them to measure specific interaction paths instead of clicking around manually.

## Files

- `workflow_manifest.json`: selector metadata for each workflow.
- `workflow_metric_budgets.json`: group-level metric budgets by mode, kind, category, fixture, or tag.
- `shared_canvas_library.json`: shared named points, fragments, and per-script metric budgets.
- `*_basic.json`, `*_handle.json`, `*_object.json`: individual workflow scripts.

Manifest fields:

- `fixture`: JSON script file to run.
- `kind`: primary drawing object or system area, such as `line`, `rectangle`, or `viewport`.
- `category`: workflow class, such as `create`, `edit`, `move`, `selection`, or `viewport`.
- `tags`: searchable labels for coverage and focused runs.

## First Probe

Use compact dry-run first. It does not launch the app.

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
- `worstFailure`: most important compact failure, or `null`.
- `reportPath`: summary artifact written by real runs.

## Operating Rule

Probe with `--dry-run --compact`, then run the smallest selector that covers the behavior under investigation. Use full dry-run only when you need exact fixture names.
