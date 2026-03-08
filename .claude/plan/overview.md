# Development Plan — corgi MVP v1.0.0

## Milestone Overview

| #  | Milestone                        | File              | Depends On |
|----|----------------------------------|-------------------|------------|
| M1 | GNU Make targets & musl support  | `m1-packaging.md` | -          |
| M2 | Directory update feature         | `m2-update.md`    | -          |
| M3 | Search by date and tags          | `m3-search.md`    | -          |
| M4 | Interactive session — core TUI   | `m4-tui.md`       | -          |
| M5 | JPEG viewer integration          | `m5-viewer.md`    | M4         |
| M6 | Interactive session — full       | `m6-session.md`   | M4, M5     |
| M7 | Release preparation              | `m7-release.md`   | M1–M6      |

M1–M3 are independent and can be developed in parallel.
M4–M6 are sequential. M7 is the final integration milestone.

## Current State

The codebase on `master` includes:
- File indexing, tagging, and name generation
- Directory scanning with sorted insertion
- Two-phase commit transactions (copy/move/delete)
- CLI argument parsing (custom, no getopt_long)
- Integration test suite (shell-based)
- CI for Linux, macOS, FreeBSD (gcc + clang)
- Doxygen and clang-tidy configuration

Not yet implemented:
- Standard Make targets (install, dist, distcheck)
- musl static build
- Directory update (incremental sync)
- Search by date/tags
- Interactive session / TUI
- Media viewer
