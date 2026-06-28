# Changelog

All notable changes to this project will be documented in this file.

The format is based on
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to
[Semantic Versioning](https://semver.org/spec/v2.0.0.html).

### [Unreleased]

#### Added
- GNU standard `install`, `uninstall`, `dist`, `distcheck`,
  `distclean` make targets
- `make check` runs integration tests per GNU standard;
  clang-tidy moved to `make tidy`
- `make all MUSL=1 TARGET=Release` produces a fully static binary via `musl-gcc`
- CI jobs for musl static build verification and `distcheck`
- Integration tests for `make install` / `make uninstall`
- File indexing system for scanning and organizing files in directories
- File transaction system with two-phase commit
  (prepare/commit/rollback) for safe file operations
- Command-line interface (CLI) for user interaction
- Tag manipulation functions for managing file metadata
- String utility functions for common string operations
- Intrusive linked list implementation for efficient data structures
- File naming format with timestamp and tag support
  (`YYYY-MM-DD_XXX_tag1_tag2.ext`)
- Automatic file index numbering in filenames
- Alphabetical sorting of tags in filenames
- Doxygen documentation configuration
- clang-tidy static analysis configuration
- Automatic compiler selection for building
- Github CI to check successful builds
- Automated integration testing
- Github CI to run tests
- Timestamp override CLI flags: `--date` (absolute or offset tokens)
  and `--set-year`/`-month`/`-day` (absolute, component-wise)
- Timestamp override application: `--date` and `--set-*` flags now
  modify file dates before naming (offset tokens normalize calendar
  dates correctly; absolute forms require all source files to share a
  date)
- Destination files now have their mtime and atime set to match the
  computed timestamp when using `--date` or `--set-*` flags
- `--update`/`-u` flag: only copy/move source files newer than the
  most recent file already in the target directory, continuing
  per-date index numbering from the target's existing files

#### Changed
- Portable build process
