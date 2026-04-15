# GitHub Copilot Instructions For FDB

Read `AGENTS.md` first, then use `docs/ai/project-context.md` for the detailed
shared project context.

Additional Copilot-specific guidance:

- Suggest changes that match the existing C99 style in this repo.
- Prefer the current public API based on `fdb_node_t`; do not suggest the old
  removed `fnode` helpers.
- When proposing commands, prefer:
  `cmake -S . -B build -DWITH_TESTS=ON`
  and then `make -C build`.
- When suggesting tests, target the CMocka tests in `tests/*.c`.
