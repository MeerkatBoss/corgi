#!/bin/sh

set -eu
. "$(dirname "$0")/assertions.sh"

SOURCE_DIR="$TEST_DIR/source"
TARGET_DIR="$TEST_DIR/target"

setup() {
    rm -rf "$SOURCE_DIR" "$TARGET_DIR"
    mkdir -p "$SOURCE_DIR" "$TARGET_DIR"
}

setup_file() {
    setup
    create_test_file "$SOURCE_DIR/file.txt"
}

test_group "Basic usage"
setup

assert_success "Just works" \
    "$BINARY" --source "$SOURCE_DIR" \
              --target "$TARGET_DIR"
finish_test || exit 1

test_group "Create missing directory"
setup_file
rm -r "$TARGET_DIR"

assert_success "Still works" \
    "$BINARY" --source "$SOURCE_DIR" \
              --target "$TARGET_DIR"
assert_directory_exists "Target directory created" "$TARGET_DIR"
finish_test || exit 1

test_group "No source directory"
setup

assert_failure "Fails without --source" \
    "$BINARY" --target "$TARGET_DIR"
finish_test || exit 1

test_group "No target directory"
setup

assert_failure "Fails without --target" \
    "$BINARY" --source "$SOURCE_DIR"
finish_test || exit 1

test_group "Valid tags"
setup_file

assert_success "Accepts valid tags" \
    "$BINARY" --source "$SOURCE_DIR" \
              --target "$TARGET_DIR" \
              --tag "vacation" --tag "summer-trip" --dry-run
finish_test || exit 1

test_group "Dry-run mode"
setup_file

assert_success "Accepts --dry-run" \
    "$BINARY" --source "$SOURCE_DIR" \
              --target "$TARGET_DIR" \
              --tag "dryrun" --dry-run

assert_file_count "No files created" "$TARGET_DIR" 0
assert_file_count "Source file still exists" "$SOURCE_DIR" 1
finish_test || exit 1

test_group "Dry-run mode without target"
setup_file

assert_success "Accepts --dry-run" \
    "$BINARY" --source "$SOURCE_DIR" \
              --target "$TARGET_DIR" \
              --tag "dryrun" --dry-run

assert_directory_not_exists "Target directory not created" "$TARGET_DIR"
finish_test || exit 1

test_group "Verbose mode"
setup
create_test_file "$SOURCE_DIR/file1.txt"
create_test_file "$SOURCE_DIR/file2.txt"

output=$("$BINARY" --source "$SOURCE_DIR" \
                   --target "$TARGET_DIR" \
                   --tag "verbose" --verbose 2>&1)

assert_contains "File count reported" "$output" "Found 2 files"
assert_contains "Success reported" "$output" "Successfully processed 2 files"
assert_contains "Prepare reported" "$output" "Preparing 2 operations"
assert_contains "Commit reported" "$output" "Committing 2 operations"
assert_contains "Copy reported" "$output" "copy: $SOURCE_DIR/file1.txt ->"
assert_contains "NOP reported" "$output" "Nothing to commit"
finish_test || exit 1

test_group "Help command"
output=$("$BINARY" --help 2>&1 || true)
assert_contains "Help shows usage" "$output" "Usage:"
assert_contains "Help shows --source" "$output" "--source"
assert_contains "Help shows show --target" "$output" "--target"
finish_test || exit 1

exit 0
