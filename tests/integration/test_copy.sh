#!/bin/sh

set -eu
. "$(dirname "$0")/assertions.sh"

echo "Copy Operation Tests"

SOURCE_DIR="$TEST_DIR/source"
TARGET_DIR="$TEST_DIR/target"

test_group "COPY operation preserves source"
rm -rf "$SOURCE_DIR" "$TARGET_DIR"
mkdir -p "$SOURCE_DIR" "$TARGET_DIR"
create_test_file "$SOURCE_DIR/file1.txt" "content1"
create_test_file "$SOURCE_DIR/file2.txt" "content2"

"$BINARY" --source "$SOURCE_DIR" \
          --target "$TARGET_DIR" \
          --tag "test"

assert_file_exists "Source file1 still exists" "$SOURCE_DIR/file1.txt"
assert_file_exists "Source file2 still exists" "$SOURCE_DIR/file2.txt"
assert_file_count "Target has copied files" "$TARGET_DIR" 2
finish_test || exit 1

test_group "Multiple files copied with correct content"
rm -rf "$SOURCE_DIR" "$TARGET_DIR"
mkdir -p "$SOURCE_DIR" "$TARGET_DIR"
create_test_file "$SOURCE_DIR/file1.txt" "unique content 1"
create_test_file "$SOURCE_DIR/file2.txt" "unique content 2"
create_test_file "$SOURCE_DIR/file3.txt" "unique content 3"

"$BINARY" --source "$SOURCE_DIR" \
          --target "$TARGET_DIR" \
          --tag "batch" > /dev/null

assert_file_count "All three files copied" "$TARGET_DIR" 3

content_exists_in_target() {
    source_file="$1"
    for target_file in "$TARGET_DIR"/*_batch.txt; do
        if [ -f "$target_file" ] && cmp -s "$source_file" "$target_file"; then
            return 0
        fi
    done
    return 1
}

for source_file in "$SOURCE_DIR"/*.txt; do
    assert_success "Content preserved for $(basename "$source_file")" \
        content_exists_in_target "$source_file"
done
finish_test || exit 1

test_group "Target directory created if missing"
rm -rf "$SOURCE_DIR" "$TARGET_DIR"
mkdir -p "$SOURCE_DIR"
create_test_file "$SOURCE_DIR/test.txt"

assert_success "Should create target directory" \
    "$BINARY" --source "$SOURCE_DIR" \
              --target "$TARGET_DIR" \
              --tag "auto"

assert_file_count "File copied to auto-created directory" "$TARGET_DIR" 1
finish_test || exit 1

test_group "Different file types handled"
rm -rf "$SOURCE_DIR" "$TARGET_DIR"
mkdir -p "$SOURCE_DIR" "$TARGET_DIR"
create_test_file "$SOURCE_DIR/doc.txt"
create_test_file "$SOURCE_DIR/image.jpg"
create_test_file "$SOURCE_DIR/data.json"
create_test_file "$SOURCE_DIR/README"

"$BINARY" --source "$SOURCE_DIR" \
          --target "$TARGET_DIR" \
          --tag "mixed"

assert_file_count "All file types copied" "$TARGET_DIR" 4
assert_file_exists "TXT file copied" "$(find "$TARGET_DIR" -name "*_mixed.txt" | head -1)"
assert_file_exists "JPG file copied" "$(find "$TARGET_DIR" -name "*_mixed.jpg" | head -1)"
assert_file_exists "JSON file copied" "$(find "$TARGET_DIR" -name "*_mixed.json" | head -1)"
assert_file_exists "No-extension file copied" "$(find "$TARGET_DIR" -name "*_mixed" -type f ! -name "*.*" | head -1)"
finish_test || exit 1

exit 0
