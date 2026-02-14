#!/bin/sh

set -eu
. "$(dirname "$0")/assertions.sh"

SOURCE_DIR="$TEST_DIR/source"
TARGET_DIR="$TEST_DIR/target"

test_group "Preserve source"
    rm -rf "$SOURCE_DIR" "$TARGET_DIR"
    mkdir -p "$SOURCE_DIR" "$TARGET_DIR"
    create_test_file "$SOURCE_DIR/file1.txt" "content1"
    create_test_file "$SOURCE_DIR/file2.txt" "content2"

    assert_success "Just works"\
        "$BINARY" --source "$SOURCE_DIR" --target "$TARGET_DIR"

    assert_file_exists "Source file1 still exists" "$SOURCE_DIR/file1.txt"
    assert_file_exists "Source file2 still exists" "$SOURCE_DIR/file2.txt"
    assert_file_count "Target contains all files" "$TARGET_DIR" 2
finish_test || exit 1

test_group "Preserve content"
    rm -rf "$SOURCE_DIR" "$TARGET_DIR"
    mkdir -p "$SOURCE_DIR" "$TARGET_DIR"
    create_test_file "$SOURCE_DIR/file1.txt" "unique content 1"
    create_test_file "$SOURCE_DIR/file2.txt" "unique content 2"
    create_test_file "$SOURCE_DIR/file3.txt" "unique content 3"

    assert_success "Just works"
        "$BINARY" --source "$SOURCE_DIR" --target "$TARGET_DIR"

    assert_file_count "All files copied" "$TARGET_DIR" 3

    content_exists_in_target() {
        source_file="$1"
        for target_file in "$TARGET_DIR"/*.txt; do
            if [ -f "$target_file" ] \
                && cmp -s "$source_file" "$target_file"
            then
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

exit 0
