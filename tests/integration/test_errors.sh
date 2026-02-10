#!/bin/sh

set -eu
. "$(dirname "$0")/assertions.sh"

SOURCE_DIR="$TEST_DIR/source"
TARGET_DIR="$TEST_DIR/target"

test_group "Invalid source directory"
rm -rf "$SOURCE_DIR" "$TARGET_DIR"
mkdir -p "$TARGET_DIR"

output=$("$BINARY" --source "$SOURCE_DIR/nonexistent" \
                   --target "$TARGET_DIR" 2>&1 || true)

assert_contains "Error reported" "$output" "Error:"
assert_contains "Path reported" "$output" "$SOURCE_DIR/nonexistent"
finish_test || exit 1

test_group "Source is not a directory"
rm -rf "$SOURCE_DIR" "$TARGET_DIR"
mkdir -p "$(dirname "$SOURCE_DIR")"
mkdir -p "$TARGET_DIR"
echo "not a directory" > "$SOURCE_DIR"

output=$("$BINARY" --source "$SOURCE_DIR" \
                   --target "$TARGET_DIR" 2>&1 || true)

assert_contains "Error reported" "$output" "Error:"
assert_contains "Path reported" "$output" "$SOURCE_DIR"
rm "$SOURCE_DIR"
finish_test || exit 1

test_group "File collision"
rm -rf "$SOURCE_DIR" "$TARGET_DIR"
mkdir -p "$SOURCE_DIR" "$TARGET_DIR"
create_test_file "$SOURCE_DIR/test.txt"

"$BINARY" --source "$SOURCE_DIR" \
          --target "$TARGET_DIR" \
          --tag "collision"

output=$("$BINARY" --source "$SOURCE_DIR" \
                   --target "$TARGET_DIR" \
                   --tag "collision" 2>&1 || true)

assert_contains "Error reported" "$output" "Error:"
assert_contains "Cause reported" "$output" "already exists"
assert_contains "Force suggested" "$output" "--force"
finish_test || exit 1

test_group "Empty source directory"
rm -rf "$SOURCE_DIR" "$TARGET_DIR"
mkdir -p "$SOURCE_DIR" "$TARGET_DIR"

output=$("$BINARY" --source "$SOURCE_DIR" \
                   --target "$TARGET_DIR" \
                   --tag "empty" --verbose 2>&1 || true)

assert_contains "File count reported" "$output" "Found 0 files"
assert_contains "Warning reported" "$output" "Warning:"
finish_test || exit 1

test_group "Access to file denied"
rm -rf "$SOURCE_DIR" "$TARGET_DIR"
mkdir -p "$SOURCE_DIR" "$TARGET_DIR"

test_file="$SOURCE_DIR/restricted.txt"
create_test_file "$test_file" "secret"
chmod 000 "$test_file"

output=$("$BINARY" --source "$SOURCE_DIR" \
                   --target "$TARGET_DIR" \
                   --tag "perm" 2>&1 || true)

chmod 644 "$test_file" 2>/dev/null || true
assert_contains "Error reported" "$output" "Error"
assert_contains "Path reported" "$output" "$SOURCE_DIR"
assert_contains "Cause reported" "$output" "permission denied"
finish_test || exit 1

exit 0
