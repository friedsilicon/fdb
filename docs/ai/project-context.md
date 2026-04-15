# FDB Project Context

This is the canonical shared context for AI coding assistants working in this
repository. Keep it short, factual, and updated when project conventions
change.

## Project Summary

- FDB is a small embeddable C key-value database.
- The public API lives in `include/fdb/`.
- The current default storage implementation is the in-memory backend in
  `src/storage/mem_backend.c`.
- Core database orchestration lives in `src/db/db.c`.
- Persistence helpers live in `src/db/db_serialize.c`.
- Transaction coordination code lives in `src/txn/txn.c`.
- Unit tests are C tests using CMocka in `tests/*.c`.

## Build And Test

Use CMake to configure and Make to build:

```sh
cmake -S . -B build -DWITH_TESTS=ON
make -C build
```

Useful direct test commands:

```sh
./build/bin/test-fdb-init
./build/bin/test-fdb-ops
./build/bin/test-fdb-txn
./build/bin/test-fdb-serialize
./build/bin/simple
```

Notes:

- `ctest --output-on-failure` may be less reliable than running the built test
  binaries directly in some sandboxed environments.
- The top-level repo does not currently provide a root `Makefile`; `make -C build`
  is the expected Make-based workflow after CMake configure.

## Current API Shape

- The old opaque `fnode` API is gone.
- Use `fdb_node_t` from `include/fdb/fdb_types.h`.
- Check node validity with `fdb_node_valid(node)`.
- `fdb_find()` and `fiter_next()` return `fdb_node_t`, not pointers.
- `fdb_node_t` contains borrowed pointers into backend storage.
- Those `key` and `data` pointers are only valid until the next mutating
  operation on the same database handle, or until `fdb_deinit()`.

## String Size Convention

- FDB stores byte sequences, not just C strings.
- Always pass the intended byte length explicitly.
- For NUL-terminated string keys and values, prefer `strlen(s) + 1`.
- Be careful not to mix string-literal byte counts with `strlen()`-derived
  counts inconsistently across insert/find/update tests.

## Storage Backend Notes

- Storage is abstracted behind `fdb_storage_ops_t`.
- The internal backend interface is declared in `src/include/fdb_storage.h`.
- `fdb_init()` uses `fdb_mem_backend()` by default.
- Keep backend-facing code out of the public headers unless the API is meant
  for external consumers.
- `src/db/db.c` should remain storage-backend-agnostic and avoid direct
  `libdict` calls.

## Tests And Expectations

- Prefer updating or adding CMocka tests in `tests/*.c` when behavior changes.
- The legacy C++ test files were removed and should not be reintroduced unless
  the test framework strategy changes deliberately.
- When changing storage behavior, cover insert, update, remove, iteration, and
  save/load paths where relevant.

## Editing Guardrails

- Preserve existing local changes unless the user explicitly asks to revert.
- Prefer minimal, targeted patches over broad rewrites.
- Keep comments sparse and high-signal.
- Match the repo's current C style and C99 assumptions.
- Update `README.md` when user-facing build or API behavior changes.

## Good First Files To Inspect

- `README.md`
- `include/fdb/fdb.h`
- `include/fdb/fdb_types.h`
- `src/include/fdb_private.h`
- `src/include/fdb_storage.h`
- `src/db/db.c`
- `src/storage/mem_backend.c`
- `tests/fdbOpsTests.c`
