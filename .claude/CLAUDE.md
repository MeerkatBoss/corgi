# Corgi — Console ORGanizer Interface

Console-based photo/video organizer in pure C (C99). Renames, tags, and
copies/moves media files using `YYYY-MM-DD_XXX_tag1_tag2.ext` format.

- Repository: https://github.com/MeerkatBoss/corgi
- Milestone: MVP v1.0.0 — see `.claude/plan/` for details

## Build & Test

```sh
make all                    # Debug build (default)
TARGET=Release make all     # Release build
make run ARGS="..."         # Build and run
make test                   # Integration tests
make check                  # Alias for test (GNU standard)
make tidy                   # clang-tidy static analysis
make clean                  # Remove objects
make cleaner                # Remove all build artifacts
```

Compiler auto-detected (clang/gcc). Override: `CC=...`. Must stay
compiler-agnostic — CI tests both on Linux, macOS, FreeBSD.

## Architecture

- **C99**, no GNU extensions, no `getopt_long`
- **Intrusive linked lists** — `LinkedListNode as_node` embedded in structs
- **Two-phase commit** — `prepare()` → `commit()` / `rollback()`
- **Error handling** — `file_error_t` return codes for recoverable errors;
  `PANIC()` macros for programmer errors (always active)
- **Module prefixes** — `file_*`, `file_index_*`, `file_transaction_*`
- Tags: lowercase letters and `-` only. Sorted alphabetically in filenames.

## Key Rules

- `calloc` over `malloc`; every allocation checked with `PANIC_ON_BAD_ALLOC`
- `copy_string()` for duplication (not `strdup`)
- `goto`-based cleanup with single exit point for functions with resources
- No side-effects in `if` conditions; no nested ternaries
- Explicit `!= 0` for error codes; no explicit comparison for booleans
- Portable: Linux + macOS + FreeBSD. Guard platform-specific APIs.

See `.claude/rules/` for detailed coding style, error message, and
documentation conventions (loaded automatically when working on matching files).

## Workflow

- Use `/project:commit` after completing a unit of work
- Use `/project:test` when writing or running tests
- Consult `.claude/plan/overview.md` for milestone roadmap
