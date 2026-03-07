# Development Plan — corgi MVP v1.0.0

This document breaks the path to MVP v1.0.0 into ordered milestones.
Each milestone is self-contained and results in a working, testable state.

## Milestone Overview

| #  | Milestone                        | Branch Prefix      | Depends On |
|----|----------------------------------|--------------------|------------|
| M0 | Self-review fixes                | `fix/*`            | —          |
| M1 | GNU Make targets & musl support  | `feat/packaging`   | M0         |
| M2 | Directory update feature         | `feat/dir-update`  | M0         |
| M3 | Search by date and tags          | `feat/search`      | M0         |
| M4 | Interactive session — core TUI   | `feat/interactive` | M0         |
| M5 | JPEG viewer integration          | `feat/viewer`      | M4         |
| M6 | Interactive session — full       | `feat/interactive` | M4, M5     |
| M7 | Release preparation              | `release/v1.0.0`   | M1–M6      |

Milestones M1–M3 are independent of each other and can be developed in
parallel. M4–M6 are sequential. M7 is the final integration milestone.

---

## M0: Self-Review Fixes

Address known issues from the PR self-review before adding new features.
These are contained changes that improve correctness and maintainability.

**Scope:**
- Refactor `Transaction.c` commit logic into separate per-action functions
  (already partially done — verify and clean up)
- Improve memory management edge cases
- Enhance error message granularity (distinguish "file not found" vs
  "not a directory" vs "permission denied" more precisely)
- Maintain sorted/unique tag invariant in `IndexedFile` to avoid re-sorting
  on every `file_generate_name` call
- Review and fix any remaining TODO comments in codebase

**Exit criteria:** All existing tests pass, `make check` clean.

---

## M1: GNU Make Targets & musl Static Build

Add standard GNU Make targets for packaging and support fully static
builds with musl libc.

### M1.1: Standard GNU Make Targets

Add the following targets and variables to the Makefile:

**Targets:**
- `install` — install binary to `$(DESTDIR)$(PREFIX)/bin/`
- `uninstall` — remove installed binary
- `dist` — create source tarball `$(PROJECT)-$(VERSION).tar.gz`
- `distcheck` — create tarball, extract, build, test, clean up
- `distclean` — remove all generated files (build + dist artifacts)

**Variables (all overridable):**
- `PREFIX` — installation prefix (default: `/usr/local`)
- `DESTDIR` — staging directory for packaging (default: empty)
- `BINDIR` — binary installation directory (default: `$(PREFIX)/bin`)

**References:**
- [GNU Make standard targets](https://www.gnu.org/software/make/manual/html_node/Standard-Targets.html)
- [GNU directory variables](https://www.gnu.org/prep/standards/html_node/Directory-Variables.html)

### M1.2: musl Static Build Support

Add a build configuration for static linking with musl:

```sh
make all MUSL=1                     # Static build with musl-gcc
make all MUSL=1 TARGET=Release      # Static release build
```

**Implementation:**
- When `MUSL=1`, set `CC=musl-gcc` (or use `$(MUSL_CC)` for override)
- Add `-static` to `LDFLAGS`
- Disable sanitizers (incompatible with static musl builds)
- Verify no dynamic dependencies with `ldd` / `file` check

### M1.3: CI for musl Builds

Add a CI job that builds with musl and verifies the binary is static:

```yaml
build-musl:
  runs-on: ubuntu-latest
  steps:
    - uses: actions/checkout@v4
    - run: sudo apt-get install -y musl-tools
    - run: make all MUSL=1 TARGET=Release
    - run: file build/bin/corgi | grep -q "statically linked"
```

**Exit criteria:** `make install`, `make dist`, `make distcheck` work.
`make all MUSL=1` produces a static binary. CI verifies musl build.

---

## M2: Directory Update Feature

Implement "updating directory with more recent files" — the ability to
add only files from the source that are newer than the most recent file
already present in the target directory.

### M2.1: Target Directory Scanning

- Add `file_index_read_target_directory()` or extend existing scanning
  to determine the most recent timestamp in the target directory
- Parse existing filenames in target to extract dates and indices
- Determine the next available index for each date

### M2.2: Filtering by Recency

- Add `--update` / `-u` CLI flag
- When active, skip source files whose timestamp is not newer than
  the most recent file in target
- Respect existing index numbering to avoid collisions

### M2.3: Integration Tests

- Test update with empty target (should copy all)
- Test update with existing files (should skip old, copy new)
- Test index numbering continuity

**Exit criteria:** `corgi -s src/ -d dst/ --update` copies only newer files
and continues numbering from where the target directory left off.

---

## M3: Search by Date and Tags

Implement searching/filtering files in an organized directory by date
range and/or tags.

### M3.1: Filename Parsing

- Implement `file_parse_organized_name()` that extracts date, index,
  tags, and extension from a filename matching the naming format
- Handle edge cases: no tags, no extension, malformed names

### M3.2: Search CLI

- Add `--search` mode (mutually exclusive with default copy mode)
- Add `--date-from`, `--date-to` filters (format: `YYYY-MM-DD`)
- Reuse `--tag` to filter by tag presence
- Output matching filenames to stdout (one per line)

### M3.3: Integration Tests

- Test search by single tag, multiple tags
- Test search by date range
- Test combined date + tag search
- Test with no matches

**Exit criteria:** `corgi --search -d dir/ --tag vacation --date-from 2024-01-01`
lists matching files.

---

## M4: Interactive Session — Core TUI

Build the terminal user interface for interactive file organization.

### M4.1: Terminal Raw Mode & Input

- Implement terminal raw mode setup/teardown (using `termios`)
- Handle single-keypress input
- Handle terminal resize (`SIGWINCH`)
- Ensure clean restoration on exit, `SIGINT`, `SIGTERM`

### M4.2: File List Display

- Display scrollable list of files in terminal
- Show current file, action status, tags
- Highlight current selection
- Show progress (e.g., "12/47")

### M4.3: File Actions

- Key bindings for: copy (c), move (m), ignore (i), delete (d)
- Toggle between actions on current file
- Navigate with arrow keys / j/k
- Tag current file interactively (t → enter tag name)
- Remove tag from current file (T → select tag to remove)

### M4.4: Group Selection

- Enter selection mode (v), extend selection with movement keys
- Select all (A), select to end (G in selection mode)
- Apply action to entire selection
- Deselected deleted files by default
- Exit selection mode (Esc)

### M4.5: Session Finalization

- Preview files marked for deletion before executing (y/n confirm)
- Execute all pending operations via existing `FileTransaction` system
- Show summary of operations performed

**Exit criteria:** `corgi -s src/ -d dst/ --interactive` opens a TUI where
files can be tagged, marked for actions, and batch-processed.

---

## M5: JPEG Viewer Integration

Integrate an external image viewer to display the current file during
an interactive session.

### M5.1: Viewer Abstraction

- Define `Viewer` interface: `viewer_open()`, `viewer_show()`,
  `viewer_close()`
- Implement `xdg-open` backend as the simplest option
- Optionally detect and prefer `feh`, `imv`, or `sxiv` if available

### M5.2: Viewer Lifecycle

- Launch viewer as a child process when entering interactive mode
- Update displayed image when navigating to a new file
- Kill viewer process on session exit
- Handle viewer crash gracefully (restart or continue without)

### M5.3: SDL Embedded Viewer (stretch)

Per the roadmap, the MVP targets an embedded SDL viewer. If time permits:
- Embed SDL2 for window creation and image rendering
- Use `stb_image.h` or libjpeg for JPEG decoding
- Display image in a non-interactive window controlled by the TUI
- This can be deferred to a post-MVP release if the external viewer
  approach is sufficient for initial usability

**Exit criteria:** During an interactive session, the current file is
displayed in an image viewer window. Navigation in the TUI updates
the displayed image.

---

## M6: Interactive Session — Full Feature Set

Complete the remaining interactive session features.

### M6.1: Move vs Copy Selection

- Add keybind to toggle default action between copy and move
- Show current default action in status bar
- Add `--move` CLI flag to set default action to move

### M6.2: Per-File Tag Editing

- When pressing `t` on a file, enter tag input mode
- Show current tags, allow adding/removing
- Validate tag format in real-time
- Apply tags to selection if in selection mode

### M6.3: Deletion Preview

- Before finalizing, if any files are marked for deletion, show a
  confirmation screen listing all files to be deleted
- Allow canceling (return to TUI) or confirming (proceed)

### M6.4: Integration Tests

- Test interactive session via scripted input (pipe keystrokes)
- Test that operations are applied correctly
- Test deletion preview and confirmation

**Exit criteria:** All mandatory interactive session features from
`Requirements.md` are implemented and tested.

---

## M7: Release Preparation

Final integration, polish, and release.

### M7.1: Integration & Regression

- Merge all feature branches
- Run full test suite on all platforms (Linux, macOS, FreeBSD)
- Run `make distcheck`
- Build and test musl static binary

### M7.2: Documentation

- Write `README.md` with project description, installation, usage
- Update `CHANGELOG.md` with all changes for v1.0.0
- Verify Doxygen documentation builds cleanly

### M7.3: Release Artifacts

- Tag `v1.0.0`
- Create GitHub release with:
  - Source tarball (`make dist`)
  - Static Linux binary (musl build)
  - Changelog excerpt

**Exit criteria:** v1.0.0 is tagged and released on GitHub with
documentation, static binaries, and passing CI.
