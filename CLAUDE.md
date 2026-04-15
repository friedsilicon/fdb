# Claude Code Notes

Start with `docs/ai/project-context.md`.

`AGENTS.md` remains the root discovery file, but the detailed shared context
lives under `docs/ai/`.

Claude-specific reminders:

- Use the CMake configure step first, then build with `make -C build`.
- Prefer running the built test binaries directly if `ctest` output looks stale
  or sandbox-wrapped.
- Treat `docs/ai/project-context.md` as the canonical project guide and avoid
  duplicating its content here unless Claude-specific workflow advice is needed.
