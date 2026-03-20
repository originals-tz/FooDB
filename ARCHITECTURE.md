# FooDB Architecture

FooDB is organized as a minimal database prototype with three active layers. There is no SQL parser, planner, transaction manager, or buffer pool yet.

## Runtime Layers

- `src/store/bptree.cpp` and `src/store/node.cpp` implement the persistent B+Tree index.
- `src/catalog/schema.cpp`, `src/catalog/row.cpp`, and `src/catalog/table.cpp` implement table metadata, row encoding, and table persistence.
- `src/foodb.cpp` is a smoke-test entrypoint only; it does not expose a user-facing database shell.
- `test/bpt_test.cpp` is the primary regression executable for index behavior and disk reload checks.

## Data Flow

1. A `Table` owns a `Schema`.
2. A `Row` is populated according to that schema and serialized for storage.
3. `Table` writes row snapshots to a `.tbl` file and uses `BPTree` to index the primary key in a `.idx` file.
4. `BPTree` persists pages to a separate file and reloads them on startup.
5. A reopened `Table` reconstructs its in-memory rows from `.tbl` and rebuilds the primary-key index as part of load.

## Boundaries

- `src/store/` should stay focused on indexing, page layout, and on-disk B+Tree mechanics.
- `src/catalog/` should stay focused on schema validation, row encoding, and table-level persistence.
- `test/` should only depend on public interfaces and should avoid reaching into internal node details unless a test is explicitly about index structure.
- `docs/sql.md` is documentation only; it is not an execution contract.

## High-Coupling Areas

- `Table` and `BPTree` are intentionally coupled through the primary-key index.
- `Row` serialization is coupled to `Schema` versioning and column order.
- `BPTree` persistence is coupled to fixed page sizing and node size configuration.
- `Table` currently rewrites the `.tbl` snapshot on insert; treat that as the current behavior unless a task explicitly changes persistence semantics.
