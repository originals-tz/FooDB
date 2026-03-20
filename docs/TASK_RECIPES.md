# Task Recipes

## Bug Fix

- Inspect the relevant layer first:
  - indexing bugs: `src/store/bptree.cpp`, `src/store/node.cpp`
  - schema/row/table bugs: `src/catalog/schema.cpp`, `src/catalog/row.cpp`, `src/catalog/table.cpp`
- Update or add a focused test in `test/bpt_test.cpp` or a new executable under `test/`.
- Validate with `cmake -S . -B build`, `cmake --build build`, and `./build/bpt_test`.

## Feature Work

- Keep indexing features in `src/store/`.
- Keep table/schema/row features in `src/catalog/`.
- Add new sources to `CMakeLists.txt` explicitly.
- Prefer introducing the smallest public API needed before wiring deeper storage behavior.
- If the feature needs persistence, decide whether it belongs in `.idx` or `.tbl` before writing code.

## Refactor

- Move behavior across `store` and `catalog` only if the boundary is becoming clearer.
- Avoid reintroducing duplicate node/page models.
- Check that the refactor still passes `./build/bpt_test`.
- Do not use directory-wide source globbing to hide boundary issues.

## Performance Work

- Start by measuring the hot path in `src/store/bptree.cpp` or `src/catalog/table.cpp`.
- Prefer keeping the current validation target and add a focused benchmark only if the change is meaningful.
- Keep performance work scoped to one layer unless the measurement proves the bottleneck crosses layers.

## Docs Work

- Keep `docs/sql.md` for grammar notes.
- Update `ARCHITECTURE.md` when layer boundaries change.
- Update `VALIDATION.md` when the build or test commands change.
