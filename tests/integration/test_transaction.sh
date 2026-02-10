#!/bin/sh

set -eu
. "$(dirname "$0")/assertions.sh"

echo "Two-Phase Commit Tests"

SOURCE_DIR="$TEST_DIR/source"
TARGET_DIR="$TEST_DIR/target"

test_group "Rollback on file collision"
rm -rf "$SOURCE_DIR" "$TARGET_DIR"
mkdir -p "$SOURCE_DIR" "$TARGET_DIR"
create_test_file "$SOURCE_DIR/file1.txt" "content1"

"$BINARY" --source "$SOURCE_DIR" \
          --target "$TARGET_DIR" \
          --tag "first" > /dev/null

original_count=$(find "$TARGET_DIR" -type f | wc -l)

output=$("$BINARY" --source "$SOURCE_DIR" \
                   --target "$TARGET_DIR" \
                   --tag "first" 2>&1 || true)

assert_contains "Should report collision error" "$output" "already exists"

assert_file_count "Original files preserved" "$TARGET_DIR" "$original_count"
finish_test || exit 1

test_group "Force flag overwrites existing files"
rm -rf "$SOURCE_DIR" "$TARGET_DIR"
mkdir -p "$SOURCE_DIR" "$TARGET_DIR"
create_test_file "$SOURCE_DIR/test.txt" "original content"

"$BINARY" --source "$SOURCE_DIR" \
          --target "$TARGET_DIR" \
          --tag "version" > /dev/null

create_test_file "$SOURCE_DIR/test.txt" "updated content"
assert_success "Force overwrite succeeds" \
    "$BINARY" --source "$SOURCE_DIR" \
              --target "$TARGET_DIR" \
              --tag "version" --force

target_file=$(find "$TARGET_DIR" -name "*_version.txt" -type f | head -1)
content=$(cat "$target_file")
assert_contains "Content was updated" "$content" "updated content"
finish_test || exit 1

test_group "Verbose mode output"
rm -rf "$SOURCE_DIR" "$TARGET_DIR"
mkdir -p "$SOURCE_DIR" "$TARGET_DIR"
create_test_file "$SOURCE_DIR/file1.txt"
create_test_file "$SOURCE_DIR/file2.txt"

output=$("$BINARY" --source "$SOURCE_DIR" \
                   --target "$TARGET_DIR" \
                   --tag "verbose" --verbose 2>&1)

assert_contains "Shows file count" "$output" "Found"
assert_contains "Shows success message" "$output" "Successfully processed"
finish_test || exit 1

test_group "Multiple files processed together"
rm -rf "$SOURCE_DIR" "$TARGET_DIR"
mkdir -p "$SOURCE_DIR" "$TARGET_DIR"
create_test_file "$SOURCE_DIR/file1.txt"
create_test_file "$SOURCE_DIR/file2.txt"
create_test_file "$SOURCE_DIR/file3.txt"

"$BINARY" --source "$SOURCE_DIR" \
          --target "$TARGET_DIR" \
          --tag "batch"

assert_file_count "All files copied" "$TARGET_DIR" 3
assert_file_count "Source files preserved" "$SOURCE_DIR" 3
finish_test || exit 1

exit 0
