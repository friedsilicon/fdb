# FDB design contract

These requirements guide implementation. The current code does not yet satisfy
all of them. Work sequencing and undecided details belong in [ROADMAP.md](ROADMAP.md).

Memory-mapped I/O is the primary storage goal. Design records for direct access
in a mapped file; heap-backed operation must preserve the same default format.

The target is an embedded C record database inspired by the user's experience
with Ericsson RDB: multiple Patricia indices, private editable record copies,
and concurrent transactions protected by subtree locks. This describes FDB's
intended behavior, not a claim about RDB's internal implementation.

## Records and memory

- Each owned record is one contiguous block: metadata, key bytes, and value bytes.
  Do not allocate the value separately to simplify updates.
- Records must be relocatable. Moving a record must not require repairing
  absolute addresses stored inside it. Temporary borrowed views may use pointers;
  indexes referencing moved records need a defined update or handle strategy.
- On-disk records must be portable across supported architectures. Define field
  widths, byte order, and encoding explicitly rather than dumping native structs.
  The default record bytes in RAM and on disk are identical and directly
  persistable. Alternate encodings may use explicit translation; normal mapped
  access must not require materializing a separate heap record.
- Read metadata through accessors that respect the format's byte order and
  alignment. Keys and values remain opaque bytes; application payload encoding
  is the application's responsibility unless a separate codec is selected.
- Persistent references must survive mapping at another address. Define offsets
  or stable identifiers for indexes and allocation metadata; temporary process
  pointers must not become part of the persistent representation.
- Direct persistability does not provide atomic commit or crash recovery by
  itself. Mapping growth, borrowed-view invalidation, write ordering, and durable
  commit require explicit contracts before implementation.
- Memory-management policy must be replaceable without redefining the data
  structures. Support for heap, pool, or caller-provided memory belongs behind
  an allocation interface; its detailed design is deferred.
- Account for index nodes, iterators, transactions, and snapshot staging as well
  as records when describing memory costs. Do not silently add allocations or
  bypass a selected memory policy.
- A single arena for the whole database and zero allocations after initialization
  are undecided. Pool allocation can serve new records without calling the heap.

## API and architecture

- FDB is a small embeddable C library. Zig migration is deferred.
- Keys and values are byte sequences with explicit lengths.
- Storage backends remain separate from core orchestration and allocation policy.
- Public node views borrow storage; document when operations invalidate them.
- Multiple Patricia indices reference the same logical record. Define record
  identity, key extraction, composite-key encoding, and uniqueness before
  implementing index maintenance; the current libdict tree is not this target.
- Transactions edit private record copies through temporary C pointers, then
  publish whole-record changes and affected index updates atomically. Abort
  discards private changes. Working copies must respect the memory policy.
- Subtree locks prevent conflicting edits while allowing independent transactions
  to proceed concurrently. "Merge" means publishing record changes, not combining
  competing field edits. Lock coverage across indices, acquisition/release timing,
  tree restructuring, read visibility, and deadlock handling remain undecided.
- The earlier one-active-transaction-per-database choice is superseded as the
  final target. Any interim restriction and the eventual transaction API require
  review; do not silently constrain the architecture to the existing API.
- Editable C objects do not authorize persisting arbitrary native struct layouts
  or pointers. Define typed access or translation consistent with the portable
  record contract before introducing an object API.

## Change discipline

- Before implementation, identify affected requirements and the tests that will
  verify them. Surface missing decisions and conflicts before changing code.
- Obtain user agreement before changing record layout, allocator ownership, or
  transaction/snapshot memory strategy. Include allocation costs and failure cases.
- Use the explicit review triggers in the shared agent guide. Accepted design
  directions do not settle the open implementation contracts listed above.
- Follow TDD where practical. Behavioral tests alone do not establish layout,
  relocation, or allocation-policy compliance; test those properties explicitly.
- Follow the commit and review limits in the shared agent guide. Implementation
  remains paused while these design decisions are being settled.
