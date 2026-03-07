# Corgi — Console ORGanizer Interface

A command-line tool for organizing photos and files with timestamp-based
naming and tagging. Written in pure C (C99).

- Repository: https://github.com/MeerkatBoss/corgi
- Target milestone: **MVP v1.0.0** (see `.claude/PLAN.md`)

## Repository Layout

```
corgi/
├── src/
│   ├── Main.c                  # Entry point, CLI workflow orchestration
│   ├── Cli.c / Cli.h           # Command-line argument parsing
│   ├── Common/
│   │   ├── List.h              # Intrusive linked list (header-only)
│   │   ├── Panic.h             # Macros for unrecoverable error reporting
│   │   └── Strings.c/.h        # String manipulation helpers
│   └── Files/
│       ├── Error.c/.h          # Error codes and error-to-string conversion
│       ├── File.c/.h           # File metadata, tagging, and name generation
│       ├── Index.c/.h          # Directory scanning and file indexing
│       └── Transaction.c/.h    # Two-phase commit file operations
├── tests/
│   └── integration/            # Shell-based integration tests
│       ├── runner.sh           # Test runner
│       ├── assertions.sh       # Test helpers
│       └── test_*.sh           # Test suites
├── .claude/                    # Claude Code development files (this directory)
├── Makefile                    # Build system
├── Doxyfile                    # Doxygen configuration
├── .clang-tidy                 # Static analysis rules
├── .github/workflows/ci.yml   # CI: build + test on Linux/macOS/FreeBSD
├── CHANGELOG.md
└── TODO.md
```

## Key Architecture Decisions

- **Intrusive linked lists**: Files and operations use `LinkedListNode as_node`
  for memory efficiency
- **Two-phase commit**: All file operations are prepared first, then committed
  atomically
  - `prepare()` creates copies and validates operations
  - `commit()` performs destructive operations (delete source files)
  - `rollback()` removes created files if commit fails
- **Error handling**: Functions return `file_error_t` enum values, not errno
  directly
  - Each module uses context-specific `*_error_to_string()` helpers
  - All user-facing errors must explain the reason and be actionable
- **Tag validation**: Tags must contain only lowercase letters and dashes
  (validated by `file_tag_is_valid()`)
- **File naming format**: `YYYY-MM-DD_XXX_tag1_tag2.ext` (tags sorted
  alphabetically, deduplicated; index zero-padded to at least 3 digits;
  date from `override_timestamp`, defaults to `ctime`)
- **Resource cleanup pattern**: Use goto cleanup for proper resource
  deallocation
- **Function extraction**: Complex workflows extracted into helper functions
  (e.g., `execute_operations()` in Main.c)

## Build System

### Quick Reference

```sh
make all                    # Build debug binary (default)
TARGET=Release make all     # Build release binary
make run ARGS="..."         # Build and run
make test                   # Run integration tests
make check                  # Run clang-tidy
make clean                  # Remove object files
make cleaner                # Remove all build artifacts
make doc                    # Generate Doxygen documentation
```

### Compiler Selection

The Makefile auto-detects clang or gcc. Override with `CC=...`.
Both compilers are tested in CI. The build must remain compiler-agnostic.

### Build Output

- Objects: `build/obj/`
- Binary: `build/bin/corgi`
- Dependencies: `build/make/`

## Coding Style Guidelines

### Language & Standard

- **C99** (`-std=c99`), no GNU extensions
- No C++ anywhere in the codebase
- No `getopt_long` or other non-portable POSIX extensions

### Naming (enforced by clang-tidy)

| Entity              | Style              | Example                |
|---------------------|--------------------|------------------------|
| Enum types          | CamelCase          | `FileError`            |
| Enum constants      | UPPER_CASE         | `FERR_NONE`            |
| Struct types        | CamelCase          | `IndexedFile`          |
| Typedefs            | CamelCase or `*_t` | `file_error_t`         |
| Functions           | lower_case         | `file_add_tag`         |
| Parameters/variables| lower_case         | `tag_count`            |
| Static variables    | sCamelCase         | `sInstance`            |
| Macros              | UPPER_CASE         | `PANIC_IF_NULL`        |

### Module Prefixes

Functions and types use module-based prefixes:
- `file_*` — single file operations (`Files/File.h`)
- `file_index_*` — index operations (`Files/Index.h`)
- `file_transaction_*` — transaction operations (`Files/Transaction.h`)
- `file_error_*` — error utilities (`Files/Error.h`)

New modules should follow the same pattern.

### Memory Management

- Always use `calloc` instead of `malloc`
- Every `calloc` followed by `PANIC_ON_BAD_ALLOC`
- Use `copy_string()` for string duplication (not `strdup`, which is POSIX)
- Use goto-based cleanup for resource deallocation
- Functions with resources should have a single cleanup section at the end
- Cleanup functions must be safe to call on zero-initialized structs
- Intrusive linked list (`Common/List.h`) — nodes embedded in structs

### Control Flow

- For complicated error handling with resource cleanup, use goto-based error
  handling with single point of exit from function
- Avoid nested ternary operators
- Avoid operations with side-effects in `if` conditions

### Error Handling

- Recoverable errors use `file_error_t` return codes
- For integers that are not used as boolean values (e.g. error codes) always
  use explicit comparison with zero
- Never use explicit comparison with zero for booleans
- To ensure correct handling of `errno` values, consult man pages when using
  libc functions
- Always check return values from functions that can fail
- Unrecoverable errors, such as memory allocation failure or null argument,
  must be reported with fail-fast macros from `Common/Panic.h`, not propagated
  to the user
- Null argument should only be checked in non-static functions
- `PANIC` macros are always active (not disabled by `NDEBUG`)
- Never call `exit()` from library code; only from `main()` or CLI help

### File Organization

- Each module is a directory under `src/` (e.g., `Files/`, `Common/`)
- One `.c`/`.h` pair per logical unit
- Headers use include guards: `#ifndef __MODULE_NAME_H`
- Internal includes use `"Module/Header.h"` style (not `<>`)
- Source includes its own header first

### Documentation

- Doxygen comments on all public functions and types
- `@file`, `@author`, `@brief`, `@version`, `@date`, `@copyright` in headers
- `@param`, `@return`, `@note`, `@warning` on functions
- Use `/*!< ... */` for inline field documentation
- Big functions should contain short descriptive comments for main logic blocks
- Comments should not describe things that are obvious from immediately
  following code
- Do not write documentation for function parameters if their purpose is clear
  from their name and type
- Update function documentation when changing behavior or parameters

### Performance

- Prioritize code readability over performance unless explicitly instructed
  otherwise

## Error Message Guidelines

### General Principles

- All user-facing errors must include specific reason using context-specific
  error-to-string helpers
- Errors should be actionable (tell user what's wrong and how to fix it)
- Use consistent error message format:
  `"Error: <action> '<path/value>': <reason>"`

### Error-to-String Functions

- `file_error_to_string()` — generic error messages (defined in `Files/Error.c`)
- `directory_error_to_string()` — directory access errors (in `Main.c`)
- `file_tag_error_to_string()` — tag validation errors (in `Main.c`)

### Error Message Examples

```
Error: Failed to read source directory '.tmp/nonexistent': path is invalid or does not exist
Warning: Failed to add tag '2024' to file 'photo.jpg': tag contains invalid characters (only lowercase letters and '-' allowed)
```

## Testing

Tests are located in `tests/integration/` subdirectory.

### Running Tests

```sh
make test                           # Build and run all integration tests
make test-integration TESTS=cli     # Run specific test suite
TEST_DIR=/tmp/my-tests make test    # Run tests with custom test directory
```

### Writing New Tests

#### Basic Test Structure

```sh
#!/bin/sh
set -eu
. "$(dirname "$0")/test_helpers.sh"

echo "My Feature Tests"

# Setup
SOURCE_DIR="$TEST_DIR/source"
TARGET_DIR="$TEST_DIR/target"

# Test group
test_group "Feature description"
rm -rf "$SOURCE_DIR" "$TARGET_DIR"
mkdir -p "$SOURCE_DIR" "$TARGET_DIR"

# Your test code here
assert_success "Should work" \
    "$BINARY" --source "$SOURCE_DIR" --target "$TARGET_DIR"

finish_test || exit 1

exit 0
```

#### Available Assertions

- `assert_success(desc, command...)` — command must succeed
- `assert_failure(desc, command...)` — command must fail
- `assert_file_exists(desc, path)` — file must exist
- `assert_file_not_exists(desc, path)` — file must not exist
- `assert_contains(desc, haystack, needle)` — string contains substring
- `assert_contains_count(desc, haystack, needle, count)` — string contains
  substring exactly `count` times
- `assert_filename_matches(desc, filename, regex)` — filename matches pattern
- `assert_files_identical(desc, file1, file2)` — files are identical
- `assert_file_count(desc, dir, count)` — directory has N files

#### Helper Functions

- `test_group(name)` — start a test group
- `finish_test()` — return test group result (call at end of each group)
- `create_test_file(path, content)` — create a test file

#### Test Best Practices

1. **Always cleanup**: Use `rm -rf` before each test group
2. **Check exit codes**: End each test group with `finish_test || exit 1`
3. **Use POSIX shell**: No bash-specific features (`[[`, `echo -e`, etc.)
4. **Be specific**: Use descriptive assertion messages
5. **Isolate tests**: Each test group should be independent

### Test Directory

By default, tests use `./.tmp/tests` for temporary files. Configure with:

```sh
export TEST_DIR=/custom/path
make test
```

The test directory is created automatically by `make test-setup`, cleaned
before each test run, and removed by `make test-clean`.

### Test Makefile Targets

- `make test` — run all integration tests
- `make test-integration` — same as `make test`
- `make test-setup` — create test directory
- `make test-clean` — remove test artifacts

### Test Environment Variables

- `TEST_DIR` — directory for temporary test files (default: `.tmp/tests`)
- `CORGI_BINARY` — path to corgi binary (default: `build/bin/corgi`)

## Current State & Known Issues

See `TODO.md` for the authoritative list. Key items:
- Standard Make targets not yet implemented (install, dist, etc.)
- No musl static build support yet
- Tags are sorted on each name generation (optimize to maintain sorted order)
- No interactive session or media viewer yet
- No "update directory" feature (compare source/target timestamps)
- No search-by-date-and-tags feature

## Guidelines for Claude Code

1. **Read before writing.** Before modifying any file, read it fully to
   understand its current state and surrounding context.
2. **Respect the architecture.** Follow existing module structure, naming,
   and error handling patterns. Do not introduce new patterns without reason.
3. **Keep it portable.** No Linux-specific APIs unless guarded. The project
   builds on Linux, macOS, and FreeBSD. Use POSIX where needed, C99 where
   possible.
4. **Test everything.** Every new feature must have integration tests.
   Run `make test` after changes.
5. **One concern per commit.** Small, focused commits with descriptive
   messages (e.g., `feat: add install target to Makefile`).
6. **Run static analysis.** Use `make check` (clang-tidy) before finalizing.
7. **Consult the plan.** See `claude/PLAN.md` for milestone breakdown and
   `claude/TASKS.md` for the detailed task list.
