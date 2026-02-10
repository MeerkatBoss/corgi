#!/bin/sh

set -eu
. "$(dirname "$0")/assertions.sh"

SOURCE_DIR="$TEST_DIR/source"
TARGET_DIR="$TEST_DIR/target"

test_group "Basic usage"
rm -rf "$SOURCE_DIR" "$TARGET_DIR"
mkdir -p "$SOURCE_DIR"
mkdir -p "$TARGET_DIR"

assert_success "Just works" \
    "$BINARY" --source "$SOURCE_DIR" \
              --target "$TARGET_DIR"
finish_test || exit 1

test_group "Create missing directory"
rm -rf "$SOURCE_DIR" "$TARGET_DIR"
create_test_file "$SOURCE_DIR/test.txt"
mkdir -p "$SOURCE_DIR"

assert_success "Still works" \
    "$BINARY" --source "$SOURCE_DIR" \
              --target "$TARGET_DIR"
assert_directory_exists "Target directory created" "$TARGET_DIR"
finish_test || exit 1

test_group "No source directory"
mkdir -p "$TARGET_DIR"
assert_failure "Fails without --source" \
    "$BINARY" --target "$TARGET_DIR"
finish_test || exit 1

test_group "No target directory"
rm -rf "$SOURCE_DIR" "$TARGET_DIR"
mkdir -p "$SOURCE_DIR"
assert_failure "Fails without --target" \
    "$BINARY" --source "$SOURCE_DIR"
finish_test || exit 1

test_group "Valid tags"
rm -rf "$SOURCE_DIR" "$TARGET_DIR"
mkdir -p "$SOURCE_DIR" "$TARGET_DIR"
create_test_file "$SOURCE_DIR/test.txt"

assert_success "Accepts valid tags" \
    "$BINARY" --source "$SOURCE_DIR" \
              --target "$TARGET_DIR" \
              --tag "vacation" --tag "summer-trip" --dry-run
finish_test || exit 1

test_group "Dry-run mode"
rm -rf "$SOURCE_DIR" "$TARGET_DIR"
mkdir -p "$SOURCE_DIR" "$TARGET_DIR"
create_test_file "$SOURCE_DIR/file1.txt" "content1"
create_test_file "$SOURCE_DIR/file2.txt" "content2"

assert_success "Accepts --dry-run" \
    "$BINARY" --source "$SOURCE_DIR" \
              --target "$TARGET_DIR" \
              --tag "dryrun" --dry-run

assert_file_count "No files created" "$TARGET_DIR" 0
assert_file_exists "Source files still exist" "$SOURCE_DIR/file1.txt"
finish_test || exit 1

test_group "Dry-run mode without target"
rm -rf "$SOURCE_DIR" "$TARGET_DIR"
mkdir -p "$SOURCE_DIR"
assert_success "Accepts --dry-run" \
    "$BINARY" --source "$SOURCE_DIR" \
              --target "$TARGET_DIR" \
              --tag "dryrun" --dry-run

assert_directory_not_exists "Target directory not created" "$TARGET_DIR"
finish_test || exit 1

test_group "Help command"
output=$("$BINARY" --help 2>&1 || true)
assert_contains "Help shows usage" "$output" "Usage:"
assert_contains "Help shows --source" "$output" "--source"
assert_contains "Help shows show --target" "$output" "--target"
finish_test || exit 1

exit 0
