#!/bin/sh

set -eu
. "$(dirname "$0")/assertions.sh"

echo "CLI Argument Parsing Tests"

SOURCE_DIR="$TEST_DIR/source"
TARGET_DIR="$TEST_DIR/target"

test_group "Missing source directory"
mkdir -p "$TARGET_DIR"
assert_failure "Should fail without --source" \
    "$BINARY" --target "$TARGET_DIR"
finish_test || exit 1

test_group "Missing target directory"
rm -rf "$SOURCE_DIR" "$TARGET_DIR"
mkdir -p "$SOURCE_DIR"
assert_failure "Should fail without --target" \
    "$BINARY" --source "$SOURCE_DIR"
finish_test || exit 1

test_group "Valid tags accepted"
rm -rf "$SOURCE_DIR" "$TARGET_DIR"
mkdir -p "$SOURCE_DIR" "$TARGET_DIR"
create_test_file "$SOURCE_DIR/test.txt"

assert_success "Should accept lowercase letters and dashes" \
    "$BINARY" --source "$SOURCE_DIR" \
              --target "$TARGET_DIR" \
              --tag "vacation" --tag "summer-trip" --dry-run
finish_test || exit 1

test_group "Dry-run mode creates no files"
rm -rf "$SOURCE_DIR" "$TARGET_DIR"
mkdir -p "$SOURCE_DIR" "$TARGET_DIR"
create_test_file "$SOURCE_DIR/file1.txt" "content1"
create_test_file "$SOURCE_DIR/file2.txt" "content2"

"$BINARY" --source "$SOURCE_DIR" \
          --target "$TARGET_DIR" \
          --tag "dryrun" --dry-run > /dev/null

assert_file_count "Dry-run creates nothing" "$TARGET_DIR" 0
assert_file_exists "Source files still exist" "$SOURCE_DIR/file1.txt"
finish_test || exit 1


test_group "Help command"
output=$("$BINARY" --help 2>&1 || true)
assert_contains "Help should show usage" "$output" "Usage:"
assert_contains "Help should show --source" "$output" "--source"
assert_contains "Help should show --target" "$output" "--target"
finish_test || exit 1

exit 0
