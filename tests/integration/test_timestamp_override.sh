#!/bin/sh

set -eu
. "$(dirname "$0")/assertions.sh"

SOURCE_DIR="$TEST_DIR/source"
TARGET_DIR="$TEST_DIR/target"

setup() {
    rm -rf "$SOURCE_DIR" "$TARGET_DIR"
    mkdir -p "$SOURCE_DIR" "$TARGET_DIR"
}

# `find -newer` only fails on errors, not on "no match" -- it always
# exits 0 when the listing succeeds, even if empty. So "is $1 newer than
# $2" has to be read from find's *output*, not its exit status.
not_newer() {
    [ -z "$(find "$1" -newer "$2" 2>/dev/null)" ]
}

# NOTE: real_timestamp comes from st_ctime, which the kernel always sets
# to "now" on creation -- there is no portable, non-root way to give a
# test fixture file an arbitrary past ctime. So this file cannot test:
#   - the same-date precondition actually *rejecting* differing dates
#   - per-date numbering actually *resetting* across two different dates
# (both require two files with genuinely different ctimes within one
# index). Both code paths are exercised instead by M2's target-directory
# scan tests, since those dates come from filenames, not ctime.

test_group "Absolute date override"
    setup
    create_test_file "$SOURCE_DIR/a.jpg"
    create_test_file "$SOURCE_DIR/b.jpg"

    assert_success "Just works" \
        "$BINARY" --source "$SOURCE_DIR" --target "$TARGET_DIR" \
                  --date "2020-03-05"

    assert_file_exists "First file at index 000" \
        "$TARGET_DIR/2020-03-05_000.jpg"
    assert_file_exists "Second file at index 001 (same-date numbering)" \
        "$TARGET_DIR/2020-03-05_001.jpg"
finish_test || exit 1

test_group "Destination mtime/atime match override date"
    setup
    create_test_file "$SOURCE_DIR/a.jpg"

    "$BINARY" --source "$SOURCE_DIR" --target "$TARGET_DIR" \
              --date "2020-03-05" > /dev/null

    touch -t 202003050000 "$TEST_DIR/day_start"
    touch -t 202003060000 "$TEST_DIR/day_end"

    assert_success "mtime not before override day" \
        not_newer "$TEST_DIR/day_start" "$TARGET_DIR/2020-03-05_000.jpg"
    assert_success "mtime not after override day" \
        not_newer "$TARGET_DIR/2020-03-05_000.jpg" "$TEST_DIR/day_end"
finish_test || exit 1

test_group "Set individual date components"
    setup
    create_test_file "$SOURCE_DIR/a.jpg"

    assert_success "Just works" \
        "$BINARY" --source "$SOURCE_DIR" --target "$TARGET_DIR" \
                  --set-year 2021 --set-month 7 --set-day 9

    assert_file_exists "Exact date applied" "$TARGET_DIR/2021-07-09_000.jpg"
finish_test || exit 1

test_group "Date offset: year"
    setup
    create_test_file "$SOURCE_DIR/a.jpg"
    today_year=$(date -u +%Y)
    expected_year=$((today_year + 1))

    assert_success "Just works" \
        "$BINARY" --source "$SOURCE_DIR" --target "$TARGET_DIR" \
                  --date "+1Y"

    target_file=$(find "$TARGET_DIR" -type f | head -1)
    filename=$(basename "$target_file")
    assert_contains "Year incremented by one" "$filename" "${expected_year}-"
finish_test || exit 1

test_group "Date offset: repeated --date flags accumulate"
    setup
    create_test_file "$SOURCE_DIR/a.jpg"
    today_year=$(date -u +%Y)
    expected_year=$((today_year + 2))

    assert_success "Just works" \
        "$BINARY" --source "$SOURCE_DIR" --target "$TARGET_DIR" \
                  --date "+1Y" --date "+1Y"

    target_file=$(find "$TARGET_DIR" -type f | head -1)
    filename=$(basename "$target_file")
    assert_contains "Offsets accumulated" "$filename" "${expected_year}-"
finish_test || exit 1

test_group "Date offset: month (with year rollover)"
    setup
    create_test_file "$SOURCE_DIR/a.jpg"
    today_year=$(date -u +%Y)
    today_month=$(date -u +%m | sed 's/^0//')
    total_months=$((today_year * 12 + (today_month - 1) + 5))
    expected_year=$((total_months / 12))
    expected_month=$((total_months % 12 + 1))
    expected_month_padded=$(printf '%02d' "$expected_month")

    assert_success "Just works" \
        "$BINARY" --source "$SOURCE_DIR" --target "$TARGET_DIR" \
                  --date "+5M"

    target_file=$(find "$TARGET_DIR" -type f | head -1)
    filename=$(basename "$target_file")
    assert_contains "Month offset normalized correctly" "$filename" \
        "${expected_year}-${expected_month_padded}-"
finish_test || exit 1

test_group "Date offset: day moves date forward"
    setup
    create_test_file "$SOURCE_DIR/a.jpg"
    baseline_date=$(date -u +%Y-%m-%d)

    assert_success "Just works" \
        "$BINARY" --source "$SOURCE_DIR" --target "$TARGET_DIR" \
                  --date "+10D"

    target_file=$(find "$TARGET_DIR" -type f | head -1)
    filename=$(basename "$target_file")
    new_date=$(echo "$filename" | cut -c1-10)
    assert_success "Resulting date is chronologically later" \
        test "$new_date" '>' "$baseline_date"
finish_test || exit 1

test_group "Mutually exclusive overrides rejected"
    setup
    create_test_file "$SOURCE_DIR/a.jpg"

    assert_failure "offset + set-year rejected" \
        "$BINARY" --source "$SOURCE_DIR" --target "$TARGET_DIR" \
                  --date "+1Y" --set-year 2020
    assert_failure "offset + absolute date rejected" \
        "$BINARY" --source "$SOURCE_DIR" --target "$TARGET_DIR" \
                  --set-day 1 --date "+1Y"
    assert_file_count "Nothing copied on rejected combo" "$TARGET_DIR" 0
finish_test || exit 1

test_group "Malformed override values rejected"
    setup
    create_test_file "$SOURCE_DIR/a.jpg"

    assert_failure "invalid absolute date" \
        "$BINARY" --source "$SOURCE_DIR" --target "$TARGET_DIR" \
                  --date "2020-13-01"
    assert_failure "invalid offset letter" \
        "$BINARY" --source "$SOURCE_DIR" --target "$TARGET_DIR" \
                  --date "+1Z"
    assert_failure "out-of-range set-month" \
        "$BINARY" --source "$SOURCE_DIR" --target "$TARGET_DIR" \
                  --set-month 13
    assert_failure "non-positive set-year" \
        "$BINARY" --source "$SOURCE_DIR" --target "$TARGET_DIR" \
                  --set-year 0
finish_test || exit 1

exit 0
