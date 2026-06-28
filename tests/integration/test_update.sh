#!/bin/sh

set -eu
. "$(dirname "$0")/assertions.sh"

SOURCE_DIR="$TEST_DIR/source"
TARGET_DIR="$TEST_DIR/target"

setup() {
    rm -rf "$SOURCE_DIR" "$TARGET_DIR"
    mkdir -p "$SOURCE_DIR" "$TARGET_DIR"
}

# `touch -t` interprets its argument as *local* time, not UTC, which
# would silently shift the calendar day corgi's gmtime-based logic sees
# on a runner with a non-zero UTC offset. `TZ=UTC` makes "local" mean
# UTC, portably across GNU/BSD/macOS touch (unlike GNU-only `touch -d`).
set_mtime_utc() {
    TZ=UTC touch -t "$1" "$2"
}

today() {
    date -u +%Y-%m-%d
}

today_stamp() {
    date -u +%Y%m%d0000
}

test_group "Missing target directory: copy all, start at index 0"
    setup
    rm -rf "$TARGET_DIR"
    create_test_file "$SOURCE_DIR/a.jpg"

    assert_success "Just works" \
        "$BINARY" --source "$SOURCE_DIR" --target "$TARGET_DIR" --update

    assert_file_exists "Copied at index 000" \
        "$TARGET_DIR/$(today)_000.jpg"
finish_test || exit 1

test_group "Target with no organized filenames: copy all"
    setup
    echo "not organized" > "$TARGET_DIR/readme.txt"
    create_test_file "$SOURCE_DIR/a.jpg"

    assert_success "Just works" \
        "$BINARY" --source "$SOURCE_DIR" --target "$TARGET_DIR" --update

    assert_file_exists "Non-matching file untouched" "$TARGET_DIR/readme.txt"
    assert_file_exists "Copied at index 000" \
        "$TARGET_DIR/$(today)_000.jpg"
finish_test || exit 1

test_group "Target older than source: copy all, continue per-date index"
    setup
    target_file="$TARGET_DIR/$(today)_003_vacation.jpg"
    create_test_file "$target_file"
    set_mtime_utc "$(today_stamp)" "$target_file"
    create_test_file "$SOURCE_DIR/a.jpg"

    assert_success "Just works" \
        "$BINARY" --source "$SOURCE_DIR" --target "$TARGET_DIR" --update

    assert_file_exists "Existing target file kept" "$target_file"
    assert_file_exists "New file continues numbering at 004" \
        "$TARGET_DIR/$(today)_004.jpg"
finish_test || exit 1

test_group "Target newer than source: nothing copied"
    setup
    target_file="$TARGET_DIR/2099-01-01_000_future.jpg"
    create_test_file "$target_file"
    set_mtime_utc "209901010000" "$target_file"
    create_test_file "$SOURCE_DIR/a.jpg"

    output=$("$BINARY" --source "$SOURCE_DIR" --target "$TARGET_DIR" \
                       --update 2>&1)

    assert_contains "Nothing-to-process warning reported" "$output" \
        "No files to process"
    assert_file_count "Only the pre-existing target file remains" \
        "$TARGET_DIR" 1
finish_test || exit 1

test_group "Target file with filename/mtime mismatch is skipped, not fatal"
    setup
    drifted_file="$TARGET_DIR/2020-01-01_007_old.jpg"
    create_test_file "$drifted_file"
    set_mtime_utc "$(today_stamp)" "$drifted_file"
    create_test_file "$SOURCE_DIR/a.jpg"

    output=$("$BINARY" --source "$SOURCE_DIR" --target "$TARGET_DIR" \
                       --update 2>&1)

    assert_contains "Mismatch reported" "$output" "date mismatch"
    assert_file_exists \
        "Copy still proceeds at index 000 (drifted entry excluded)" \
        "$TARGET_DIR/$(today)_000.jpg"
finish_test || exit 1

test_group "Update with dry-run: no files written, count reflects filter"
    setup
    target_file="$TARGET_DIR/2099-01-01_000_future.jpg"
    create_test_file "$target_file"
    set_mtime_utc "209901010000" "$target_file"
    create_test_file "$SOURCE_DIR/a.jpg"
    create_test_file "$SOURCE_DIR/b.jpg"

    output=$("$BINARY" --source "$SOURCE_DIR" --target "$TARGET_DIR" \
                       --update --dry-run --verbose 2>&1)

    assert_contains "Filtered count reported" "$output" "0 files newer"
    assert_file_count "No new files written" "$TARGET_DIR" 1
finish_test || exit 1

exit 0
