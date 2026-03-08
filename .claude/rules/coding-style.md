---
paths:
  - "src/**"
---
# Coding Style

## Naming (enforced by clang-tidy)

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

New modules follow `prefix_*` pattern matching their header path
(e.g., `Files/Index.h` → `file_index_*`).

## Memory Management

- Always `calloc`, never `malloc`
- Every `calloc` followed by `PANIC_ON_BAD_ALLOC`
- `copy_string()` for string duplication (not `strdup`, which is POSIX)
- `goto`-based cleanup; functions with resources have a single cleanup
  section at the end
- Cleanup functions must be safe to call on zero-initialized structs

## Control Flow

- Goto-based error handling with single point of exit for complex
  resource management
- No nested ternary operators
- No operations with side-effects in `if` conditions

## Error Handling

- Explicit `!= 0` comparison for error codes and non-boolean integers
- No explicit comparison with zero for booleans
- Consult man pages for correct `errno` handling of libc functions
- Always check return values from functions that can fail
- Unrecoverable errors (bad alloc, null argument) use `PANIC()` macros,
  not propagation to user
- Null argument checks only in non-static functions
- Never call `exit()` from library code

## File Organization

- One `.c`/`.h` pair per logical unit
- Module = directory under `src/` (e.g., `Files/`, `Common/`)
- Include guards: `#ifndef __MODULE_NAME_H`
- Internal includes: `"Module/Header.h"` style (not `<>`)
- Source file includes its own header first

## Performance

- Prioritize readability over performance unless explicitly instructed
