#!/bin/sh

set -eu
. "$(dirname "$0")/assertions.sh"

SOURCE_DIR="$TEST_DIR/source"
TARGET_DIR="$TEST_DIR/target"

test_group "Basic format"
rm -rf "$SOURCE_DIR" "$TARGET_DIR"
mkdir -p "$SOURCE_DIR" "$TARGET_DIR"
create_test_file "$SOURCE_DIR/photo.jpg"

assert_success "Just works" \
    "$BINARY" --source "$SOURCE_DIR" \
              --target "$TARGET_DIR"
assert_file_count "File created" "$TARGET_DIR" 1

target_file=$(find "$TARGET_DIR" -name "*.jpg" -type f | head -1)
filename=$(basename "$target_file")
assert_matches "Correct date format" \
    "$filename" "^[0-9]{4}-[0-9]{2}-[0-9]{2}_[0-9]{3}\.jpg$"
finish_test || exit 1

test_group "Single tag"
rm -rf "$SOURCE_DIR" "$TARGET_DIR"
mkdir -p "$SOURCE_DIR" "$TARGET_DIR"
create_test_file "$SOURCE_DIR/photo.jpg"

assert_success "Just works"\
    "$BINARY" --source "$SOURCE_DIR" \
              --target "$TARGET_DIR" \
              --tag "vacation"

target_file=$(find "$TARGET_DIR" -name "*.jpg" -type f | head -1)
filename=$(basename "$target_file")
assert_matches "Tag in filename" \
    "$filename" "^[0-9]{4}-[0-9]{2}-[0-9]{2}_[0-9]{3}_vacation\.jpg$"
finish_test || exit 1

test_group "Multiple tags"
rm -rf "$SOURCE_DIR" "$TARGET_DIR"
mkdir -p "$SOURCE_DIR" "$TARGET_DIR"
create_test_file "$SOURCE_DIR/photo.jpg"

assert_success "Just works" \
    "$BINARY" --source "$SOURCE_DIR" \
              --target "$TARGET_DIR" \
              --tag "zebra" --tag "apple" --tag "middle"

target_file=$(find "$TARGET_DIR" -name "*.jpg" -type f | head -1)
filename=$(basename "$target_file")
assert_contains "Tags sorted alphabetically" \
    "$filename" "apple_middle_zebra"
finish_test || exit 1

test_group "Preserve extension"
rm -rf "$SOURCE_DIR" "$TARGET_DIR"
mkdir -p "$SOURCE_DIR" "$TARGET_DIR"
create_test_file "$SOURCE_DIR/doc.txt"
create_test_file "$SOURCE_DIR/image.JPG"
create_test_file "$SOURCE_DIR/data.json"
create_test_file "$SOURCE_DIR/README"

assert_success "Just works" \
    "$BINARY" --source "$SOURCE_DIR" \
              --target "$TARGET_DIR" \
              --tag "mixed"

assert_file_count "All files copied" "$TARGET_DIR" 4
assert_file_exists "TXT file copied" "$(find "$TARGET_DIR" -name "*_mixed.txt" | head -1)"
assert_file_exists "JPG file copied (case-sensitive)" "$(find "$TARGET_DIR" -name "*_mixed.JPG" | head -1)"
assert_file_exists "JSON file copied" "$(find "$TARGET_DIR" -name "*_mixed.json" | head -1)"
assert_file_exists "No-extension file copied" "$(find "$TARGET_DIR" -name "*_mixed" -type f ! -name "*.*" | head -1)"
finish_test || exit 1

exit 0
