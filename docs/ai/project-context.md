# FDB Project Context

This is the canonical shared context for AI coding assistants working in this
repository. Keep it short, factual, and updated when project conventions
change.

Read [DESIGN.md](../../DESIGN.md) before changing implementation. Its architectural
requirements govern the roadmap; identify affected requirements and surface
missing decisions before coding. Implementation is paused for design discussion.

## Project Summary

- FDB is a small embeddable C key-value database.
- The public API lives in `include/fdb/`.
- The current default storage implementation is the in-memory backend in
  `src/storage/mem_backend.c`.
- Core database orchestration lives in `src/db/db.c`.
- Persistence helpers live in `src/db/db_serialize.c`.
- Transaction coordination code lives in `src/txn/txn.c`.
- Unit tests are C tests using CMocka in `tests/*.c`.
- See [ROADMAP.md](../../ROADMAP.md) for priorities and release milestones.

## Build And Test

Initialize submodules, then configure, build, and test with CMake:

```sh
git submodule update --init --recursive
cmake -S . -B build -DWITH_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
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

- CMocka must be installed and discoverable through CMake. The current build
  looks for it even with `WITH_TESTS=OFF`.
- There is no root `Makefile`. `make -C build` also works when CMake uses the
  Makefiles generator; `cmake --build build` works across generators.
- `libdict/` and `cmake_modules/` are Git submodules. Inspect their status before
  editing them; changes there belong to separate repositories.

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
- `fdb_init_with_storage()` is public, but its operations table is internal;
  external backend support is not yet a complete installed API.
- `src/db/db.c` should remain storage-backend-agnostic and avoid direct
  `libdict` calls.

## Current Limitations

- Transaction callbacks on normal database handles are stubs. Commit/abort do
  not provide storage isolation or rollback.
- Save/load is a raw snapshot format, not crash-safe persistence. It currently
  has incomplete error handling and writes native-endian lengths.
- The memory backend's lookup comparator does not safely honor arbitrary byte
  keys. Its size limits are assertions: keys below 64 bytes, values at most
  1024 bytes. Treat these as defects to fix, not stable API guarantees.
- No thread-safety guarantee exists. Node lifetime rules do not establish that
  iterators survive mutations or database destruction.

## Tests And Expectations

- Tests are P0. Follow TDD wherever practical: add a failing behavioral test,
  run it and confirm the expected failure, implement the smallest fix, then
  refactor with tests passing. Document exceptions when test-first is impractical.
- Use CMocka tests in `tests/*.c` for the current C implementation. Prefer public
  API assertions that remain useful across backend changes or a language migration.
- Cover failure paths with allocation/I/O fault injection and verify that failed
  operations preserve data. Callback counts alone do not prove transaction semantics.
- The legacy C++ test files were removed and should not be reintroduced unless
  the test framework strategy changes deliberately.
- When changing storage behavior, cover insert, update, remove, iteration, and
  save/load paths where relevant.

## Editing Guardrails

- Work down `ROADMAP.md` from P0. Make a separate, thematic commit for each
  completed item; split larger items into independently reviewable changes.
- Target at most roughly 1,000 changed lines per commit (additions + deletions).
  Keep any single review unit below 3,000 changed lines. Stop and request human
  review and design decisions before exceeding that limit; do not hide a larger
  inseparable change across small commits.
- Include regression tests with their fix after observing the expected failure.
  Keep committed checkpoints passing and record validation in progress updates.
- Notify the user when a design choice or review is needed. Continue independent
  authorized work while awaiting input; do not invent an answer to a pending decision.
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
