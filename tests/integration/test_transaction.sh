#!/bin/sh

set -eu
. "$(dirname "$0")/assertions.sh"

SOURCE_DIR="$TEST_DIR/source"
TARGET_DIR="$TEST_DIR/target"

test_group "Rollback on file collision"
    rm -rf "$SOURCE_DIR" "$TARGET_DIR"
    mkdir -p "$SOURCE_DIR" "$TARGET_DIR"
    create_test_file "$SOURCE_DIR/file1.txt" "content1"

    assert_success "Works first time" \
        "$BINARY" --source "$SOURCE_DIR" --target "$TARGET_DIR"

    original_count=$(find "$TARGET_DIR" -type f | wc -l)

    output=$("$BINARY" --source "$SOURCE_DIR" \
                       --target "$TARGET_DIR" 2>&1 || true)

    assert_contains "Error reported" "$output" "Error:"
    assert_contains "Cause reported" "$output" "already exists"
    assert_file_count "Original files preserved" "$TARGET_DIR" "$original_count"
finish_test || exit 1

test_group "Force flag"
    rm -rf "$SOURCE_DIR" "$TARGET_DIR"
    mkdir -p "$SOURCE_DIR" "$TARGET_DIR"
    create_test_file "$SOURCE_DIR/test.txt" "original content"

    assert_success "Works first time" \
        "$BINARY" --source "$SOURCE_DIR" --target "$TARGET_DIR"

    create_test_file "$SOURCE_DIR/test.txt" "updated content"
    assert_success "Force overwrite succeeds" \
        "$BINARY" --source "$SOURCE_DIR" --target "$TARGET_DIR" --force

    target_file=$(find "$TARGET_DIR" -name "*.txt" -type f | head -1)
    content=$(cat "$target_file")
    assert_contains "Content was updated" "$content" "updated content"
finish_test || exit 1

exit 0
