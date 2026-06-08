# EDI

EDI is a C++ Qt Widgets shell for the drawing-core workbench and related project profiles.

The current runtime rule is simple: app behavior lives in typed C++ contracts and controllers. 

## Build

```sh
cmake -S . -B build
cmake --build build
```

## Run

```sh
./build/edi

```

## Validate


build/drawing_control_workflow_report --all --compare-baseline --failures-only
ctest --test-dir build --output-on-failure
```

## Source Map

- App shell and widget composition: `app/main.cpp`
- Drawing canvas widget: `src/widgets/DrawingCanvasWidget.cpp`
- Drawing model contracts: `src/core/`
- Canvas behavior contracts: `src/canvas/`
- Runtime catalogs and workflow planning: `src/runtime/`
- Validation and report CLIs: `src/tools/`
