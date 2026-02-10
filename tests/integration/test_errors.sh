#!/bin/sh

set -eu
. "$(dirname "$0")/assertions.sh"

echo "Error Handling Tests"

SOURCE_DIR="$TEST_DIR/source"
TARGET_DIR="$TEST_DIR/target"

test_group "Invalid source directory"
rm -rf "$SOURCE_DIR" "$TARGET_DIR"
mkdir -p "$TARGET_DIR"

output=$("$BINARY" --source "$SOURCE_DIR/nonexistent" \
                   --target "$TARGET_DIR" 2>&1 || true)

assert_contains "Reports invalid source path" "$output" "Error:"
finish_test || exit 1

test_group "Source is a file, not directory"
rm -rf "$SOURCE_DIR" "$TARGET_DIR"
mkdir -p "$(dirname "$SOURCE_DIR")"
mkdir -p "$TARGET_DIR"
echo "not a directory" > "$SOURCE_DIR"

output=$("$BINARY" --source "$SOURCE_DIR" \
                   --target "$TARGET_DIR" 2>&1 || true)

assert_contains "Reports source not a directory" "$output" "Error:"
rm "$SOURCE_DIR"
finish_test || exit 1

test_group "File collision error message"
rm -rf "$SOURCE_DIR" "$TARGET_DIR"
mkdir -p "$SOURCE_DIR" "$TARGET_DIR"
create_test_file "$SOURCE_DIR/test.txt"

"$BINARY" --source "$SOURCE_DIR" \
          --target "$TARGET_DIR" \
          --tag "collision"

output=$("$BINARY" --source "$SOURCE_DIR" \
                   --target "$TARGET_DIR" \
                   --tag "collision" 2>&1 || true)

assert_contains "Reports collision" "$output" "already exists"
finish_test || exit 1

test_group "Empty source directory"
rm -rf "$SOURCE_DIR" "$TARGET_DIR"
mkdir -p "$SOURCE_DIR" "$TARGET_DIR"

output=$("$BINARY" --source "$SOURCE_DIR" \
                   --target "$TARGET_DIR" \
                   --tag "empty" --verbose 2>&1 || true)

assert_contains "Reports zero files" "$output" "Found 0 files"
finish_test || exit 1

test_group "Permission handling"
rm -rf "$SOURCE_DIR" "$TARGET_DIR"
mkdir -p "$SOURCE_DIR" "$TARGET_DIR"

test_file="$SOURCE_DIR/restricted.txt"
create_test_file "$test_file" "secret"
chmod 000 "$test_file"

output=$("$BINARY" --source "$SOURCE_DIR" \
                   --target "$TARGET_DIR" \
                   --tag "perm" 2>&1 || true)

chmod 644 "$test_file" 2>/dev/null || true

if echo "$output" | grep -qi "error\|permission\|denied\|access"; then
    printf "    ${GREEN}[OK]${NC} Permission error reported\n"
else
    printf "    ${RED}[FAIL]${NC} No permission error in output\n"
    FAILED_ASSERTIONS=$((FAILED_ASSERTIONS + 1))
fi
ASSERTION_COUNT=$((ASSERTION_COUNT + 1))
finish_test || exit 1

exit 0
