#!/bin/sh

set -eu
. "$(dirname "$0")/assertions.sh"

echo "File Naming Format Tests"

SOURCE_DIR="$TEST_DIR/source"
TARGET_DIR="$TEST_DIR/target"

test_group "Basic file naming format"
rm -rf "$SOURCE_DIR" "$TARGET_DIR"
mkdir -p "$SOURCE_DIR" "$TARGET_DIR"
create_test_file "$SOURCE_DIR/photo.jpg"

"$BINARY" --source "$SOURCE_DIR" \
          --target "$TARGET_DIR"

target_file=$(find "$TARGET_DIR" -name "*.jpg" -type f | head -1)
assert_file_exists "File created" "$target_file"

filename=$(basename "$target_file")
# Pattern: YYYY-MM-DD_XXX.jpg
assert_filename_matches "Date format YYYY-MM-DD_XXX.ext" \
    "$filename" "^[0-9]{4}-[0-9]{2}-[0-9]{2}_[0-9]{3}\.jpg$"
finish_test || exit 1

test_group "File naming with single tag"
rm -rf "$SOURCE_DIR" "$TARGET_DIR"
mkdir -p "$SOURCE_DIR" "$TARGET_DIR"
create_test_file "$SOURCE_DIR/photo.jpg"

"$BINARY" --source "$SOURCE_DIR" \
          --target "$TARGET_DIR" \
          --tag "vacation" > /dev/null

target_file=$(find "$TARGET_DIR" -name "*.jpg" -type f | head -1)
filename=$(basename "$target_file")
assert_filename_matches "Single tag in filename" \
    "$filename" "^[0-9]{4}-[0-9]{2}-[0-9]{2}_[0-9]{3}_vacation\.jpg$"
finish_test || exit 1

test_group "File naming with multiple sorted tags"
rm -rf "$SOURCE_DIR" "$TARGET_DIR"
mkdir -p "$SOURCE_DIR" "$TARGET_DIR"
create_test_file "$SOURCE_DIR/photo.jpg"

"$BINARY" --source "$SOURCE_DIR" \
          --target "$TARGET_DIR" \
          --tag "zebra" --tag "apple" --tag "middle"

target_file=$(find "$TARGET_DIR" -name "*.jpg" -type f | head -1)
filename=$(basename "$target_file")
assert_contains "Tags sorted alphabetically" \
    "$filename" "apple_middle_zebra"
finish_test || exit 1

test_group "File extension preservation"
rm -rf "$SOURCE_DIR" "$TARGET_DIR"
mkdir -p "$SOURCE_DIR" "$TARGET_DIR"
create_test_file "$SOURCE_DIR/document.pdf"
create_test_file "$SOURCE_DIR/image.PNG"

"$BINARY" --source "$SOURCE_DIR" \
          --target "$TARGET_DIR" \
          --tag "test"

pdf_file=$(find "$TARGET_DIR" -name "*_test.pdf" -type f | head -1)
png_file=$(find "$TARGET_DIR" -name "*_test.PNG" -type f | head -1)

assert_file_exists "PDF extension preserved" "$pdf_file"
assert_file_exists "PNG extension preserved (case-sensitive)" "$png_file"
finish_test || exit 1

test_group "Files without extensions"
rm -rf "$SOURCE_DIR" "$TARGET_DIR"
mkdir -p "$SOURCE_DIR" "$TARGET_DIR"
create_test_file "$SOURCE_DIR/README"

"$BINARY" --source "$SOURCE_DIR" \
          --target "$TARGET_DIR" \
          --tag "docs"

target_file=$(find "$TARGET_DIR" -name "*_docs" -type f | head -1)
assert_file_exists "File without extension handled correctly" "$target_file"
finish_test || exit 1

exit 0
