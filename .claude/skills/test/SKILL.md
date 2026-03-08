---
name: test
description: Write and run integration tests for corgi. Use when creating new tests, debugging test failures, or running the test suite.
---

# Testing Skill

## Running Tests

```sh
make test                           # Build and run all integration tests
make test-integration TESTS=cli     # Run a specific test suite
TEST_DIR=/tmp/my-tests make test    # Custom test directory
make test-setup                     # Create test directory
make test-clean                     # Remove test artifacts
```

### Environment Variables

- `TEST_DIR` — temporary test files (default: `.tmp/tests`)
- `CORGI_BINARY` — path to binary (default: `build/bin/corgi`)

## Writing New Tests

Create `tests/integration/test_<name>.sh`. Tests are auto-discovered
by `runner.sh`.

### Template

```sh
#!/bin/sh
set -eu
. "$SCRIPT_DIR/assertions.sh"

echo "My Feature Tests"

SOURCE_DIR="$TEST_DIR/source"
TARGET_DIR="$TEST_DIR/target"

test_group "Feature description"
rm -rf "$SOURCE_DIR" "$TARGET_DIR"
mkdir -p "$SOURCE_DIR" "$TARGET_DIR"

# Create test files, run binary, assert results
create_test_file "$SOURCE_DIR/photo.jpg" "test content"
assert_success "Should copy files" \
    "$BINARY" --source "$SOURCE_DIR" --target "$TARGET_DIR"
assert_file_count "Target has one file" "$TARGET_DIR" 1

finish_test || exit 1

exit 0
```

## Assertions

- `assert_success(desc, command...)` — command must exit 0
- `assert_failure(desc, command...)` — command must exit non-zero
- `assert_file_exists(desc, path)` — file must exist
- `assert_file_not_exists(desc, path)` — file must not exist
- `assert_contains(desc, haystack, needle)` — string contains substring
- `assert_contains_count(desc, haystack, needle, count)` — substring
  appears exactly `count` times
- `assert_filename_matches(desc, filename, regex)` — filename matches
- `assert_files_identical(desc, file1, file2)` — byte-identical files
- `assert_file_count(desc, dir, count)` — directory has N files

## Helpers

- `test_group(name)` — start a named test group
- `finish_test()` — finalize group result (call at end of each group)
- `create_test_file(path, content)` — create a file with content

## Rules

1. **Always cleanup**: `rm -rf` before each test group
2. **Check exit codes**: end each group with `finish_test || exit 1`
3. **POSIX shell only**: no `[[`, no `echo -e`, no bash-isms
4. **Descriptive messages**: assertion descriptions should explain intent
5. **Isolate tests**: each test group must be independent
