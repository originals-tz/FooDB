# Validation

Run these commands from the repository root:

```bash
cmake -S . -B build
cmake --build build
./build/bpt_test
```

## What These Checks Cover

- `cmake -S . -B build` regenerates the build system from the current source tree.
- `cmake --build build` confirms the catalog layer, B+Tree layer, and test target compile together.
- `./build/bpt_test` exercises B+Tree insert/split/reload behavior and verifies the persisted file format.
- If a change touches only `src/catalog/`, rerun the build and `./build/bpt_test`; do not invent a separate narrower command unless the repo gets one.

## Notes

- The build is currently CMake-based and targets C++17.
- `bpt_test` is the narrowest meaningful regression target in this repository.
- There is no dedicated SQL test runner yet.
