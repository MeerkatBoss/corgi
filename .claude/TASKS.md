# Task List — corgi MVP v1.0.0

Granular tasks for each milestone. Check off as completed.

## M0: Self-Review Fixes

- [ ] Audit `Transaction.c` for overwrite handling edge cases
- [ ] Maintain sorted/unique tag invariant in `IndexedFile` (avoid
      re-sorting on every `file_generate_name` call)
  - [ ] Keep `tags[]` sorted on insertion in `file_add_tag`
  - [ ] Deduplicate on insertion rather than on retrieval
  - [ ] Update `file_remove_tag` to preserve sorted order
  - [ ] Simplify `file_get_unique_tags` accordingly
  - [ ] Update tests to cover sorted insertion
- [ ] Improve error message granularity in `Main.c` error reporting
- [ ] Review and resolve all `TODO` comments in source code
- [ ] Run `make check` — fix any clang-tidy findings
- [ ] Run `make test` — all tests pass

## M1: GNU Make Targets & musl Static Build

### M1.1: Standard GNU Make Targets

- [ ] Add `PREFIX ?= /usr/local` variable
- [ ] Add `DESTDIR ?=` variable
- [ ] Add `BINDIR ?= $(PREFIX)/bin` variable
- [ ] Implement `install` target
  - [ ] Create `$(DESTDIR)$(BINDIR)` directory
  - [ ] Install binary with mode 755
- [ ] Implement `uninstall` target
  - [ ] Remove `$(DESTDIR)$(BINDIR)/$(PROJECT)`
- [ ] Implement `dist` target
  - [ ] Create `$(PROJECT)-$(VERSION).tar.gz` from tracked files
  - [ ] Use `git archive` or manual tar with version-prefixed directory
- [ ] Implement `distcheck` target
  - [ ] Extract tarball to temp directory
  - [ ] Build in extracted directory
  - [ ] Run tests in extracted directory
  - [ ] Clean up temp directory
- [ ] Implement `distclean` target
  - [ ] Remove `build/`, dist tarballs, generated docs
- [ ] Add all new targets to `.PHONY`
- [ ] Write integration tests for install/uninstall

### M1.2: musl Static Build

- [ ] Add `MUSL ?= 0` flag
- [ ] When `MUSL=1`:
  - [ ] Set `CC = musl-gcc` (overridable via `MUSL_CC`)
  - [ ] Add `-static` to `LDFLAGS`
  - [ ] Disable sanitizers (`CDEBUG` without `-fsanitize=...`)
  - [ ] Force `TARGET=Release` or warn if Debug
- [ ] Test: `make all MUSL=1 TARGET=Release` produces static binary
- [ ] Test: `file build/bin/corgi` reports "statically linked"
- [ ] Test: binary runs on a minimal system (e.g., Alpine container)

### M1.3: CI for musl & Packaging

- [ ] Add `build-musl` CI job
  - [ ] Install `musl-tools`
  - [ ] Build with `MUSL=1 TARGET=Release`
  - [ ] Verify static linkage
- [ ] Add `distcheck` CI job
  - [ ] Run `make dist` and `make distcheck`
- [ ] Upload static binary as CI artifact

## M2: Directory Update Feature

### M2.1: Target Directory Scanning

- [ ] Implement `file_parse_organized_name()` in `Files/File.c`
  - [ ] Extract date (`YYYY-MM-DD`) → `struct tm` / `time_t`
  - [ ] Extract index (`XXX`)
  - [ ] Extract tag list
  - [ ] Extract extension
  - [ ] Return error for malformed names
- [ ] Implement target directory scanning
  - [ ] Read filenames in target directory
  - [ ] Parse each with `file_parse_organized_name()`
  - [ ] Track maximum timestamp and per-date maximum index
- [ ] Unit-level integration tests for name parsing

### M2.2: Update Mode

- [ ] Add `--update` / `-u` CLI flag to `Cli.h` / `Cli.c`
- [ ] In `Main.c`, when `--update`:
  - [ ] Scan target directory for existing organized files
  - [ ] Filter source index: skip files with `real_timestamp` ≤ max target
  - [ ] Set starting index per date from target's max index + 1
- [ ] Handle edge case: target directory doesn't exist yet (= copy all)
- [ ] Handle edge case: target has files but none match naming format

### M2.3: Integration Tests

- [ ] Test: update empty target → all files copied
- [ ] Test: update with existing newer files → only new files copied
- [ ] Test: index numbering continues from target's last index
- [ ] Test: update with `--force` overwrites existing
- [ ] Test: update with `--dry-run` shows what would be copied

## M3: Search by Date and Tags

### M3.1: Search Implementation

- [ ] Reuse `file_parse_organized_name()` from M2
- [ ] Implement `file_index_filter()` or similar
  - [ ] Filter by date range (inclusive)
  - [ ] Filter by tag presence (AND semantics: all specified tags required)
  - [ ] Combined filtering

### M3.2: Search CLI Mode

- [ ] Add `--search` flag (mutually exclusive with default mode)
- [ ] Add `--date-from DATE` option
- [ ] Add `--date-to DATE` option
- [ ] Validate date format (`YYYY-MM-DD`)
- [ ] In search mode, `--target` points to the organized directory to search
- [ ] `--source` is not required in search mode
- [ ] Output matching filenames to stdout, one per line

### M3.3: Integration Tests

- [ ] Test: search by single tag
- [ ] Test: search by multiple tags (AND)
- [ ] Test: search by date range
- [ ] Test: search by date + tag combined
- [ ] Test: search with no matches → empty output, exit 0
- [ ] Test: search in empty directory → empty output, exit 0

## M4: Interactive Session — Core TUI

### M4.1: Terminal Handling (`src/Tui/Terminal.c`)

- [ ] Implement `terminal_enter_raw_mode()` using `termios`
- [ ] Implement `terminal_restore_mode()` — safe to call multiple times
- [ ] Register `atexit` handler to restore terminal on any exit
- [ ] Handle `SIGINT`, `SIGTERM` — restore terminal, then exit
- [ ] Handle `SIGWINCH` — update stored terminal dimensions
- [ ] Implement `terminal_get_size()` → rows, cols
- [ ] Implement `terminal_read_key()` → key code
  - [ ] Handle escape sequences for arrow keys, Home, End, etc.
  - [ ] Return symbolic constants (e.g., `KEY_UP`, `KEY_DOWN`)

### M4.2: Screen Rendering (`src/Tui/Screen.c`)

- [ ] Implement screen buffer (double-buffering to reduce flicker)
- [ ] Implement `screen_clear()`, `screen_refresh()`
- [ ] Render file list with scrolling
  - [ ] Current file highlighted
  - [ ] Show: filename, action icon, tag list
  - [ ] Handle more files than terminal rows
- [ ] Render status bar (bottom): mode, progress, default action
- [ ] Render message bar: transient messages, prompts

### M4.3: Session Logic (`src/Session/Session.c`)

- [ ] Define `Session` struct: index ref, cursor, selection, state
- [ ] Main loop: render → read key → dispatch action → repeat
- [ ] Key dispatch table:
  - [ ] `j` / `↓` — move cursor down
  - [ ] `k` / `↑` — move cursor up
  - [ ] `c` — set action to copy
  - [ ] `m` — set action to move
  - [ ] `i` — set action to ignore
  - [ ] `d` — set action to delete
  - [ ] `t` — enter tag input mode
  - [ ] `T` — remove tag (select from list)
  - [ ] `v` — enter selection mode
  - [ ] `A` — select all
  - [ ] `G` — select to end (in selection mode)
  - [ ] `Esc` — cancel selection / exit mode
  - [ ] `Enter` / `x` — finalize session
  - [ ] `q` — quit without changes (confirm if changes pending)
- [ ] Tag input sub-mode:
  - [ ] Read tag string character by character
  - [ ] Validate in real-time (reject invalid characters)
  - [ ] `Enter` to confirm, `Esc` to cancel

### M4.4: Group Selection (`src/Session/Selection.c`)

- [ ] Track selection range (start, end) or bitmask
- [ ] Visual indication of selected files
- [ ] Apply action to all selected files
- [ ] Deselect files marked as deleted by default
- [ ] "Select all" and "select to end" shortcuts

### M4.5: Session Finalization

- [ ] Collect all file actions into `FileIndex` changes
- [ ] If deletions pending, show preview and ask for confirmation
- [ ] Execute via `file_transaction_prepare` → `commit`
- [ ] On failure, `rollback` and show error
- [ ] Show summary: N copied, N moved, N deleted, N ignored

### M4.6: CLI Integration

- [ ] Add `--interactive` / `-I` flag
- [ ] When set, enter interactive session instead of batch mode
- [ ] `--source` required, `--target` required
- [ ] Tags from CLI are applied as defaults (can be modified per-file)

### M4.7: Integration Tests

- [ ] Test: scripted keystrokes via stdin pipe
- [ ] Test: navigate and set actions
- [ ] Test: tag operations
- [ ] Test: finalize and verify file operations
- [ ] Test: quit without changes → no files modified

## M5: JPEG Viewer Integration

### M5.1: Viewer Abstraction (`src/Viewer/Viewer.c`)

- [ ] Define `Viewer` struct and interface
- [ ] `viewer_init()` — detect available viewer backend
- [ ] `viewer_show(path)` — display file
- [ ] `viewer_close()` — terminate viewer process
- [ ] Backend priority: `feh` → `imv` → `sxiv` → `xdg-open`

### M5.2: Process Management

- [ ] Launch viewer as child process (`fork` + `exec`)
- [ ] Track child PID
- [ ] On `viewer_show()`, signal viewer to update (or kill + relaunch)
- [ ] On `viewer_close()`, send `SIGTERM`, wait, then `SIGKILL`
- [ ] Handle `SIGCHLD` — detect viewer crash, set flag for TUI

### M5.3: TUI Integration

- [ ] On cursor movement in interactive session, call `viewer_show()`
- [ ] If viewer unavailable, show message in status bar
- [ ] Add `--no-viewer` flag to disable viewer

### M5.4: SDL Embedded Viewer (stretch goal)

- [ ] Link SDL2 for window management
- [ ] Use stb_image.h for JPEG decoding
- [ ] Render image in SDL window
- [ ] Accept commands via pipe from main process
- [ ] If SDL or JPEG libraries unavailable, fall back to external viewer

### M5.5: Integration Tests

- [ ] Test: viewer launch and termination (mock or check process)
- [ ] Test: `--no-viewer` mode works without viewer
- [ ] Test: session works if viewer binary not found

## M6: Interactive Session — Full Feature Set

### M6.1: Move vs Copy Selection

- [ ] Add status bar indicator for default action
- [ ] Keybind `M` to toggle default action (copy ↔ move)
- [ ] Add `--move` CLI flag to set initial default to move
- [ ] New files default to the selected action

### M6.2: Per-File Tag Editing Polish

- [ ] Show current tags in file detail area
- [ ] Autocomplete tags from other files in index
- [ ] Apply tag to entire selection if in selection mode

### M6.3: Deletion Preview Screen

- [ ] Separate screen showing files marked for deletion
- [ ] Scroll through deletion list
- [ ] `y` to confirm, `n` / `Esc` to return to TUI
- [ ] Option to unmark individual files from deletion screen

### M6.4: Integration Tests

- [ ] Test: toggle default action
- [ ] Test: per-file tag editing
- [ ] Test: deletion preview and confirmation
- [ ] Test: deletion preview and cancellation

## M7: Release Preparation

### M7.1: Integration & Regression

- [ ] Merge all feature branches into master
- [ ] Run full test suite: Linux (gcc, clang), macOS (gcc, clang), FreeBSD
- [ ] Run `make distcheck`
- [ ] Build musl static binary
- [ ] Manual smoke test with real photo collection

### M7.2: Documentation

- [ ] Write `README.md`
  - [ ] Project description and motivation
  - [ ] Installation instructions (from source, static binary)
  - [ ] Usage examples (batch mode, interactive mode, search)
  - [ ] Build instructions
  - [ ] Contributing guidelines
- [ ] Update `CHANGELOG.md` for v1.0.0
- [ ] Run `make doc` — verify Doxygen builds without warnings

### M7.3: Release

- [ ] Bump `VERSION` in Makefile to `1.0.0`
- [ ] Tag `v1.0.0`
- [ ] Create GitHub release
  - [ ] Attach source tarball
  - [ ] Attach static Linux x86_64 binary
  - [ ] Write release notes from CHANGELOG
- [ ] Announce (optional): r/C_programming, relevant communities
