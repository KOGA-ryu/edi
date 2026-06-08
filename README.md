# EDI

EDI is a C++ Qt Widgets runtime shell.


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

```sh
ctest --test-dir build --output-on-failure
```

## Source Map

- App shell: `app/main.cpp`
- Drawing model contracts: `src/core/`
- Canvas behavior contracts: `src/canvas/`
- Runtime catalogs and workflow planning: `src/runtime/`
