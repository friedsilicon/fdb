# FDB Roadmap

Build a small embeddable C database with pluggable storage, reliable persistence,
transactions, and eventually multiple indexes and remote caching.

Current state: in-memory CRUD and iteration, basic snapshots, transaction
scaffolding. All 30 existing tests pass; correctness gaps remain and sanitizer
validation is unresolved. Complete the milestones below in order.

## P0. Tests and TDD

For each behavior change: write a failing test, confirm the failure, implement
the smallest fix, then refactor with tests passing. Apply this throughout the
roadmap, including any language migration; document exceptions when a meaningful
failing test is impractical.

- [x] Check libdict upstream: pin `897feb1` includes upstream `d2f32ed` plus
  three CMake commits; no newer upstream revision found on October 19, 2026.
- [ ] Before any dependency bump, baseline tests and known failures, review the
  target revision, preserve FDB build integration, and rerun the same tests.
- [ ] Expand public-API regressions for binary keys and size boundaries.
- [ ] Add allocation and I/O fault injection to verify failure atomicity.
- [ ] Test snapshot corruption and real transaction rollback before implementing fixes.
- [ ] Resolve sanitizer timeouts; gate changes on Debug/Release and ASan/UBSan CI.
- [ ] Keep public-API tests reusable across storage backends and language implementations.

## 1. Correct core storage

- [ ] Honor explicit key lengths; remove unsafe header reads and `strlen()` fallback.
- [ ] Validate size limits and allocation overflow at runtime.
- [ ] Preserve existing records when updates fail to allocate.
- [ ] Define errors, empty-value behavior, iterator lifetimes, and threading rules.
- [ ] Add binary-key, boundary, allocation-failure, and randomized CRUD tests.
- [ ] Exercise supported Linux/macOS targets in CI.

Done when arbitrary byte keys work and failed mutations preserve data, with
regression and sanitizer checks passing.

## 2. Ship reliable snapshots and embedding

- [ ] Propagate write, flush, and close errors; atomically replace snapshot files.
- [ ] Reject malformed input without partially changing the database.
- [ ] Define load replacement/merge behavior and support empty snapshots.
- [ ] Version the format; specify byte order, limits, and integrity checks.
- [ ] Test truncated files, duplicate keys, disk-full errors, and interrupted saves.
- [ ] Make CMocka optional, support static builds, and test an installed consumer.
- [ ] Decide whether the backend API is public; validate its callbacks and ownership.
- [ ] Align package versions and choose release scope and licensing.

Done when failed saves preserve the previous snapshot, failed loads preserve
database contents, and consumers build without test dependencies.

## 3. Implement real transactions

- [ ] Define transaction ownership and read visibility for one database first.
- [ ] Stage writes; make commit publish changes and abort discard them.
- [ ] Enforce state transitions, participant limits, and handle lifetimes.
- [ ] Test rollback of inserts, updates, and deletes against actual storage.
- [ ] Decide whether commits require durability; add logging/recovery if required.
- [ ] Add multi-database atomicity only with a defined failure/recovery protocol.

Done when commit and abort control stored data and failure tests verify the
chosen isolation and durability guarantees.

## 4. Extend indexes and caching

- [ ] Define composite-key encoding and ordering.
- [ ] Maintain secondary indexes atomically with primary records.
- [ ] Add a remote cache adapter with expiry, eviction, and invalidation policies.
- [ ] Test stale data, remote failures, and index consistency.
- [ ] Benchmark latency and memory against agreed workloads.

Before expanding scope, choose target platforms, memory budgets, and allocation
requirements for hosted versus constrained embedded use.

**Next change:** write and run failing binary-key and bounds regression tests,
then fix `src/storage/mem_backend.c` until they pass.
