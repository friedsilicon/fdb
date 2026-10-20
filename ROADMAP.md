# FDB Roadmap

Build an embedded C record database centered on memory-mapped I/O, with multiple
Patricia indices, private editable record copies, and subtree-locked concurrent
transactions. Remote caching is a later extension.

Follow [DESIGN.md](DESIGN.md) for architectural requirements. Zig migration is
deferred; continue with C.

Current state: in-memory CRUD and iteration, basic snapshots, transaction
scaffolding. The original 30 tests pass; correctness gaps remain and sanitizer
validation is unresolved. Complete the milestones below in order.

## Design decisions

- [x] Use identical, directly persistable record bytes in RAM and on disk by
  default; allow explicit translation for alternate encodings.
- [x] Establish the RDB-inspired target: Patricia indices, private record edits,
  and concurrent transactions protected by subtree locks, not field-level merging.
- [ ] Define record identity, C object access/translation, index key extraction,
  composite-key encoding, ordering, and uniqueness before index implementation.
- [ ] Specify record field widths, byte order, alignment, index references,
  versioning, and compatibility with existing files.
- [ ] Decide whether indexes persist in the mapping or are rebuilt on open;
  define mapping growth, reclamation, and borrowed-view invalidation.
- [ ] Define supported key/value sizes, empty-value behavior, and growth rules.
- [ ] Design the allocator interface later: ownership, alignment, exhaustion,
  reclamation, and index allocation. Preserve allocator independence meanwhile.
- [ ] Define threading and iterator/view lifetime guarantees.
- [ ] Review the transaction API for multiple active transactions. The earlier
  one-active-transaction-per-database decision is not the final target.
- [ ] Define subtree lock coverage across indices, lock acquisition/release and
  ordering, tree restructuring, deadlock handling, and read visibility.
- [ ] Define private-copy staging, atomic record/index publication, abort,
  reclamation, and durability; include bounded-memory exhaustion behavior.
- [ ] Define snapshot format, merge/replacement semantics, and staging memory costs.

## P0. Tests and TDD

For each behavior change: write a failing test, confirm the failure, implement
the smallest fix, then refactor with tests passing. Apply this throughout the
roadmap; document exceptions when a meaningful
failing test is impractical.

- [x] Check libdict upstream: pin `897feb1` includes upstream `d2f32ed` plus
  three CMake commits; no newer upstream revision found on October 19, 2026.
- [ ] Before any dependency bump, baseline tests and known failures, review the
  target revision, preserve FDB build integration, and rerun the same tests.
- [ ] Expand public-API regressions for binary keys and size boundaries.
- [x] Protect copied input ownership and adjacent key/value bytes across insert,
  growth, and shrink. These characterize existing behavior (no production fix
  or red phase); they do not establish portable headers or single-allocation use.
- [ ] Test contiguous layout, relocation, and portable record encoding against
  the agreed design before changing record representation.
- [ ] Verify mapped records after close/reopen and mapping at a different address,
  including malformed offsets and lengths.
- [ ] Add allocation and I/O fault injection to verify failure atomicity.
- [ ] Test snapshot corruption and real transaction rollback before implementing fixes.
- [ ] After transaction design review, test independent concurrent updates,
  overlapping lock coverage, secondary-index conflicts, structural tree changes,
  and atomic record/index commit or rollback using deterministic schedules.
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

## 2. Build mapped storage and persistence

- [ ] Implement mapping lifecycle and bounded record access using the default
  portable format, without allocating a separate heap object for each record.
- [ ] Implement the agreed index and free-space strategy for mapped storage.
- [ ] Propagate write, flush, and close errors; atomically replace snapshot files.
- [ ] Reject malformed input without partially changing the database.
- [ ] Define load replacement/merge behavior and support empty snapshots.
- [ ] Version the format; specify byte order, limits, and integrity checks.
- [ ] Test truncated files, duplicate keys, disk-full errors, and interrupted saves.
- [ ] Make CMocka optional, support static builds, and test an installed consumer.
- [ ] Decide whether the backend API is public; validate its callbacks and ownership.
- [ ] Align package versions and choose release scope and licensing.

Done when records remain readable after reopening at a different address,
snapshot failures preserve prior data, and consumers build without test dependencies.

## 3. Implement real transactions

- [ ] Implement the reviewed transaction ownership, lock, and visibility contracts.
- [ ] Stage private record copies through the selected memory policy.
- [ ] Allow concurrent transactions on independent subtrees; protect conflicts
  across all affected indices and structural tree changes.
- [ ] Publish records and index changes atomically; abort discards private changes.
- [ ] Implement the agreed deadlock prevention or detection and recovery policy.
- [ ] Enforce state transitions, participant limits, and handle lifetimes.
- [ ] Test rollback of inserts, updates, and deletes against actual storage.
- [ ] Decide whether commits require durability; add logging/recovery if required.
- [ ] Add multi-database atomicity only with a defined failure/recovery protocol.

Done when independent transactions can proceed concurrently, conflicting work is
protected, and failure tests verify atomic record/index updates and the chosen
isolation and durability guarantees.

## 4. Extend querying and caching

- [ ] Extend index/query capabilities beyond the reviewed transaction baseline.
- [ ] Add a remote cache adapter with expiry, eviction, and invalidation policies.
- [ ] Test stale data, remote failures, and index consistency.
- [ ] Benchmark latency and memory against agreed workloads.

Before expanding scope, choose target platforms, memory budgets, and allocation
requirements for hosted versus constrained embedded use.

**Next step:** review record representation, index identity/key semantics, and the
transaction/locking contract before dependent implementation. Detailed allocator
work can follow, but staging must not invent an unreviewed allocation strategy.
