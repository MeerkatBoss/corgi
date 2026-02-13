#!/bin/sh

set -eu
. "$(dirname "$0")/assertions.sh"

SOURCE_DIR="$TEST_DIR/source"
TARGET_DIR="$TEST_DIR/target"

test_group "Valid tag"
    rm -rf "$SOURCE_DIR" "$TARGET_DIR"
    mkdir -p "$SOURCE_DIR" "$TARGET_DIR"
    create_test_file "$SOURCE_DIR/test.txt"

    assert_success "Lowercase letters accepted" \
        "$BINARY" --source "$SOURCE_DIR" --target "$TARGET_DIR" \
                  --tag "vacation" --dry-run

    assert_success "Dashes accepted" \
        "$BINARY" --source "$SOURCE_DIR" --target "$TARGET_DIR" \
                  --tag "summer-vacation" --dry-run

    assert_success "Multiple dashes accepted" \
        "$BINARY" --source "$SOURCE_DIR" --target "$TARGET_DIR" \
                  --tag "my-summer-vacation-photos" --dry-run
finish_test || exit 1

test_group "Invalid tag"
    rm -rf "$SOURCE_DIR" "$TARGET_DIR"
    mkdir -p "$SOURCE_DIR" "$TARGET_DIR"
    create_test_file "$SOURCE_DIR/test.txt"

    output=$("$BINARY" --source "$SOURCE_DIR" \
                       --target "$TARGET_DIR" \
                       --tag "Invalid" 2>&1 || true)
    assert_contains "Uppercase rejected" "$output" "invalid characters"

    output=$("$BINARY" --source "$SOURCE_DIR" \
                       --target "$TARGET_DIR" \
                       --tag "tag123" 2>&1 || true)
    assert_contains "Numbers rejected" "$output" "invalid characters"

    output=$("$BINARY" --source "$SOURCE_DIR" \
                       --target "$TARGET_DIR" \
                       --tag "tag_underscore" 2>&1 || true)
    assert_contains "Underscores rejected" "$output" "invalid characters"

    output=$("$BINARY" --source "$SOURCE_DIR" \
                       --target "$TARGET_DIR" \
                       --tag "tag.dot" 2>&1 || true)
    assert_contains "Dots rejected" "$output" "invalid characters"

    output=$("$BINARY" --source "$SOURCE_DIR" \
                       --target "$TARGET_DIR" \
                       --tag "tag space" 2>&1 || true)
    assert_contains "Spaces rejected" "$output" "invalid characters"
finish_test || exit 1

exit 0