# Changelog

All notable changes to this project will be documented in this file.

The format is based on
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to
[Semantic Versioning](https://semver.org/spec/v2.0.0.html).

### [Unreleased]

#### Added
- File indexing system for scanning and organizing files in directories
- File transaction system with two-phase commit (prepare/commit/rollback) for safe file operations
- Command-line interface (CLI) for user interaction
- Tag manipulation functions for managing file metadata
- String utility functions for common string operations
- Intrusive linked list implementation for efficient data structures
- File naming format with timestamp and tag support (`YYYY-MM-DD_XXX_tag1_tag2.ext`)
- Automatic file index numbering in filenames
- Alphabetical sorting of tags in filenames
- Doxygen documentation configuration
- clang-tidy static analysis configuration
