# FDB design contract

These requirements guide implementation. The current code does not yet satisfy
all of them. Work sequencing and undecided details belong in [ROADMAP.md](ROADMAP.md).

Memory-mapped I/O is the primary storage goal. Design records for direct access
in a mapped file; heap-backed operation must preserve the same default format.

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
- Initial transactions retain the existing API: one active transaction per
  database, with CRUD on that handle participating. Isolation, staging, and
  durability details require design before implementation.

## Change discipline

- Before implementation, identify affected requirements and the tests that will
  verify them. Surface missing decisions and conflicts before changing code.
- Obtain user agreement before changing record layout, allocator ownership, or
  transaction/snapshot memory strategy. Include allocation costs and failure cases.
- Follow TDD where practical. Behavioral tests alone do not establish layout,
  relocation, or allocation-policy compliance; test those properties explicitly.
- Follow the commit and review limits in the shared agent guide. Implementation
  remains paused while these design decisions are being settled.
