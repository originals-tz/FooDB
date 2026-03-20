# FooDB Agent Guide

This repository is a small C++ database prototype built around a persistent B+Tree and a thin catalog layer. The active codepaths are `src/store/` and `src/catalog/`; treat anything else as support or test-only code unless a change explicitly says otherwise.

## Core Areas

- `src/store/` contains the storage engine core: `BPTree`, `Node`, page encoding, and file persistence.
- `src/catalog/` contains the table abstraction: `Schema`, `Row`, and `Table`.
- `test/` contains executable tests. `test/bpt_test.cpp` is currently the main validation target.
- `docs/` contains design notes and workflow docs.

## Change Workflow

1. Inspect the relevant layer first.
2. Keep `src/store/` limited to index/page mechanics and `src/catalog/` limited to schema/row/table semantics.
3. Add new C++ sources to `CMakeLists.txt` explicitly; do not rely on directory globbing.
4. Run `cmake -S . -B build`, `cmake --build build`, and `./build/bpt_test` before finishing any non-doc change.

## Safety Rules

- Do not reintroduce the old `Leaf/Record/IndexNode/def.h` line of code.
- Do not add new table semantics to `BPTree`; it must remain an index/persistence component.
- Do not add new row/table serialization to `src/store/`; keep it in `src/catalog/`.
- Do not change `test/bpt_test.cpp` into a general integration harness unless the task explicitly requires it.

## References

- Architecture: [ARCHITECTURE.md](./ARCHITECTURE.md)
- Validation: [docs/VALIDATION.md](./docs/VALIDATION.md)
- Task recipes: [docs/TASK_RECIPES.md](./docs/TASK_RECIPES.md)
- Repo index: [docs/generated/repo-index.json](./docs/generated/repo-index.json)
